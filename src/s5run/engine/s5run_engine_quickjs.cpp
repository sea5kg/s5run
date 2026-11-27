/**********************************************************************************
 *
 *            ░░░░░░░
 *           ░██▓▓▓▓▒
 *  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 *  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 *  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
 *
 * MIT License
 *
 * Copyright (c) 2026 Evgenii Sopov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Original repository: https://github.com/sea5kg/s5run
 *
 ***********************************************************************************/

#include "s5run_engine.h"

extern "C" {

#include "quickjs.h"
}

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

namespace engine {
namespace {

// ---------------------------------------------------------------
// RAII wrappers
// ---------------------------------------------------------------

struct RuntimeDeleter {
  void operator()(JSRuntime *rt) const {
    JS_FreeRuntime(rt);
  }
};

struct ContextDeleter {
  void operator()(JSContext *ctx) const {
    JS_FreeContext(ctx);
  }
};

// ---------------------------------------------------------------
// Opaque contexts stored inside JS objects
// ---------------------------------------------------------------

// Context for a global host function (getFile, readLine, ...).
struct CallbackCtx {
  class QuickJSEngine *engine;
  std::string method;
};

// Context for an object method (file.size, file.read, ...).
struct MethodCtx {
  int64_t this_id;
  std::string method;
};

// Context for an object property accessor (file.name, ...).
struct PropCtx {
  int64_t this_id;
  std::string getter; // "" - write-only
  std::string setter; // "" - read-only
};

// ---------------------------------------------------------------
// Opaque handle helpers
// ---------------------------------------------------------------

inline JSContext *to_ctx(JsContext c) {
  return static_cast<JSContext *>(c);
}

inline JsValue to_handle(JSValue v) {
  return new JSValue(v);
}

inline JSValue from_handle(JsValue h) {
  return *static_cast<JSValue *>(h);
}

inline void free_handle(JsValue h) {
  delete static_cast<JSValue *>(h);
}

// ---------------------------------------------------------------
// JSON <-> JSValue helpers
// ---------------------------------------------------------------

s5run::variable js_to_value(JSContext *ctx, JSValueConst v) {
  if (JS_IsNull(v))
    return s5run::variable(nullptr); // JS null
  if (JS_IsUndefined(v))
    return s5run::variable(); // JS undefined
  if (JS_IsBool(v))
    return s5run::variable((bool)JS_ToBool(ctx, v));
  if (JS_IsNumber(v)) {
    double d = 0;
    JS_ToFloat64(ctx, &d, v);
    if (d == (double)(int64_t)d)
      return s5run::variable((int64_t)d);
    return s5run::variable(d);
  }
  if (JS_IsString(v)) {
    const char *s = JS_ToCString(ctx, v);
    std::string out = s ? s : "";
    if (s)
      JS_FreeCString(ctx, s);
    return s5run::variable(out);
  }
  if (JS_IsArray(v)) {
    JSValue len_val = JS_GetPropertyStr(ctx, v, "length");
    uint32_t len = 0;
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);
    s5run::variable_array out;
    out.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
      JSValue elem = JS_GetPropertyUint32(ctx, v, i);
      out.push_back(js_to_value(ctx, elem));
      JS_FreeValue(ctx, elem);
    }
    return s5run::variable(out);
  }
  if (JS_IsObject(v)) {
    JSValue marker = JS_GetPropertyStr(ctx, v, "__s5run_this_id");
    if (JS_IsNumber(marker)) {
      double d = 0;
      JS_ToFloat64(ctx, &d, marker);
      JS_FreeValue(ctx, marker);
      return s5run::variable(s5run::object_ref((int)d, ""));
    }
    JS_FreeValue(ctx, marker);

    JSPropertyEnum *tab = nullptr;
    uint32_t len = 0;
    if (JS_GetOwnPropertyNames(ctx, &tab, &len, v, JS_GPN_STRING_MASK) < 0)
      return s5run::variable();
    if (len == 1) {
      const char *name = JS_AtomToCString(ctx, tab[0].atom);
      bool is_this_id = name && strcmp(name, "this_id") == 0;
      JS_FreeCString(ctx, name);
      if (is_this_id) {
        JSValue id_val = JS_GetProperty(ctx, v, tab[0].atom);
        if (JS_IsNumber(id_val)) {
          double d = 0;
          JS_ToFloat64(ctx, &d, id_val);
          JS_FreeValue(ctx, id_val);
          JS_FreePropertyEnum(ctx, tab, len);
          return s5run::variable(s5run::object_ref((int)d, ""));
        }
        JS_FreeValue(ctx, id_val);
      }
    }
    s5run::variable_object out;
    for (uint32_t i = 0; i < len; ++i) {
      const char *name = JS_AtomToCString(ctx, tab[i].atom);
      if (!name)
        continue;
      std::string key = name;
      JS_FreeCString(ctx, name);
      JSValue prop = JS_GetProperty(ctx, v, tab[i].atom);
      if (!JS_IsFunction(ctx, prop))
        out[key] = js_to_value(ctx, prop);
      JS_FreeValue(ctx, prop);
    }
    JS_FreePropertyEnum(ctx, tab, len);
    return s5run::variable(out);
  }
  return s5run::variable();
}

