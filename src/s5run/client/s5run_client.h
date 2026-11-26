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

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "../engine/s5run_variable.h"

namespace s5run {

class channel;

// Result of a remote script execution.
struct run_result {
  bool ok = false;
  std::string error;
  variable value;
  double duration_ms = 0.0;
};

// Convert a wire value to a display string (for console methods and result printing).
std::string value_to_display_string(const variable &v);

// Built-in console object: log -> stdout, warning/error -> stderr.
// Inherit and override to customize.
class console_object {
public:
  virtual ~console_object() {
  }
  virtual void log(const std::vector<std::string> &args);
  virtual void warning(const std::vector<std::string> &args);
  virtual void error(const std::vector<std::string> &args);
};

// Remote context provider entry (from the provider metadata):
// a function, a class or a global object to mount into the script.
struct provider_entry {
  enum kind_t { FUNCTION = 1, CLASS = 2, OBJECT = 3 };
  uint8_t kind = 0;
  std::string name;
  std::vector<std::string> methods; // method names (classes/objects)
  std::vector<std::string> prop_names;
  std::vector<std::string> prop_getters;
  std::vector<std::string> prop_setters;
  int32_t this_id = 0; // objects
};

// Client-side execution context: connects to a server, registers the built-in
// console object and runs scripts. Server-side callbacks (console.log etc.)
// are dispatched locally while the script is running. Context entries from
// remote providers (functions/classes/objects) are mounted on the server and
// their callbacks are relayed to the provider.
class client_context {
public:
  client_context();
  ~client_context();

  client_context(const client_context &) = delete;
  client_context &operator=(const client_context &) = delete;

  // Connect to the server (performs the hello handshake).
  void connect(const std::string &host, uint16_t port);
  void disconnect();
  bool is_connected() const;

  // Register the built-in console object. Call after connect(), before run().
  // Pass nullptr to use the default implementation.
  void register_console(console_object *console);

  // Connect to a remote context provider (host:port) and fetch metadata for
  // the given names. The entries are registered on the server inside run()
  // and their callbacks are relayed to the provider.
  void add_provider(const std::string &host, uint16_t port,
                    const std::vector<std::string> &names);

  // Run a script on the server and pump callbacks until it finishes.
  run_result run(const std::string &script, const std::string &entry,
                 const std::vector<variable> &args);

private:
  void send_register_object(const std::string &name, int32_t this_id,
                            const std::vector<std::string> &methods);
  void send_register_type(const std::string &name,
                          const std::vector<std::string> &methods,
                          const provider_entry &entry);
  void send_register_function(const std::string &name);
  void register_provider_entries();
  void send_callback_ok(int32_t cb_id, const variable &result);
  void send_callback_error(int32_t cb_id, const std::string &error);
  void dispatch_callback(const std::vector<uint8_t> &body);
  bool relay_to_provider(size_t provider_index, const std::vector<uint8_t> &body);

  channel *m_channel;
  console_object *m_console;
  console_object m_default_console;
  bool m_console_registered;
  int32_t m_console_id;
  int32_t m_next_id;

  // remote providers: connection + mounted entries
  struct provider_link {
    channel *ch;
    std::vector<provider_entry> entries;
  };
  std::vector<provider_link> m_providers;
  std::map<std::string, size_t> m_method_owner; // method/function name -> provider
};

} // namespace s5run
