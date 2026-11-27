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

#include "s5run_globals.h"

#include "s5run_thread_pool.h"
#include "s5run_queue.h"
#include "protocol/s5run_protocol.h"
#include "s5run_server.h"
#include "engine/s5run_engine.h"
#include "s5run_variable_wire.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace s5run {

std::atomic<bool> g_stop{false};
std::atomic<int> g_connected_clients{0};
job_queue *g_queue = nullptr;

static int g_listen_fd = -1;

namespace {

// Synchronous callback channel: sends a request to the client and waits
// for the response. Assumes one in-flight message per direction.
class ClientChannel {
public:
  explicit ClientChannel(int fd) : fd_(fd) {
  }

  variable call(const std::string &method, const std::vector<variable> &args) {
    int32_t id = ++last_id_;
    wire::Writer w;
    w.put_i32(id);
    w.put_string(method);
    w.put_u32(static_cast<uint32_t>(args.size()));
    for (const auto &a : args)
      value_to_wire(a, w);

    if (!s5run::write_message(fd_, protocol_msg_type::M_CALLBACK_REQ, w.data()))
      throw std::runtime_error("callback: write failed");

    uint8_t msg_type = 0;
    std::vector<uint8_t> body;
    if (!s5run::read_message(fd_, msg_type, body))
      throw std::runtime_error("callback: no response (timeout/eof)");
    if (msg_type != protocol_msg_type::M_CALLBACK_RESP)
      throw std::runtime_error("callback: unexpected message type");

    wire::Reader r(body.data(), body.size());
    if (r.get_i32() != id)
      throw std::runtime_error("callback: id mismatch");
    if (r.get_u8() == 0)
      throw std::runtime_error("callback '" + method + "': " + r.get_string());
    return value_from_wire(r);
  }

private:
  int fd_;
  int32_t last_id_ = 0;
};


} // namespace

class client_handler : public thread_pool_task {
public:
  client_handler(int fd, thread_pool *pool);
  virtual void handle() override;

private:
  struct object_type_info {
    std::vector<std::string> methods;
    std::vector<engine::object_property> properties;
  };

  int m_fd;
  thread_pool *m_pool;
  std::map<std::string, object_type_info> m_object_types;

  static std::vector<engine::object_property> parse_properties(wire::Reader &r,
                                                               const std::string &where) {
    std::vector<engine::object_property> out;
    if (r.remaining() < 4)
      return out; // old clients send no properties
    uint32_t count = r.get_u32();
    out.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
      engine::object_property p;
      p.name = r.get_string();
      p.getter = r.get_string();
      p.setter = r.get_string();
      if (p.name.empty())
        throw std::runtime_error(where + ": empty property name");
      out.push_back(p);
    }
    return out;
  }
  void handle_register_function(engine::IScriptEngine *, const std::vector<uint8_t> &body);
  void handle_register_object(engine::IScriptEngine *, const std::vector<uint8_t> &body);
  void handle_register_object_type(engine::IScriptEngine *, const std::vector<uint8_t> &body);
  void handle_exec(engine::IScriptEngine *, const std::vector<uint8_t> &body, int fd);
  void send_exec_error(int fd, std::string);
  void send_stats(int fd);
  void handle_job_submit(const std::vector<uint8_t> &body, int fd);
  void handle_job_result(const std::vector<uint8_t> &body, int fd);
  void send_register_resp(int fd, bool, std::string);
  // Register method callbacks dispatching to the given fd (the client or
  // a mounted provider).
  void register_method_callbacks(engine::IScriptEngine *, int fd,
                                const std::vector<std::string> &methods);
  // Register a global function callback dispatched to the given fd.
  void register_function_callback(engine::IScriptEngine *, const std::string &name, int fd);
  // Convert a callback response to a JS value (refs become proxy objects).
  engine::JsValue to_js_value(engine::IScriptEngine *eng, engine::JsContext ctx,
                              const variable &resp);
  // Mount context entries from a remote provider (server connects,
  // fetches the metadata and registers the entries; callbacks are
  // relayed to the provider directly).
  void handle_provider_mount(engine::IScriptEngine *eng, const std::vector<uint8_t> &body, int fd);
  // provider connections (closed when the client disconnects)
  std::vector<int> m_provider_fds;
};

