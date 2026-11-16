#include "json_mini.h"
#include "protocol.h"
#include "script_engine.h"
#include "thread_pool.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

static std::atomic<bool> g_stop{false};
static int g_listen_fd = -1;

static void on_signal(int) {
  g_stop = true;
  if (g_listen_fd >= 0)
    ::shutdown(g_listen_fd, SHUT_RDWR);
}

namespace {

// Synchronous callback channel: sends a request to the client and waits
// for the response. Assumes one in-flight message per direction.
class ClientChannel {
public:
  explicit ClientChannel(int fd) : fd_(fd) {
  }

  json::Value call(const std::string &method, const json::Array &args) {
    int id = ++last_id_;
    json::Object req;
    req["type"] = json::Value(std::string("callback"));
    req["id"] = json::Value((double)id);
    req["method"] = json::Value(method);
    req["args"] = json::Value(args);

    if (!proto::write_frame(fd_, json::dump(json::Value(req))))
      throw std::runtime_error("callback: write failed");

    auto resp_frame = proto::read_frame(fd_);
    if (!resp_frame)
      throw std::runtime_error("callback: no response (timeout/eof)");

    json::Value resp = json::parse(*resp_frame);
    if (!resp.is_obj())
      throw std::runtime_error("callback: response is not an object");
    const auto &o = resp.as_obj();

    if (!o.count("type") || o.at("type").as_str() != "callback_result")
      throw std::runtime_error("callback: unexpected message type");
    if (o.count("id") && (int)o.at("id").as_num() != id)
      throw std::runtime_error("callback: id mismatch");

    if (o.count("ok") && o.at("ok").is_bool() && !o.at("ok").as_bool()) {
      std::string err = o.count("error") ? o.at("error").as_str() : "unknown error";
      throw std::runtime_error("callback '" + method + "': " + err);
    }
    return o.count("value") ? o.at("value") : json::Value();
  }

private:
  int fd_;
  int last_id_ = 0;
};

} // namespace

static void handle_client(int fd) {
  // Timeout for all reads. Callbacks block the worker, so a stuck client
  // must not hold the thread forever.
  proto::set_recv_timeout(fd, 10'000);

  std::unique_ptr<engine::IScriptEngine> eng;
  try {
    eng = engine::make_quickjs_engine();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "engine init: %s\n", e.what());
    ::close(fd);
    return;
  }

  ClientChannel channel(fd);

  // getFile: asks the client for a file descriptor and builds a JS object
  // with method stubs. The client responds with, e.g.:
  //   {"type":"File","this":111,"methods":["size"]}
  eng->register_js_callback(
    "getFile", [&channel, eng_raw = eng.get()](engine::JsContext ctx, const json::Array &args) -> engine::JsValue {
      json::Value resp = channel.call("getFile", args);
      if (!resp.is_obj())
        throw std::runtime_error("getFile: bad response");
      const auto &o = resp.as_obj();
      if (!o.count("this") || !o.count("methods"))
        throw std::runtime_error("getFile: missing fields");
      int64_t this_id = (int64_t)o.at("this").as_num();
      return eng_raw->make_object(ctx, this_id, o.at("methods").as_arr());
    }
  );

  // size: simple callback. First arg is this_id, rest are JS args.
  eng->register_callback("size", [&channel](const std::string &method, const json::Array &args) {
    return channel.call(method, args);
  });

  while (true) {
    auto frame = proto::read_frame(fd);
    if (!frame)
      break;

    json::Value resp;
    try {
      json::Value req = json::parse(*frame);
      if (!req.is_obj())
        throw std::runtime_error("request must be a JSON object");

      const auto &o = req.as_obj();
      if (!o.count("script") || !o.at("script").is_str())
        throw std::runtime_error("missing 'script' (string)");
      if (!o.count("entry") || !o.at("entry").is_str())
        throw std::runtime_error("missing 'entry' (string)");

      json::Array args;
      if (o.count("args")) {
        if (!o.at("args").is_arr())
          throw std::runtime_error("'args' must be an array");
        args = o.at("args").as_arr();
      }

      auto result = eng->execute(o.at("script").as_str(), o.at("entry").as_str(), args);

      json::Object r;
      r["ok"] = json::Value(result.ok);
      r["value"] = result.value;
      if (!result.ok)
        r["error"] = json::Value(result.error);
      resp = json::Value(std::move(r));
    } catch (const std::exception &e) {
      json::Object r;
      r["ok"] = json::Value(false);
      r["error"] = json::Value(std::string("protocol: ") + e.what());
      resp = json::Value(std::move(r));
    }

    if (!proto::write_frame(fd, json::dump(resp)))
      break;
  }
  ::close(fd);
}

int main(int argc, char **argv) {
  uint16_t port = 7777;
  size_t threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 4;
  if (argc > 1)
    port = static_cast<uint16_t>(std::atoi(argv[1]));
  if (argc > 2)
    threads = static_cast<size_t>(std::atoi(argv[2]));

  std::signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  g_listen_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (g_listen_fd < 0) {
    perror("socket");
    return 1;
  }

  int opt = 1;
  setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (::bind(g_listen_fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (::listen(g_listen_fd, 128) < 0) {
    perror("listen");
    return 1;
  }

  std::printf("sea5kg-run-scripts-server listening on :%u, threads=%zu\n", port, threads);

  ThreadPool pool(threads);

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

    pool.submit([cfd] {
      handle_client(cfd);
    });
  }

  if (g_listen_fd >= 0)
    ::close(g_listen_fd);
  std::printf("server stopped\n");
  return 0;
}