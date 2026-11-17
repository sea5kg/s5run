// src/quickjs_value.cpp
#include "quickjs_value.h"

#include <stdexcept>
#include <string>

namespace example {

JSValue value_to_js(JSContext *ctx, const Value &v) {
  switch (v.kind()) {
  case Value::K_NULL:
    return JS_NULL;
  case Value::K_BOOL:
    return JS_NewBool(ctx, v.as_bool());
  case Value::K_INT:
    return JS_NewInt64(ctx, v.as_int());
  case Value::K_DOUBLE:
    return JS_NewFloat64(ctx, v.as_double());
  case Value::K_STRING:
    return JS_NewStringLen(ctx, v.as_string().data(), v.as_string().size());
  case Value::K_ARRAY: {
    const auto &a = v.as_array();
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
  case Value::K_OBJECT: {
    const auto &o = v.as_object();
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
  case Value::K_REF: {
    // Represent an ObjectRef as {"this_id": N} in JS.
    JSValue obj = JS_NewObject(ctx);
    if (JS_IsException(obj))
      return obj;
    JS_SetPropertyStr(ctx, obj, "this_id", JS_NewInt32(ctx, v.as_ref().this_id));
    return obj;
  }
  }
  return JS_UNDEFINED;
}

Value js_to_value(JSContext *ctx, JSValueConst v) {
  if (JS_IsNull(v) || JS_IsUndefined(v))
    return Value();
  if (JS_IsBool(v))
    return Value((bool)JS_ToBool(ctx, v));
  if (JS_IsNumber(v)) {
    double d;
    if (JS_ToFloat64(ctx, &d, v) < 0)
      throw std::runtime_error("js_to_value: number conversion failed");
    if (d == (double)(int64_t)d)
      return Value((int64_t)d);
    return Value(d);
  }
  if (JS_IsString(v)) {
    const char *s = JS_ToCString(ctx, v);
    if (!s)
      throw std::runtime_error("js_to_value: string conversion failed");
    std::string out(s);
    JS_FreeCString(ctx, s);
    return Value(out);
  }
  if (JS_IsArray(ctx, v)) {
    // Iterate array elements.
    JSValue len_val = JS_GetPropertyStr(ctx, v, "length");
    uint32_t len = 0;
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    ValueArray a;
    a.reserve(len);
    for (uint32_t i = 0; i < len; ++i) {
      JSValue elem = JS_GetPropertyUint32(ctx, v, i);
      if (JS_IsException(elem)) {
        JS_FreeValue(ctx, elem);
        throw std::runtime_error("js_to_value: array element failed");
      }
      a.push_back(js_to_value(ctx, elem));
      JS_FreeValue(ctx, elem);
    }
    return Value(a);
  }
  if (JS_IsObject(v)) {
    // Try ObjectRef: {"this_id": N}
    JSValue id_val = JS_GetPropertyStr(ctx, v, "this_id");
    if (!JS_IsUndefined(id_val)) {
      int32_t id = 0;
      if (JS_ToInt32(ctx, &id, id_val) == 0) {
        JS_FreeValue(ctx, id_val);
        return Value(ObjectRef(id));
      }
      JS_FreeValue(ctx, id_val);
    }

    // Generic object: iterate own property names.
    JSValue keys = JS_GetOwnPropertyNames(ctx, &v, JS_GPN_STRING_MASK);
    // Note: quickjs-ng signature may differ; fall back to JSON.stringify.
    // Simpler: use JSON.stringify and parse — but that defeats the purpose.
    // For now, iterate via JS_GetPropertyStr on known keys is not possible.
    // Use JS_GetOwnPropertyNames if available.
    throw std::runtime_error("js_to_value: object not supported yet");
  }
  return Value();
}

} // namespace example