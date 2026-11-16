#include "json_mini.h"
#include "protocol.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace {

// Client-side emulation of the object store.
struct FileObject {
  int64_t id;
  double size;
};

int64_t g_next_id = 100;

json::Value handle_callback(const std::string &method, const json::Array &args) {
  if (method == "getFile") {
    std::string name = args.empty() ? "" : args[0].as_str();
    // Pretend the client resolved the name to a file object.
    FileObject f{++g_next_id, 1024.0};

    json::Object o;
    o["type"] = json::Value(std::string("File"));
    o["this"] = json::Value((double)f.id);
    o["methods"] = json::Value(json::Array{json::Value(std::string("size"))});
    return json::Value(std::move(o));
  }
  if (method == "size") {
    // First arg is this_id.
    if (args.empty())
      throw std::runtime_error("size: missing this_id");
    int64_t this_id = (int64_t)args[0].as_num();
    // In a real client we would look up the object by id.
    return json::Value(1024.0);
  }
  throw std::runtime_error("unknown callback: " + method);
}

} // namespace

int main(int argc, char **argv) {
  const char *host = argc > 1 ? argv[1] : "127.0.0.1";
  uint16_t port = argc > 2 ? (uint16_t)std::atoi(argv[2]) : 7777;

  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  sockaddr_in a{};
  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  inet_pton(AF_INET, host, &a.sin_addr);
  if (::connect(fd, (sockaddr *)&a, sizeof(a)) < 0) {
    perror("connect");
    return 1;
  }

  std::string script = R"(
        function main(name) {
            var file = getFile(name);
            var sz = file.size();
            return { name: name, size: sz };
        }
    )";

  json::Object req;
  req["script"] = json::Value(script);
  req["entry"] = json::Value(std::string("main"));
  req["args"] = json::Value(json::Array{json::Value(std::string("test-client."))});

  if (!proto::write_frame(fd, json::dump(json::Value(req)))) {
    std::fprintf(stderr, "write failed\n");
    return 1;
  }

  while (true) {
    auto frame = proto::read_frame(fd);
    if (!frame)
      break;
    json::Value msg = json::parse(*frame);
    const auto &o = msg.as_obj();
    std::string type = o.count("type") ? o.at("type").as_str() : "exec";

    if (type == "callback") {
      int id = (int)o.at("id").as_num();
      std::string method = o.at("method").as_str();
      json::Array args;
      if (o.count("args") && o.at("args").is_arr())
        args = o.at("args").as_arr();

      json::Object resp;
      resp["type"] = json::Value(std::string("callback_result"));
      resp["id"] = json::Value((double)id);
      try {
        resp["ok"] = json::Value(true);
        resp["value"] = handle_callback(method, args);
      } catch (const std::exception &e) {
        resp["ok"] = json::Value(false);
        resp["error"] = json::Value(std::string(e.what()));
      }
      if (!proto::write_frame(fd, json::dump(json::Value(resp))))
        break;
    } else {
      std::printf("result: %s\n", frame->c_str());
      break;
    }
  }
  ::close(fd);
  return 0;
}