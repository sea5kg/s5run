// include/quickjs_value.h
#pragma once

#include "value.h"

extern "C" {
#include "quickjs.h"
}

namespace example {

// Convert a Value to a JSValue. Caller owns the returned JSValue.
JSValue value_to_js(JSContext* ctx, const Value& v);

// Convert a JSValue to a Value. Throws std::runtime_error on failure.
Value js_to_value(JSContext* ctx, JSValueConst v);

} // namespace example
