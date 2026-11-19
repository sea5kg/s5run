/*
 * MIT License
 *
 *           ░░░░░░░
 *          ░██▓▓▓▓▒
 * ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 * ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 * ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
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
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "s5run_value.h"

namespace engine {

// Opaque handles. The public interface knows nothing about the
// underlying engine (QuickJS, Lua, V8, ...).
using JsContext = void *;
using JsValue = void *;

struct ExecResult {
  bool ok = false;
  std::string error;
  s5run::Value value;
};

// Simple callback: the engine converts JS args to values, calls the
// handler, then converts the result back to a JS value.
using CallbackHandler = std::function<s5run::Value(
    const std::string &method, const std::vector<s5run::Value> &args)>;

// JS callback: the handler receives an opaque context and must return
// an opaque value, typically produced by make_object(). The engine
// takes ownership of the returned handle and releases it after use.
using JsCallbackHandler = std::function<JsValue(
    JsContext ctx, const std::vector<s5run::Value> &args)>;

class IScriptEngine {
public:
  virtual ~IScriptEngine() = default;

  // Plain host function returning a value.
  virtual void register_callback(const std::string &method, CallbackHandler handler) = 0;

  // Host function that builds a JS object manually.
  virtual void register_js_callback(const std::string &method, JsCallbackHandler handler) = 0;

  // Build a JS object with method stubs. Each method, when called from
  // JS, is dispatched as a simple callback with signature
  //   method(this_id, ...js_args)
  virtual JsValue make_object(JsContext ctx, int64_t this_id, const std::vector<std::string> &methods) = 0;

  // Install a JS object with method stubs as a global property, so
  // scripts call it as `name.method(...)`.
  virtual void register_global_object(const std::string &name, int64_t this_id,
                                      const std::vector<std::string> &methods) = 0;

  // Build a JS array from opaque item handles. Takes ownership of the
  // items and releases them after use.
  virtual JsValue make_array(JsContext ctx, size_t count, const JsValue *items) = 0;

  virtual JsValue make_value(JsContext ctx, const s5run::Value &v) = 0;

  virtual ExecResult execute(const std::string &script, const std::string &entry,
                             const std::vector<s5run::Value> &args) = 0;
};

std::shared_ptr<IScriptEngine> make_quickjs_engine();

} // namespace engine