JSValue value_to_js(JSContext *ctx, const s5run::variable &v) {
  switch (v.kind()) {
  case s5run::variable::K_UNDEFINED:
    return JS_UNDEFINED;
  case s5run::variable::K_NULL:
    return JS_NULL;
  case s5run::variable::K_BOOL:
    return JS_NewBool(ctx, v.as_bool());
  case s5run::variable::K_INT:
    return JS_NewInt64(ctx, v.as_int());
  case s5run::variable::K_DOUBLE:
    return JS_NewFloat64(ctx, v.as_double());
  case s5run::variable::K_STRING:
    return JS_NewStringLen(ctx, v.as_string().data(), v.as_string().size());
  case s5run::variable::K_ARRAY: {
    const s5run::variable_array &a = v.as_array();
    JSValue arr = JS_NewArray(ctx);
    if (JS_IsException(arr))
      return arr;
    for (uint32_t i = 0; i < a.size(); ++i) {
      JSValue elem = value_to_js(ctx, a[i]);
      if (JS_IsException(elem)) {
        JS_FreeValue(ctx, arr);
        return elem;
      }
      JS_SetPropertyUint32(ctx, arr, i, elem); // consumes elem
    }
    return arr;
  }
  case s5run::variable::K_OBJECT: {
    const s5run::variable_object &o = v.as_object();
    JSValue obj = JS_NewObject(ctx);
    if (JS_IsException(obj))
      return obj;
    for (const auto &kv : o) {
      JSValue elem = value_to_js(ctx, kv.second);
      if (JS_IsException(elem)) {
        JS_FreeValue(ctx, obj);
        return elem;
      }
      JS_SetPropertyStr(ctx, obj, kv.first.c_str(), elem); // consumes elem
    }
    return obj;
  }
  case s5run::variable::K_REF: {
    // Represent an object_ref as {"this_id": N} in JS.
    JSValue obj = JS_NewObject(ctx);
    if (JS_IsException(obj))
      return obj;
    JS_SetPropertyStr(ctx, obj, "this_id", JS_NewInt32(ctx, v.as_ref().this_id));
    return obj;
  }
  }
  return JS_UNDEFINED;
}

// ---------------------------------------------------------------
// QuickJSEngine
// ---------------------------------------------------------------

class QuickJSEngine : public IScriptEngine {
public:
  QuickJSEngine() {
    rt_.reset(JS_NewRuntime());
    if (!rt_)
      throw std::runtime_error("JS_NewRuntime failed");
    JS_SetMaxStackSize(rt_.get(), 2 * 1024 * 1024);

    ctx_.reset(JS_NewContext(rt_.get()));
    if (!ctx_)
      throw std::runtime_error("JS_NewContext failed");

    JS_SetContextOpaque(ctx_.get(), this); // <-- added
    register_classes();
  }

  ~QuickJSEngine() override {
    // Registries own the opaque contexts and outlive any JS object,
    // so we free them manually here.
    for (auto *p : callback_ctxs_)
      delete p;
    for (auto *p : method_ctxs_)
      delete p;
  }

  // -----------------------------------------------------------
  // Public API
  // -----------------------------------------------------------