client_handler::client_handler(int fd, thread_pool *pool)
    : m_fd(fd), m_pool(pool) {

      };

void client_handler::handle() {
  g_connected_clients.fetch_add(1);
  s5run::set_recv_timeout(m_fd, 10'000);

  // Greet the client: "s5run <server-version> <protocol-version>"
  {
    std::string hello = std::string("s5run ") + WSJCPP_APP_VERSION + " "
                        + std::to_string(S5RUN_PROTOCOL_VERSION);
    s5run::write_message(m_fd, protocol_msg_type::M_SERVER_HELLO,
                         std::vector<uint8_t>(hello.begin(), hello.end()));
  }

  std::shared_ptr<engine::IScriptEngine> eng;
  try {
    eng = engine::make_quickjs_engine();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "engine init: %s\n", e.what());
    ::close(m_fd);
    return;
  }

  while (true) {
    uint8_t msg_type;
    std::vector<uint8_t> body;
    if (!s5run::read_message(m_fd, msg_type, body))
      break;

    try {
      switch (msg_type) {
      case protocol_msg_type::M_REGISTER_FUNC:
        handle_register_function(eng.get(), body);
        send_register_resp(m_fd, true, "");
        break;
      case protocol_msg_type::M_REGISTER_OBJ:
        handle_register_object(eng.get(), body);
        send_register_resp(m_fd, true, "");
        break;
      case protocol_msg_type::M_REGISTER_TYPE:
        handle_register_object_type(eng.get(), body);
        send_register_resp(m_fd, true, "");
        break;
      case protocol_msg_type::M_PROVIDER_MOUNT_REQ:
        handle_provider_mount(eng.get(), body, m_fd);
        break;
      case protocol_msg_type::M_STATS_REQ:
        send_stats(m_fd);
        break;
      case protocol_msg_type::M_JOB_SUBMIT:
        handle_job_submit(body, m_fd);
        break;
      case protocol_msg_type::M_JOB_RESULT_REQ:
        handle_job_result(body, m_fd);
        break;
      case protocol_msg_type::M_EXEC_REQ:
        handle_exec(eng.get(), body, m_fd);
        break;
      default:
        throw std::runtime_error("unknown message type");
      }
    } catch (const std::exception &e) {
      if (msg_type == protocol_msg_type::M_EXEC_REQ) {
        send_exec_error(m_fd, e.what());
      } else {
        send_register_resp(m_fd, false, e.what());
      }
    }
  }
  g_connected_clients.fetch_sub(1);
  for (int pfd : m_provider_fds)
    ::close(pfd);
  ::close(m_fd);
}

void client_handler::handle_job_submit(const std::vector<uint8_t> &body, int fd) {
  wire::Reader r(body.data(), body.size());
  std::string script = r.get_string();
  std::string entry = r.get_string();
  std::vector<uint8_t> args_blob;
  {
    // re-encode args with the count prefix for the queue worker
    uint32_t count = r.get_u32();
    wire::Writer w;
    w.put_u32(count);
    for (uint32_t i = 0; i < count; ++i) {
      variable v = value_from_wire(r);
      value_to_wire(v, w);
    }
    args_blob = w.data();
  }
  std::string uuid = g_queue->submit(script, entry, args_blob);
  wire::Writer w;
  w.put_u8(1);
  w.put_string(uuid);
  s5run::write_message(fd, protocol_msg_type::M_JOB_SUBMIT_RESP, w.data());
}

