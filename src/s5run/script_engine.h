#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "json_mini.h"

namespace engine {

// Opaque handles. The public interface knows nothing about the
// underlying engine (QuickJS, Lua, V8, ...).
using JsContext = void*;
using JsValue   = void*;

struct ExecResult {
    bool ok = false;
    std::string error;
    json::Value value;
};

// Simple callback: the engine converts JS args to JSON, calls the handler,
// then converts the JSON result back to a JS value.
using CallbackHandler = std::function<json::Value(
    const std::string& method, const json::Array& args)>;

// JS callback: the handler receives an opaque context and must return
// an opaque value, typically produced by make_object(). The engine
// takes ownership of the returned handle and releases it after use.
using JsCallbackHandler = std::function<JsValue(
    JsContext ctx, const json::Array& args)>;

class IScriptEngine {
public:
    virtual ~IScriptEngine() = default;

    // Plain host function returning JSON-convertible data.
    virtual void register_callback(const std::string& method,
                                   CallbackHandler handler) = 0;

    // Host function that builds a JS object manually.
    virtual void register_js_callback(const std::string& method,
                                      JsCallbackHandler handler) = 0;

    // Build a JS object with method stubs. Each method, when called from
    // JS, is dispatched as a simple callback with signature
    //   method(this_id, ...js_args)
    // and must return a JSON-convertible value.
    virtual JsValue make_object(JsContext ctx,
                                int64_t this_id,
                                const json::Array& methods) = 0;

    virtual ExecResult execute(const std::string& script,
                               const std::string& entry,
                               const json::Array& args) = 0;
};

std::unique_ptr<IScriptEngine> make_quickjs_engine();

} // namespace engine