  void register_callback(const std::string &method, CallbackHandler handler) override {
    simple_callbacks_[method] = std::move(handler);

    auto *cctx = new CallbackCtx{this, method};
    callback_ctxs_.push_back(cctx);

    JSValue holder = wrap_opaque(callback_class_id_, cctx);
    JSValue fn = make_cfunction(&QuickJSEngine::js_global_trampoline, callback_class_id_, holder);

    JSValue global = JS_GetGlobalObject(ctx_.get());
    JS_SetPropertyStr(ctx_.get(), global, method.c_str(), fn);
    JS_FreeValue(ctx_.get(), global);
  }

  void register_js_callback(const std::string &method, JsCallbackHandler handler) override {
    js_callbacks_[method] = std::move(handler);

    auto *cctx = new CallbackCtx{this, method};
    callback_ctxs_.push_back(cctx);

    JSValue holder = wrap_opaque(callback_class_id_, cctx);
    JSValue fn = make_cfunction(&QuickJSEngine::js_global_js_trampoline, callback_class_id_, holder);

    JSValue global = JS_GetGlobalObject(ctx_.get());
    JS_SetPropertyStr(ctx_.get(), global, method.c_str(), fn);
    JS_FreeValue(ctx_.get(), global);
  }

  void register_constructor(const std::string &name, JsCallbackHandler handler) override {
    // C functions created by QuickJS are not constructors, so the public
    // name is a plain JS function (created via eval - JS functions ARE
    // constructors) that forwards to a hidden C callback. When the handler
    // returns a proxy object, 'new name(...)' yields that object.
    std::string hidden = "__s5run_ctor_" + name;
    register_js_callback(hidden, std::move(handler));

    std::string code = "function " + name + "(...args) { return " + hidden + "(...args); }";
    JSValue eval_res = JS_Eval(ctx_.get(), code.data(), code.size(), "<ctor>",
                               JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(eval_res)) {
      JS_FreeValue(ctx_.get(), eval_res);
      throw std::runtime_error("register_constructor: eval failed for " + name);
    }
    JS_FreeValue(ctx_.get(), eval_res);
  }

  JsValue make_object(JsContext ctx_handle, int64_t this_id,
                       const std::vector<std::string> &methods,
                       const std::vector<object_property> &properties) override {
    JSContext *ctx = to_ctx(ctx_handle);

    JSValue obj = JS_NewObject(ctx);
    if (JS_IsException(obj))
      return nullptr;

    // Hidden marker: lets js_to_value recognize proxy objects and pass
    // them to callbacks as object_ref instead of a plain object.
    JS_DefinePropertyValueStr(ctx, obj, "__s5run_this_id",
                              JS_NewInt64(ctx, this_id), 0);

    for (const auto &name : methods) {
      auto *mctx = new MethodCtx{this_id, name};
      method_ctxs_.push_back(mctx);

      JSValue holder = wrap_opaque(method_class_id_, mctx);
      JSValue fn = make_cfunction(&QuickJSEngine::js_method_trampoline, method_class_id_, holder);

      JS_SetPropertyStr(ctx, obj, name.c_str(), fn);
    }

    for (const auto &prop : properties) {
      auto *pctx = new PropCtx{this_id, prop.getter, prop.setter};
      prop_ctxs_.push_back(pctx);

      JSValue getter = JS_UNDEFINED;
      JSValue setter = JS_UNDEFINED;
      if (!prop.getter.empty()) {
        JSValue holder = wrap_opaque(prop_class_id_, pctx);
        getter = make_cfunction(&QuickJSEngine::js_prop_getter_trampoline, prop_class_id_, holder);
      }
      if (!prop.setter.empty()) {
        JSValue holder = wrap_opaque(prop_class_id_, pctx);
        setter = make_cfunction(&QuickJSEngine::js_prop_setter_trampoline, prop_class_id_, holder);
      }

      JSAtom atom = JS_NewAtom(ctx, prop.name.c_str());
      int prop_flags = JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE
                       | JS_PROP_HAS_CONFIGURABLE | JS_PROP_HAS_ENUMERABLE;
      if (!prop.getter.empty())
        prop_flags |= JS_PROP_HAS_GET;
      if (!prop.setter.empty())
        prop_flags |= JS_PROP_HAS_SET;
      JS_DefineProperty(ctx, obj, atom, JS_UNDEFINED, getter, setter, prop_flags);
      JS_FreeAtom(ctx, atom);
    }
    return to_handle(obj);
  }