void client_handler::handle_job_result(const std::vector<uint8_t> &body, int fd) {
  wire::Reader r(body.data(), body.size());
  std::string uuid = r.get_string();
  job_queue::job_info job;
  wire::Writer w;
  if (!g_queue || !g_queue->get(uuid, job)) {
    w.put_u8(0xFF); // unknown
    s5run::write_message(fd, protocol_msg_type::M_JOB_RESULT_RESP, w.data());
    return;
  }
  w.put_u8((uint8_t)job.state);
  if (job.has_result) {
    w.put_u8(job.result_ok ? 1 : 0);
    if (job.result_ok)
      w.put_bytes(job.result_value.data(), job.result_value.size());
    else
      w.put_string(job.result_error);
  }
  s5run::write_message(fd, protocol_msg_type::M_JOB_RESULT_RESP, w.data());
}

void client_handler::send_stats(int fd) {
  wire::Writer w;
  w.put_u32((uint32_t)(m_pool ? m_pool->threads_count() : 0));
  w.put_u32((uint32_t)(m_pool ? m_pool->busy_count() : 0));
  w.put_u32((uint32_t)(m_pool ? m_pool->queued_count() : 0));
  w.put_u32((uint32_t)g_connected_clients.load());
  s5run::write_message(fd, protocol_msg_type::M_STATS_RESP, w.data());
}

void client_handler::handle_register_function(engine::IScriptEngine *eng, const std::vector<uint8_t> &body) {
  wire::Reader r(body.data(), body.size());
  std::string name = r.get_string();
  register_function_callback(eng, name, m_fd);
}

// Register a global function whose callback is dispatched to the given fd
// (the client connection or a mounted provider).
void client_handler::register_function_callback(engine::IScriptEngine *eng,
                                                const std::string &name, int fd) {
  eng->register_js_callback(
    name, [this, eng, name, fd](engine::JsContext ctx, const std::vector<variable> &jargs) -> engine::JsValue {
      ClientChannel channel(fd);
      variable resp = channel.call(name, jargs);
      return to_js_value(eng, ctx, resp);
    }
  );
}

engine::JsValue client_handler::to_js_value(engine::IScriptEngine *eng, engine::JsContext ctx,
                                            const variable &resp) {
  if (resp.is_ref()) {
    auto it = m_object_types.find(resp.as_ref().type);
    if (it == m_object_types.end())
      throw std::runtime_error("unknown object type: " + resp.as_ref().type);
    return eng->make_object(ctx, resp.as_ref().this_id, it->second.methods,
                            it->second.properties);
  }
  if (resp.is_array()) {
    bool has_refs = false;
    for (const auto &e : resp.as_array()) {
      if (e.is_ref()) {
        has_refs = true;
        break;
      }
    }
    if (has_refs) {
      std::vector<engine::JsValue> items;
      items.reserve(resp.as_array().size());
      for (const auto &e : resp.as_array()) {
        if (e.is_ref()) {
          auto it = m_object_types.find(e.as_ref().type);
          if (it == m_object_types.end())
            throw std::runtime_error("unknown object type: " + e.as_ref().type);
          items.push_back(eng->make_object(ctx, e.as_ref().this_id, it->second.methods,
                                           it->second.properties));
        } else {
          items.push_back(eng->make_value(ctx, e));
        }
      }
      return eng->make_array(ctx, items.size(), items.empty() ? nullptr : items.data());
    }
  }
  return eng->make_value(ctx, resp);
}

void client_handler::handle_register_object(
  engine::IScriptEngine *eng, const std::vector<uint8_t> &body
) {
  wire::Reader r(body.data(), body.size());
  std::string name = r.get_string();
  int32_t this_id = r.get_i32();
  std::string type_name = r.get_string();
  uint32_t count = r.get_u32();
  std::vector<std::string> methods;
  methods.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    methods.push_back(r.get_string());

  std::vector<engine::object_property> properties =
    parse_properties(r, "register_object(" + name + ")");

  register_method_callbacks(eng, m_fd, methods);

  if (!type_name.empty())
    m_object_types[type_name] = object_type_info{methods, properties};

  eng->register_global_object(name, this_id, methods, properties);
}

