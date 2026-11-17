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

  while (true) {
    uint8_t msg_type;
    std::vector<uint8_t> body;
    if (!proto::read_message(fd, msg_type, body))
      break;

    try {
      switch (msg_type) {
      case MsgType::M_REGISTER_FUNC:
        handle_register_function(eng.get(), body);
        send_register_resp(fd, true, "");
        break;
      case MsgType::M_REGISTER_OBJ:
        handle_register_object(eng.get(), channel, body);
        send_register_resp(fd, true, "");
        break;
      case MsgType::M_REGISTER_TYPE:
        handle_register_object_type(eng.get(), channel, body);
        send_register_resp(fd, true, "");
        break;
      case MsgType::M_EXEC_REQ:
        handle_exec(eng.get(), body, fd);
        break;
      default:
        throw std::runtime_error("unknown message type");
      }
    } catch (const std::exception &e) {
      if (msg_type == MsgType::M_EXEC_REQ) {
        send_exec_error(fd, e.what());
      } else {
        send_register_resp(fd, false, e.what());
      }
    }
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