  JsValue make_array(JsContext ctx_handle, size_t count, const JsValue *items) override {
    JSContext *ctx = to_ctx(ctx_handle);
    JSValue arr = JS_NewArray(ctx);
    if (JS_IsException(arr)) {
      for (size_t i = 0; i < count; ++i)
        free_handle(items[i]);
      return nullptr;
    }
    for (size_t i = 0; i < count; ++i) {
      JSValue v = from_handle(items[i]);
      free_handle(items[i]);
      if (JS_SetPropertyUint32(ctx, arr, (uint32_t)i, v) < 0) {
        JS_FreeValue(ctx, v);
        JS_FreeValue(ctx, arr);
        for (size_t j = i + 1; j < count; ++j)
          free_handle(items[j]);
        return nullptr;
      }
    }
    return to_handle(arr);
  }

  JsValue make_value(JsContext ctx_handle, const s5run::variable &v) override {
    JSContext *ctx = to_ctx(ctx_handle);
    return to_handle(value_to_js(ctx, v));
  }

  void register_global_object(const std::string &name, int64_t this_id,
                              const std::vector<std::string> &methods,
                              const std::vector<object_property> &properties) override {
    JsValue handle = make_object(ctx_.get(), this_id, methods, properties);
    JSValue obj = from_handle(handle);
    free_handle(handle);
    JSValue global = JS_GetGlobalObject(ctx_.get());
    JS_SetPropertyStr(ctx_.get(), global, name.c_str(), obj);
    JS_FreeValue(ctx_.get(), global);
  }

  ExecResult execute(const std::string &script, const std::string &entry,
                     const std::vector<s5run::variable> &args) override {
    ExecResult res;

    JSValue eval_res = JS_Eval(ctx_.get(), script.data(), script.size(), "<script>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(eval_res)) {
      res.error = "eval: " + take_exception();
      return res;
    }
    JS_FreeValue(ctx_.get(), eval_res);

    JSValue global = JS_GetGlobalObject(ctx_.get());
    JSValue fn = JS_GetPropertyStr(ctx_.get(), global, entry.c_str());
    JS_FreeValue(ctx_.get(), global);

    if (!JS_IsFunction(ctx_.get(), fn)) {
      JS_FreeValue(ctx_.get(), fn);
      res.error = "entry not a function: " + entry;
      return res;
    }

    int argc = static_cast<int>(args.size());
    std::vector<JSValue> argv(argc);
    for (int i = 0; i < argc; ++i) {
      argv[i] = value_to_js(ctx_.get(), args[i]);
      if (JS_IsException(argv[i])) {
        for (int j = 0; j < i; ++j)
          JS_FreeValue(ctx_.get(), argv[j]);
        JS_FreeValue(ctx_.get(), fn);
        res.error = "args: conversion failed";
        return res;
      }
    }

    JSValue call_res = JS_Call(ctx_.get(), fn, JS_UNDEFINED, argc, argv.data());

    for (auto &v : argv)
      JS_FreeValue(ctx_.get(), v);
    JS_FreeValue(ctx_.get(), fn);

    if (JS_IsException(call_res)) {
      res.error = "call: " + take_exception();
      return res;
    }

    try {
      res.value = js_to_value(ctx_.get(), call_res);
      res.ok = true;
    } catch (const std::exception &e) {
      res.error = std::string("result conversion: ") + e.what();
    }
    JS_FreeValue(ctx_.get(), call_res);
    return res;
  }

  // Used by the trampolines.
  s5run::variable dispatch_simple(const std::string &method, const std::vector<s5run::variable> &args) {
    auto it = simple_callbacks_.find(method);
    if (it == simple_callbacks_.end())
      throw std::runtime_error("callback not registered: " + method);
    return it->second(method, args);
  }

