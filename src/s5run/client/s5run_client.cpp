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

#include "s5run_client.h"
#include "protocol/s5run_channel.h"
#include "protocol/s5run_wire.h"
#include "s5run_globals.h"
#include "server/s5run_variable_wire.h"

#include <chrono>
#include <cstdio>

namespace s5run {

std::string value_to_display_string(const variable &v) {
  switch (v.kind()) {
  case variable::K_UNDEFINED:
    return "undefined";
  case variable::K_NULL:
    return "null";
  case variable::K_BOOL:
    return v.as_bool() ? "true" : "false";
  case variable::K_INT: {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v.as_int()));
    return buf;
  }
  case variable::K_DOUBLE: {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%g", v.as_double());
    return buf;
  }
  case variable::K_STRING:
    return v.as_string();
  case variable::K_ARRAY: {
    std::string out = "[";
    const variable_array &arr = v.as_array();
    for (size_t i = 0; i < arr.size(); ++i) {
      if (i > 0)
        out += ", ";
      out += value_to_display_string(arr[i]);
    }
    out += "]";
    return out;
  }
  case variable::K_OBJECT: {
    std::string out = "{";
    const variable_object &obj = v.as_object();
    bool first = true;
    for (const auto &kv : obj) {
      if (!first)
        out += ", ";
      first = false;
      out += kv.first + ": " + value_to_display_string(kv.second);
    }
    out += "}";
    return out;
  }
  case variable::K_REF:
    return "[ref " + v.as_ref().type + "#" + std::to_string(v.as_ref().this_id) + "]";
  }
  return "<unknown>";
}

void console_object::log(const std::vector<std::string> &args) {
  std::string line;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      line += " ";
    line += args[i];
  }
  std::printf("%s\n", line.c_str());
  std::fflush(stdout);
}

void console_object::warning(const std::vector<std::string> &args) {
  std::string line = "[warning] ";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      line += " ";
    line += args[i];
  }
  std::fprintf(stderr, "%s\n", line.c_str());
  std::fflush(stderr);
}

void console_object::error(const std::vector<std::string> &args) {
  std::string line = "[error] ";
  for (size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      line += " ";
    line += args[i];
  }
  std::fprintf(stderr, "%s\n", line.c_str());
  std::fflush(stderr);
}

client_context::client_context()
    : m_channel(nullptr), m_console(&m_default_console), m_console_registered(false),
      m_console_id(0), m_next_id(0) {
}

client_context::~client_context() {
  disconnect();
}

void client_context::connect(const std::string &host, uint16_t port) {
  disconnect();
  m_channel = new channel(host, port); // connect + hello
}

void client_context::disconnect() {
  delete m_channel;
  m_channel = nullptr;
  m_console_registered = false;
  m_console_id = 0;
}

bool client_context::is_connected() const {
  return m_channel != nullptr && m_channel->is_connected();
}

void client_context::register_console(console_object *console) {
  if (console != nullptr)
    m_console = console;
  if (m_channel == nullptr)
    throw std::runtime_error("client_context: not connected");

  m_console_id = ++m_next_id;
  send_register_object("console", m_console_id, {"log", "warning", "error"});
  m_console_registered = true;
}

void client_context::send_register_object(const std::string &name, int32_t this_id,
                                          const std::vector<std::string> &methods) {
  wire::Writer w;
  w.put_string(name);
  w.put_i32(this_id);
  w.put_string(name); // type name
  w.put_u32(static_cast<uint32_t>(methods.size()));
  for (const auto &m : methods)
    w.put_string(m);

  uint8_t resp_type = 0;
  std::vector<uint8_t> resp;
  if (!m_channel->send_and_expect(protocol_msg_type::M_REGISTER_OBJ, w.data(),
                                  protocol_msg_type::M_REGISTER_RESP, resp)) {
    resp_type = 0;
    throw std::runtime_error("register_object(" + name + "): no response");
  }
  wire::Reader r(resp.data(), resp.size());
  if (r.get_u8() == 0)
    throw std::runtime_error("register_object(" + name + "): " + r.get_string());
}

