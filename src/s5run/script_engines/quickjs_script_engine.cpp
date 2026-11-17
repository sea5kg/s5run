#include "script_engine.h"

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

json::Value js_to_json(JSContext *ctx, JSValueConst v) {
  if (JS_IsNull(v) || JS_IsUndefined(v))
    return json::Value();
  if (JS_IsBool(v))
    return json::Value((bool)JS_ToBool(ctx, v));
  if (JS_IsNumber(v)) {
    double d = 0;
    JS_ToFloat64(ctx, &d, v);
    return json::Value(d);
  }
  if (JS_IsString(v)) {
    const char *s = JS_ToCString(ctx, v);
    std::string out = s ? s : "";
    if (s)
      JS_FreeCString(ctx, s);
    return json::Value(out);
  }
  // Objects/arrays: round-trip through JSON.stringify.
  JSValue json_str = JS_JSONStringify(ctx, v, JS_UNDEFINED, JS_UNDEFINED);
  if (JS_IsException(json_str)) {
    JS_FreeValue(ctx, json_str);
    return json::Value();
  }
  const char *cstr = JS_ToCString(ctx, json_str);
  json::Value out;
  if (cstr) {
    try {
      out = json::parse(cstr);
    } catch (...) {
      out = json::Value();
    }
    JS_FreeCString(ctx, cstr);
  }
  JS_FreeValue(ctx, json_str);
  return out;
}

JSValue json_to_js(JSContext *ctx, const json::Value &v) {
  std::string s = json::dump(v);
  std::string code = "JSON.parse(" + s + ")";
  return JS_Eval(ctx, code.data(), code.size(), "<json>", JS_EVAL_TYPE_GLOBAL);
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
    JS_FreeValue(ctx_.get(), holder);

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
    JS_FreeValue(ctx_.get(), holder);

    JSValue global = JS_GetGlobalObject(ctx_.get());
    JS_SetPropertyStr(ctx_.get(), global, method.c_str(), fn);
    JS_FreeValue(ctx_.get(), global);
  }

  JsValue make_object(JsContext ctx_handle, int64_t this_id, const json::Array &methods) override {
    JSContext *ctx = to_ctx(ctx_handle);

    JSValue obj = JS_NewObject(ctx);
    if (JS_IsException(obj))
      return nullptr;

    for (const auto &m : methods) {
      if (!m.is_str())
        continue;
      std::string name = m.as_str();

      auto *mctx = new MethodCtx{this_id, name};
      method_ctxs_.push_back(mctx);

      JSValue holder = wrap_opaque(method_class_id_, mctx);
      JSValue fn = make_cfunction(&QuickJSEngine::js_method_trampoline, method_class_id_, holder);
      JS_FreeValue(ctx, holder);

      JS_SetPropertyStr(ctx, obj, name.c_str(), fn);
    }
    return to_handle(obj);
  }

  ExecResult execute(const std::string &script, const std::string &entry, const json::Array &args) override {
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

    JSValue js_args = json_array_to_js(args);
    if (JS_IsException(js_args)) {
      JS_FreeValue(ctx_.get(), fn);
      res.error = "args: " + take_exception();
      return res;
    }

    int argc = static_cast<int>(args.size());
    std::vector<JSValue> argv(argc);
    for (int i = 0; i < argc; ++i)
      argv[i] = JS_GetPropertyUint32(ctx_.get(), js_args, i);

    JSValue call_res = JS_Call(ctx_.get(), fn, JS_UNDEFINED, argc, argv.data());

    for (auto &v : argv)
      JS_FreeValue(ctx_.get(), v);
    JS_FreeValue(ctx_.get(), js_args);
    JS_FreeValue(ctx_.get(), fn);

    if (JS_IsException(call_res)) {
      res.error = "call: " + take_exception();
      return res;
    }

    JSValue json_str = JS_JSONStringify(ctx_.get(), call_res, JS_UNDEFINED, JS_UNDEFINED);
    JS_FreeValue(ctx_.get(), call_res);

    if (JS_IsException(json_str)) {
      res.error = "stringify: " + take_exception();
      return res;
    }
    if (JS_IsUndefined(json_str)) {
      res.ok = true;
      res.value = json::Value();
      return res;
    }
    const char *cstr = JS_ToCString(ctx_.get(), json_str);
    if (!cstr) {
      JS_FreeValue(ctx_.get(), json_str);
      res.error = "stringify: cannot get string";
      return res;
    }
    try {
      res.value = json::parse(cstr);
      res.ok = true;
    } catch (const std::exception &e) {
      res.error = std::string("json parse: ") + e.what();
    }
    JS_FreeCString(ctx_.get(), cstr);
    JS_FreeValue(ctx_.get(), json_str);
    return res;
  }

  // Used by the trampolines.
  json::Value dispatch_simple(const std::string &method, const json::Array &args) {
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

    json::Array args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
      args.push_back(js_to_json(ctx, argv[i]));

    try {
      json::Value r = eng->dispatch_simple(cctx->method, args);
      return json_to_js(ctx, r);
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

    json::Array args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
      args.push_back(js_to_json(ctx, argv[i]));

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

  // Object method: prepend this_id to args, dispatch as simple callback.
  static JSValue
  js_method_trampoline(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv, int magic, JSValue *func_data) {
    auto *eng = static_cast<QuickJSEngine *>(JS_GetContextOpaque(ctx));
    if (!eng)
      return JS_ThrowTypeError(ctx, "engine context missing");

    auto *mctx = static_cast<MethodCtx *>(JS_GetOpaque(func_data[0], static_cast<JSClassID>(magic)));
    if (!mctx)
      return JS_ThrowTypeError(ctx, "invalid method context");

    json::Array args;
    args.reserve(1 + argc);
    args.push_back(json::Value((double)mctx->this_id));
    for (int i = 0; i < argc; ++i)
      args.push_back(js_to_json(ctx, argv[i]));

    try {
      json::Value r = eng->dispatch_simple(mctx->method, args);
      return json_to_js(ctx, r);
    } catch (const std::exception &e) {
      return JS_ThrowInternalError(ctx, "%s", e.what());
    }
  }

  // -----------------------------------------------------------
  // Helpers
  // -----------------------------------------------------------
  JSValue json_array_to_js(const json::Array &args) {
    std::string arr = json::dump(json::Value(args));
    JSValue global = JS_GetGlobalObject(ctx_.get());
    JSValue json_obj = JS_GetPropertyStr(ctx_.get(), global, "JSON");
    JSValue parse_fn = JS_GetPropertyStr(ctx_.get(), json_obj, "parse");
    JSValue arg = JS_NewStringLen(ctx_.get(), arr.data(), arr.size());
    JSValue res = JS_Call(ctx_.get(), parse_fn, json_obj, 1, &arg);
    JS_FreeValue(ctx_.get(), arg);
    JS_FreeValue(ctx_.get(), parse_fn);
    JS_FreeValue(ctx_.get(), json_obj);
    JS_FreeValue(ctx_.get(), global);
    return res;
  }

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

  std::vector<CallbackCtx *> callback_ctxs_;
  std::vector<MethodCtx *> method_ctxs_;
};

} // namespace

std::shared_ptr<IScriptEngine> make_quickjs_engine() {
  return std::make_shared<QuickJSEngine>();
}

} // namespace engine