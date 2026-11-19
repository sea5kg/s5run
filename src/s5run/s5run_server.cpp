#include "internal/s5run_globals.h"

#include "json_mini.h"
#include "s5run_protocol.h"
#include "script_engine.h"
#include "value_wire.h"
#include "internal/s5run_thread_pool.h"
#include "s5run_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace s5run {

std::atomic<bool> g_stop{false};

static int g_listen_fd = -1;


namespace {

// Synchronous callback channel: sends a request to the client and waits
// for the response. Assumes one in-flight message per direction.
class ClientChannel {
public:
  explicit ClientChannel(int fd) : fd_(fd) {
  }

  Value call(const std::string &method, const std::vector<Value> &args) {
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

Value json_to_value(const json::Value &v) {
  if (v.is_null())
    return Value();
  if (v.is_bool())
    return Value(v.as_bool());
  if (v.is_num()) {
    double d = v.as_num();
    if (d == (double)(int64_t)d)
      return Value((int64_t)d);
    return Value(d);
  }
  if (v.is_str())
    return Value(v.as_str());
  if (v.is_arr()) {
    ValueArray out;
    out.reserve(v.as_arr().size());
    for (const auto &e : v.as_arr())
      out.push_back(json_to_value(e));
    return Value(out);
  }
  const auto &o = v.as_obj();
  if (o.size() == 1 && o.count("this_id") && o.at("this_id").is_num())
    return Value(ObjectRef((int)o.at("this_id").as_num(), ""));
  ValueObject out;
  for (const auto &kv : o)
    out[kv.first] = json_to_value(kv.second);
  return Value(out);
}

json::Value value_to_json(const Value &v) {
  switch (v.kind()) {
  case Value::K_NULL:
    return json::Value();
  case Value::K_BOOL:
    return json::Value(v.as_bool());
  case Value::K_INT:
    return json::Value((double)v.as_int());
  case Value::K_DOUBLE:
    return json::Value(v.as_double());
  case Value::K_STRING:
    return json::Value(v.as_string());
  case Value::K_ARRAY: {
    json::Array out;
    out.reserve(v.as_array().size());
    for (const auto &e : v.as_array())
      out.push_back(value_to_json(e));
    return json::Value(std::move(out));
  }
  case Value::K_OBJECT: {
    json::Object out;
    for (const auto &kv : v.as_object())
      out[kv.first] = value_to_json(kv.second);
    return json::Value(std::move(out));
  }
  case Value::K_REF: {
    json::Object out;
    out["this_id"] = json::Value((double)v.as_ref().this_id);
    return json::Value(std::move(out));
  }
  }
  return json::Value();
}

std::vector<Value> json_args_to_values(const json::Array &args) {
  std::vector<Value> out;
  out.reserve(args.size());
  for (const auto &a : args)
    out.push_back(json_to_value(a));
  return out;
}

} // namespace


class client_handler : public thread_pool_task {
public:
  client_handler(int fd);
  virtual void handle() override;
private:
  int m_fd;
  std::map<std::string, json::Array> m_object_types;
  void handle_register_function(engine::IScriptEngine *, const std::vector<uint8_t> &body);
  void handle_register_object(engine::IScriptEngine *, ClientChannel &channel, const std::vector<uint8_t> &body);
  void handle_register_object_type(engine::IScriptEngine *, ClientChannel &channel, const std::vector<uint8_t> &body);
  void handle_exec(engine::IScriptEngine *, const std::vector<uint8_t> &body, int fd);
  void send_exec_error(int fd, std::string);
  void send_register_resp(int fd, bool, std::string);
  void register_method_callbacks(engine::IScriptEngine *, ClientChannel &, const json::Array &methods);
};

client_handler::client_handler(int fd) : m_fd(fd) {

};

void client_handler::handle() {
  s5run::set_recv_timeout(m_fd, 10'000);

  std::shared_ptr<engine::IScriptEngine> eng;
  try {
    eng = engine::make_quickjs_engine();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "engine init: %s\n", e.what());
    ::close(m_fd);
    return;
  }

  ClientChannel channel(m_fd);

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
        handle_register_object(eng.get(), channel, body);
        send_register_resp(m_fd, true, "");
        break;
      case protocol_msg_type::M_REGISTER_TYPE:
        handle_register_object_type(eng.get(), channel, body);
        send_register_resp(m_fd, true, "");
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
  ::close(m_fd);
}

void client_handler::handle_register_function(engine::IScriptEngine *eng, const std::vector<uint8_t> &body) {
  wire::Reader r(body.data(), body.size());
  std::string name = r.get_string();

  eng->register_js_callback(
      name, [this, eng, name](engine::JsContext ctx, const json::Array &jargs) -> engine::JsValue {
        ClientChannel channel(m_fd);
        Value resp = channel.call(name, json_args_to_values(jargs));
        if (resp.is_ref()) {
          auto it = m_object_types.find(resp.as_ref().type);
          if (it == m_object_types.end())
            throw std::runtime_error("unknown object type: " + resp.as_ref().type);
          return eng->make_object(ctx, resp.as_ref().this_id, it->second);
        }
        return eng->make_value(ctx, value_to_json(resp));
      });
}

void client_handler::handle_register_object(engine::IScriptEngine *eng, ClientChannel &channel, const std::vector<uint8_t> &body) {
  wire::Reader r(body.data(), body.size());
  std::string name = r.get_string();
  int32_t this_id = r.get_i32();
  std::string type_name = r.get_string();
  uint32_t count = r.get_u32();
  json::Array methods;
  methods.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    methods.push_back(json::Value(r.get_string()));

  register_method_callbacks(eng, channel, methods);

  if (!type_name.empty())
    m_object_types[type_name] = methods;

  eng->register_js_callback(
      name, [eng, this_id, methods](engine::JsContext ctx, const json::Array &) -> engine::JsValue {
        return eng->make_object(ctx, this_id, methods);
      });
}

void client_handler::handle_register_object_type(engine::IScriptEngine *eng, ClientChannel &channel, const std::vector<uint8_t> &body) {
  wire::Reader r(body.data(), body.size());
  std::string name = r.get_string();
  uint32_t count = r.get_u32();
  json::Array methods;
  methods.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    methods.push_back(json::Value(r.get_string()));

  m_object_types[name] = methods;
  register_method_callbacks(eng, channel, methods);
}

void client_handler::register_method_callbacks(engine::IScriptEngine *eng, ClientChannel &channel, const json::Array &methods) {
  for (const auto &m : methods) {
    if (!m.is_str())
      continue;
    std::string method = m.as_str();
    eng->register_callback(
        method, [&channel](const std::string &cb_method, const json::Array &args) {
          return value_to_json(channel.call(cb_method, json_args_to_values(args)));
        });
  }
}

void client_handler::handle_exec(engine::IScriptEngine *eng, const std::vector<uint8_t> &body, int fd) {
  wire::Reader r(body.data(), body.size());
  std::string script = r.get_string();
  std::string entry = r.get_string();
  uint32_t count = r.get_u32();
  json::Array jargs;
  jargs.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
    jargs.push_back(value_to_json(value_from_wire(r)));

  engine::ExecResult result = eng->execute(script, entry, jargs);
  if (!result.ok) {
    send_exec_error(fd, result.error);
    return;
  }

  wire::Writer w;
  w.put_u8(1);
  value_to_wire(json_to_value(result.value), w);
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
    std::fprintf(stderr, "invalid listen host: %s\n", host.c_str());
    return 1;
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

  while (!g_stop) {
    sockaddr_in caddr{};
    socklen_t clen = sizeof(caddr);
    int cfd = ::accept(g_listen_fd, (sockaddr *)&caddr, &clen);
    if (cfd < 0) {
      if (g_stop)
        break;
      perror("accept");
      continue;
    }
    char ip[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
    std::printf("client %s:%u connected\n", ip, ntohs(caddr.sin_port));

    pool.submit(std::make_shared<client_handler>(cfd));
  }

  if (g_listen_fd >= 0)
    ::close(g_listen_fd);
  std::printf("server stopped\n");
  return 0;
}

} // namespace s5run