void client_handler::handle_register_object_type(
  engine::IScriptEngine *eng, const std::vector<uint8_t> &body
) {
  wire::Reader r(body.data(), body.size());
  std::string name = r.get_string();
  uint32_t count = r.get_u32();
  std::vector<std::string> methods;
  methods.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    methods.push_back(r.get_string());

  std::vector<engine::object_property> properties =
    parse_properties(r, "register_type(" + name + ")");

  m_object_types[name] = object_type_info{methods, properties};
  register_method_callbacks(eng, m_fd, methods);

  // 'new name(...)' in scripts: dispatch a constructor callback to the
  // client (method = class name); the client creates an instance and
  // answers with a ref, which becomes the constructed proxy object.
  eng->register_constructor(name, [this, eng, name](engine::JsContext ctx,
                                                    const std::vector<variable> &jargs) -> engine::JsValue {
    ClientChannel channel(m_fd);
    variable resp = channel.call(name, jargs);
    return to_js_value(eng, ctx, resp);
  });
}

void client_handler::register_method_callbacks(
  engine::IScriptEngine *eng, int fd, const std::vector<std::string> &methods
) {
  for (const auto &method : methods) {
    eng->register_callback(method, [fd](const std::string &cb_method, const std::vector<variable> &args) {
      ClientChannel channel(fd);
      return channel.call(cb_method, args);
    });
  }
}