  JsCallbackHandler *find_js_callback(const std::string &method) {
    auto it = js_callbacks_.find(method);
    if (it == js_callbacks_.end())
      return nullptr;
    return &it->second;
  }

private:
  // -----------------------------------------------------------
  // Class registration
  // -----------------------------------------------------------
  void register_classes() {
    JS_NewClassID(rt_.get(), &callback_class_id_);
    JSClassDef cb_def{};
    cb_def.class_name = "CallbackCtxHolder";
    if (JS_NewClass(rt_.get(), callback_class_id_, &cb_def) < 0)
      throw std::runtime_error("JS_NewClass(callback) failed");

    JS_NewClassID(rt_.get(), &method_class_id_);
    JSClassDef m_def{};
    m_def.class_name = "MethodCtxHolder";
    if (JS_NewClass(rt_.get(), method_class_id_, &m_def) < 0)
      throw std::runtime_error("JS_NewClass(method) failed");

    JS_NewClassID(rt_.get(), &prop_class_id_);
    JSClassDef p_def{};
    p_def.class_name = "PropCtxHolder";
    if (JS_NewClass(rt_.get(), prop_class_id_, &p_def) < 0)
      throw std::runtime_error("JS_NewClass(prop) failed");
  }

  // Wrap a raw pointer into a JS object of the given class.
  JSValue wrap_opaque(JSClassID cid, void *ptr) {
    JSValue obj = JS_NewObjectClass(ctx_.get(), cid);
    if (JS_IsException(obj))
      return obj;
    JS_SetOpaque(obj, ptr);
    return obj;
  }

  // Create a C function whose func_data carries the opaque holder.
  // The class ID is passed via magic; the engine is stored in the
  // JSContext opaque.
  JSValue make_cfunction(JSCFunctionData *fn, JSClassID cid, JSValue holder) {
    JSValue data[1] = {holder};
    JSValue out = JS_NewCFunctionData(
      ctx_.get(),
      fn,
      /*length=*/0,
      /*magic=*/static_cast<int>(cid),
      /*data_len=*/1,
      data
    );
    JS_FreeValue(ctx_.get(), holder);
    return out;
  }

  // -----------------------------------------------------------
  // Trampolines
  // -----------------------------------------------------------

  // Simple callback: JS args -> JSON -> handler -> JSON -> JS value.
  static JSValue
  js_global_trampoline(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
    auto *eng = static_cast<QuickJSEngine *>(JS_GetContextOpaque(ctx));
    if (!eng)
      return JS_ThrowTypeError(ctx, "engine context missing");

    auto *cctx = static_cast<CallbackCtx *>(JS_GetOpaque(func_data[0], static_cast<JSClassID>(magic)));
    if (!cctx)
      return JS_ThrowTypeError(ctx, "invalid callback context");

    std::vector<s5run::variable> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
      args.push_back(js_to_value(ctx, argv[i]));

    try {
      s5run::variable r = eng->dispatch_simple(cctx->method, args);
      return value_to_js(ctx, r);
    } catch (const std::exception &e) {
      return JS_ThrowInternalError(ctx, "%s", e.what());
    }
  }

  // JS callback: handler builds the JS value itself.
  static JSValue
  js_global_js_trampoline(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
    auto *eng = static_cast<QuickJSEngine *>(JS_GetContextOpaque(ctx));
    if (!eng)
      return JS_ThrowTypeError(ctx, "engine context missing");

    auto *cctx = static_cast<CallbackCtx *>(JS_GetOpaque(func_data[0], static_cast<JSClassID>(magic)));
    if (!cctx)
      return JS_ThrowTypeError(ctx, "invalid callback context");

    std::vector<s5run::variable> args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
      args.push_back(js_to_value(ctx, argv[i]));

    auto *handler = eng->find_js_callback(cctx->method);
    if (!handler)
      return JS_ThrowInternalError(ctx, "js callback not registered: %s", cctx->method.c_str());
    try {
      JsValue handle = (*handler)(ctx, args);
      if (!handle)
        return JS_UNDEFINED;
      JSValue out = from_handle(handle);
      free_handle(handle);
      return out;
    } catch (const std::exception &e) {
      return JS_ThrowInternalError(ctx, "%s", e.what());
    }
  }