void client_context::add_provider(const std::string &host, uint16_t port,
                                  const std::vector<std::string> &names) {
  if (m_channel == nullptr)
    throw std::runtime_error("client_context: not connected");

  // ask the server to mount the entries from the remote provider;
  // the server connects to the provider itself and relays callbacks
  wire::Writer w;
  w.put_string(host);
  w.put_u16(port);
  w.put_u32(static_cast<uint32_t>(names.size()));
  for (const auto &name : names)
    w.put_string(name);

  uint8_t resp_type = 0;
  std::vector<uint8_t> resp;
  if (!m_channel->send_and_expect(protocol_msg_type::M_PROVIDER_MOUNT_REQ, w.data(),
                                  protocol_msg_type::M_PROVIDER_MOUNT_RESP, resp))
    throw std::runtime_error("no response from the server");
  wire::Reader r(resp.data(), resp.size());
  if (r.get_u8() == 0)
    throw std::runtime_error(r.get_string());
}

void client_context::send_callback_ok(int32_t cb_id, const variable &result) {
  wire::Writer w;
  w.put_i32(cb_id);
  w.put_u8(1);
  value_to_wire(result, w);
  m_channel->send(protocol_msg_type::M_CALLBACK_RESP, w.data());
}

void client_context::send_callback_error(int32_t cb_id, const std::string &error) {
  wire::Writer w;
  w.put_i32(cb_id);
  w.put_u8(0);
  w.put_string(error);
  m_channel->send(protocol_msg_type::M_CALLBACK_RESP, w.data());
}

void client_context::dispatch_callback(const std::vector<uint8_t> &body) {
  wire::Reader r(body.data(), body.size());
  int32_t cb_id = r.get_i32();
  std::string method = r.get_string();
  uint32_t argc = r.get_u32();

  try {
    // args[0] is the this_id of the target object
    if (argc < 1)
      throw std::runtime_error(method + ": expected this_id");
    variable this_v = value_from_wire(r);
    if (!this_v.is_int())
      throw std::runtime_error(method + ": bad this_id");

    std::vector<std::string> args;
    args.reserve(argc - 1);
    for (uint32_t i = 1; i < argc; ++i)
      args.push_back(value_to_display_string(value_from_wire(r)));

    if (method == "log")
      m_console->log(args);
    else if (method == "warning")
      m_console->warning(args);
    else if (method == "error")
      m_console->error(args);
    else
      throw std::runtime_error("console: unknown method: " + method);

    send_callback_ok(cb_id, variable()); // undefined/null result
  } catch (const std::exception &e) {
    send_callback_error(cb_id, e.what());
  }
}

run_result client_context::run(const std::string &script, const std::string &entry,
                               const std::vector<variable> &args) {
  if (m_channel == nullptr)
    throw std::runtime_error("client_context: not connected");

  auto started = std::chrono::steady_clock::now();

  run_result result;


  wire::Writer w;
  w.put_string(script);
  w.put_string(entry);
  w.put_u32(static_cast<uint32_t>(args.size()));
  for (const auto &a : args)
    value_to_wire(a, w);

  if (!m_channel->send(protocol_msg_type::M_EXEC_REQ, w.data())) {
    result.error = "run: send failed";
    return result;
  }

  while (true) {
    uint8_t msg_type = 0;
    std::vector<uint8_t> body;
    if (!m_channel->recv(msg_type, body)) {
      result.error = "run: connection closed";
      break;
    }
    if (msg_type == protocol_msg_type::M_CALLBACK_REQ) {
      dispatch_callback(body);
      continue;
    }
    if (msg_type == protocol_msg_type::M_EXEC_RESP) {
      wire::Reader r(body.data(), body.size());
      if (r.get_u8() == 0) {
        result.ok = false;
        result.error = r.get_string();
      } else {
        result.ok = true;
        result.value = value_from_wire(r);
      }
      break;
    }
    result.error = "run: unexpected message type";
    break;
  }

  auto finished = std::chrono::steady_clock::now();
  result.duration_ms =
    std::chrono::duration<double, std::milli>(finished - started).count();
  return result;
}

} // namespace s5run