void client_handler::handle_provider_mount(engine::IScriptEngine *eng,
                                            const std::vector<uint8_t> &body, int fd) {
  wire::Reader r(body.data(), body.size());
  std::string host = r.get_string();
  uint16_t port = r.get_u16();
  uint32_t name_count = r.get_u32();
  std::vector<std::string> names;
  names.reserve(name_count);
  for (uint32_t i = 0; i < name_count; ++i)
    names.push_back(r.get_string());

  auto fail = [&](const std::string &error) {
    wire::Writer w;
    w.put_u8(0);
    w.put_string(error);
    s5run::write_message(fd, protocol_msg_type::M_PROVIDER_MOUNT_RESP, w.data());
  };

  // connect to the provider (host name or numeric IP)
  addrinfo hints{};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *resolved = nullptr;
  if (::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &resolved) != 0
      || resolved == nullptr) {
    fail("provider mount: cannot resolve " + host);
    return;
  }
  int pfd = -1;
  for (addrinfo *ai = resolved; ai != nullptr; ai = ai->ai_next) {
    pfd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (pfd < 0)
      continue;
    if (::connect(pfd, ai->ai_addr, ai->ai_addrlen) == 0)
      break;
    ::close(pfd);
    pfd = -1;
  }
  ::freeaddrinfo(resolved);
  if (pfd < 0) {
    fail("provider mount: connect failed to " + host + ":" + std::to_string(port));
    return;
  }
  s5run::set_recv_timeout(pfd, 10'000);
  int nodelay = 1;
  ::setsockopt(pfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

  // provider hello: "s5run <version> <protocol>"
  {
    uint8_t hello_type = 0;
    std::vector<uint8_t> hello_body;
    if (!s5run::read_message(pfd, hello_type, hello_body)
        || hello_type != protocol_msg_type::M_SERVER_HELLO) {
      ::close(pfd);
      fail("provider mount: expected provider hello");
      return;
    }
    std::string text(hello_body.begin(), hello_body.end());
    if (text.compare(0, 6, "s5run ") != 0) {
      ::close(pfd);
      fail("provider mount: bad provider hello: " + text);
      return;
    }
  }

  // fetch and mount the metadata for every requested name
  for (const auto &name : names) {
    wire::Writer w;
    w.put_string(name);
    if (!s5run::write_message(pfd, protocol_msg_type::M_PROVIDER_META_REQ, w.data())) {
      ::close(pfd);
      fail("provider mount: meta request failed for '" + name + "'");
      return;
    }
    uint8_t resp_type = 0;
    std::vector<uint8_t> resp;
    if (!s5run::read_message(pfd, resp_type, resp)
        || resp_type != protocol_msg_type::M_PROVIDER_META_RESP) {
      ::close(pfd);
      fail("provider mount: no metadata for '" + name + "'");
      return;
    }
    wire::Reader mr(resp.data(), resp.size());
    uint8_t count = mr.get_u8();
    if (count == 0) {
      ::close(pfd);
      fail("provider mount: not found: " + name);
      return;
    }

    uint8_t kind = mr.get_u8();
    std::string entry_name = mr.get_string();
    if (kind == 1) {
      // function: skip the signature (return type + args)
      (void)mr.get_string();
      uint32_t argc = mr.get_u32();
      for (uint32_t j = 0; j < argc; ++j) {
        (void)mr.get_string();
        (void)mr.get_string();
      }
      register_function_callback(eng, entry_name, pfd);
    } else {
      // class (2) or global object (3)
      std::vector<std::string> methods;
      uint32_t method_count = mr.get_u32();
      methods.reserve(method_count);
      for (uint32_t j = 0; j < method_count; ++j) {
        methods.push_back(mr.get_string()); // method name
        (void)mr.get_string();              // return type
        uint32_t argc = mr.get_u32();
        for (uint32_t k = 0; k < argc; ++k) {
          (void)mr.get_string();
          (void)mr.get_string();
        }
      }
      std::vector<engine::object_property> properties;
      uint32_t prop_count = mr.get_u32();
      properties.reserve(prop_count);
      for (uint32_t j = 0; j < prop_count; ++j) {
        engine::object_property p;
        p.name = mr.get_string();
        p.getter = mr.get_string();
        p.setter = mr.get_string();
        properties.push_back(p);
      }
      int32_t this_id = 0;
      if (kind == 3)
        this_id = mr.get_i32();

      register_method_callbacks(eng, pfd, methods);
      m_object_types[entry_name] = object_type_info{methods, properties};
      if (kind == 3)
        eng->register_global_object(entry_name, this_id, methods, properties);
    }
  }

  m_provider_fds.push_back(pfd);

  wire::Writer w;
  w.put_u8(1);
  w.put_string("");
  s5run::write_message(fd, protocol_msg_type::M_PROVIDER_MOUNT_RESP, w.data());
}

void client_handler::handle_exec(engine::IScriptEngine *eng, const std::vector<uint8_t> &body, int fd) {
  wire::Reader r(body.data(), body.size());
  std::string script = r.get_string();
  std::string entry = r.get_string();
  uint32_t count = r.get_u32();
  std::vector<variable> args;
  args.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    args.push_back(value_from_wire(r));

  engine::ExecResult result = eng->execute(script, entry, args);
  if (!result.ok) {
    send_exec_error(fd, result.error);
    return;
  }

  wire::Writer w;
  w.put_u8(1);
  value_to_wire(result.value, w);
  s5run::write_message(fd, protocol_msg_type::M_EXEC_RESP, w.data());
}

void client_handler::send_exec_error(int fd, std::string error) {
  wire::Writer w;
  w.put_u8(0);
  w.put_string(error);
  s5run::write_message(fd, protocol_msg_type::M_EXEC_RESP, w.data());
}

void client_handler::send_register_resp(int fd, bool ok, std::string error) {
  wire::Writer w;
  w.put_u8(ok ? 1 : 0);
  w.put_string(error);
  s5run::write_message(fd, protocol_msg_type::M_REGISTER_RESP, w.data());
}

server::server() {
}

// static
void server::on_signal(int) {
  g_stop = true;
  if (g_listen_fd >= 0)
    ::shutdown(g_listen_fd, SHUT_RDWR);
}

void server::enable_queue(const std::string &db_path, int ttl_seconds) {
  m_queue_enabled = true;
  m_queue_db = db_path;
  m_queue_ttl = ttl_seconds;
}

int server::run(const std::string &listen_address, int threads) {

  std::string host = "0.0.0.0";
  std::string port_str = listen_address;
  size_t colon = listen_address.rfind(':');
  if (colon != std::string::npos) {
    host = listen_address.substr(0, colon);
    port_str = listen_address.substr(colon + 1);
  }
  uint16_t listen_port = (uint16_t)atoi(port_str.c_str());
  if (listen_port == 0) {
    std::fprintf(stderr, "invalid listen address: %s\n", listen_address.c_str());
    return 1;
  }

  std::signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, &server::on_signal);
  std::signal(SIGTERM, &server::on_signal);

  g_listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (g_listen_fd < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(listen_port);
  if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
    // not a numeric IP - resolve the host name (e.g. localhost)
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo *resolved = nullptr;
    if (::getaddrinfo(host.c_str(), nullptr, &hints, &resolved) != 0 || resolved == nullptr) {
      std::fprintf(stderr, "invalid listen host: %s\n", host.c_str());
      return 1;
    }
    std::memcpy(&addr.sin_addr,
                &((sockaddr_in *)resolved->ai_addr)->sin_addr,
                sizeof(addr.sin_addr));
    ::freeaddrinfo(resolved);
  }

  if (::bind(g_listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (::listen(g_listen_fd, 128) < 0) {
    perror("listen");
    return 1;
  }

  std::printf("s5run listening on %s:%u, threads=%d\n", host.c_str(), listen_port, threads);

  thread_pool pool(threads);

  std::unique_ptr<job_queue> queue;
  std::thread queue_worker;
  if (m_queue_enabled) {
    try {
      queue.reset(new job_queue(m_queue_db, m_queue_ttl));
    } catch (const std::exception &e) {
      std::fprintf(stderr, "queue: %s\n", e.what());
      return 1;
    }
    g_queue = queue.get();
    queue_worker = std::thread([&queue]() {
      int cleanup_counter = 0;
      while (!g_stop) {
        job_queue::job_info job;
        if (!queue->claim_next(job)) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }
        std::vector<variable> args;
        try {
          wire::Reader r(job.args.data(), job.args.size());
          uint32_t count = r.get_u32();
          args.reserve(count);
          for (uint32_t i = 0; i < count; ++i)
            args.push_back(value_from_wire(r));
        } catch (const std::exception &e) {
          queue->store_result(job.id, false, {}, std::string("bad args: ") + e.what());
          continue;
        }
        try {
          auto eng = engine::make_quickjs_engine();
          engine::ExecResult res = eng->execute(job.script, job.entry, args);
          if (res.ok) {
            wire::Writer w;
            value_to_wire(res.value, w);
            queue->store_result(job.id, true, w.data(), "");
          } else {
            queue->store_result(job.id, false, {}, res.error);
          }
        } catch (const std::exception &e) {
          queue->store_result(job.id, false, {}, e.what());
        }
        if (++cleanup_counter >= 100) {
          cleanup_counter = 0;
          queue->cleanup();
        }
      }
    });
    std::printf("queue enabled: db=%s ttl=%ds\n", m_queue_db.c_str(), m_queue_ttl);
  }

  while (!g_stop) {
    sockaddr_in caddr{};
    socklen_t clen = sizeof(caddr);
    int cfd = ::accept(g_listen_fd, (sockaddr *)&caddr, &clen);
    if (cfd >= 0) {
      // request-response protocol: disable Nagle so the response body
      // is not held back waiting for the header ACK (40ms delayed-ack)
      int nodelay = 1;
      ::setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    }
    if (cfd < 0) {
      if (g_stop)
        break;
      perror("accept");
      continue;
    }
    char ip[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
    std::printf("client %s:%u connected\n", ip, ntohs(caddr.sin_port));

    pool.submit(std::make_shared<client_handler>(cfd, &pool));
  }

  if (g_listen_fd >= 0)
    ::close(g_listen_fd);
  g_queue = nullptr;
  if (queue_worker.joinable())
    queue_worker.join();
  std::printf("server stopped\n");
  return 0;
}

} // namespace s5run