  // Object property getter: dispatch as simple callback [this_id].
  static JSValue
  js_prop_getter_trampoline(JSContext *ctx, JSValueConst, int, JSValueConst *, int magic, JSValue *func_data) {
    auto *eng = static_cast<QuickJSEngine *>(JS_GetContextOpaque(ctx));
    if (!eng)
      return JS_ThrowTypeError(ctx, "engine context missing");
    auto *pctx = static_cast<PropCtx *>(JS_GetOpaque(func_data[0], static_cast<JSClassID>(magic)));
    if (!pctx || pctx->getter.empty())
      return JS_ThrowTypeError(ctx, "property has no getter");

    std::vector<s5run::variable> args;
    args.push_back(s5run::variable((int64_t)pctx->this_id));
    try {
      s5run::variable r = eng->dispatch_simple(pctx->getter, args);
      return value_to_js(ctx, r);
    } catch (const std::exception &e) {
      return JS_ThrowInternalError(ctx, "%s", e.what());
    }
  }

  // Object property setter: dispatch as simple callback [this_id, value].
  static JSValue
  js_prop_setter_trampoline(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
    auto *eng = static_cast<QuickJSEngine *>(JS_GetContextOpaque(ctx));
    if (!eng)
      return JS_ThrowTypeError(ctx, "engine context missing");
    auto *pctx = static_cast<PropCtx *>(JS_GetOpaque(func_data[0], static_cast<JSClassID>(magic)));
    if (!pctx || pctx->setter.empty())
      return JS_ThrowTypeError(ctx, "property has no setter");

    std::vector<s5run::variable> args;
    args.push_back(s5run::variable((int64_t)pctx->this_id));
    if (argc > 0)
      args.push_back(js_to_value(ctx, argv[0]));
    try {
      eng->dispatch_simple(pctx->setter, args);
    } catch (const std::exception &e) {
      return JS_ThrowInternalError(ctx, "%s", e.what());
    }
    return JS_UNDEFINED;
  }

  // Object method: prepend this_id to args, dispatch as simple callback.
  static JSValue
  js_method_trampoline(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
    auto *eng = static_cast<QuickJSEngine *>(JS_GetContextOpaque(ctx));
    if (!eng)
      return JS_ThrowTypeError(ctx, "engine context missing");

    auto *mctx = static_cast<MethodCtx *>(JS_GetOpaque(func_data[0], static_cast<JSClassID>(magic)));
    if (!mctx)
      return JS_ThrowTypeError(ctx, "invalid method context");

    std::vector<s5run::variable> args;
    args.reserve(1 + argc);
    args.push_back(s5run::variable((int64_t)mctx->this_id));
    for (int i = 0; i < argc; ++i)
      args.push_back(js_to_value(ctx, argv[i]));

    try {
      s5run::variable r = eng->dispatch_simple(mctx->method, args);
      return value_to_js(ctx, r);
    } catch (const std::exception &e) {
      return JS_ThrowInternalError(ctx, "%s", e.what());
    }
  }

  // -----------------------------------------------------------
  // Helpers
  // -----------------------------------------------------------
  std::string take_exception() {
    JSValue exc = JS_GetException(ctx_.get());
    const char *s = JS_ToCString(ctx_.get(), exc);
    std::string out = s ? s : "unknown";
    if (s)
      JS_FreeCString(ctx_.get(), s);
    if (JS_IsObject(exc)) {
      JSValue stack = JS_GetPropertyStr(ctx_.get(), exc, "stack");
      if (!JS_IsUndefined(stack)) {
        const char *ss = JS_ToCString(ctx_.get(), stack);
        if (ss) {
          out += "\n";
          out += ss;
          JS_FreeCString(ctx_.get(), ss);
        }
        JS_FreeValue(ctx_.get(), stack);
      }
    }
    JS_FreeValue(ctx_.get(), exc);
    return out;
  }

  // -----------------------------------------------------------
  // Fields
  // -----------------------------------------------------------
  std::unique_ptr<JSRuntime, RuntimeDeleter> rt_;
  std::unique_ptr<JSContext, ContextDeleter> ctx_;

  std::map<std::string, CallbackHandler> simple_callbacks_;
  std::map<std::string, JsCallbackHandler> js_callbacks_;

  JSClassID callback_class_id_ = 0;
  JSClassID method_class_id_ = 0;
  JSClassID prop_class_id_ = 0;

  std::vector<CallbackCtx *> callback_ctxs_;
  std::vector<MethodCtx *> method_ctxs_;
  std::vector<PropCtx *> prop_ctxs_;
};

} // namespace

std::shared_ptr<IScriptEngine> make_quickjs_engine() {
  return std::make_shared<QuickJSEngine>();
}

} // namespace engine
