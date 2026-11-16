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
#include <ctime>

#include "example/run_script_client.h"

class MyFile : public example::RunScriptClientObject {
public:
  explicit MyFile(std::string path)
      : example::RunScriptClientObject("File"),
        path_(std::move(path)) {}

  std::vector<std::string> methods() const override {
    return {"GetSize", "ReadBytes"};
  }

  virtual example::Value execute(example::IRunScriptClient &, const std::string &method,
                         const std::vector<example::Value> &args) override {
    if (method == "GetSize") {
      return example::Value(static_cast<int64_t>(data_.size()));
    }
    if (method == "ReadBytes") {
      int64_t n = args.at(0).as_int();
      example::ValueArray out;
      for (int64_t i = 0; i < n && i < (int64_t)data_.size(); ++i) {
        out.push_back(example::Value(static_cast<int64_t>(data_[i])));
      }
      return example::Value(out);
    }
    throw std::runtime_error("MyFile: unknown method: " + method);
  }

private:
  std::string path_;
  std::vector<uint8_t> data_ = std::vector<uint8_t>(1024, 0);
};


class MyFileSystemImpl : public example::RunScriptClientGlobalObject {
public:
  MyFileSystemImpl() : example::RunScriptClientGlobalObject("FileSystem") {}

  std::vector<std::string> methods() const override {
    return {"GetFile", "HasFile"};
  }

  example::Value execute(example::IRunScriptClient &, const std::string &method,
                         const std::vector<example::Value> &args) override {
    // if (method == "GetFile") return get_file(args.at(0).as_string());
    if (method == "HasFile") return example::Value(true);
    throw std::runtime_error("unknown method: " + method);
  }

private:
  // example::Value get_file(const std::string &path) { /* ... */ }
};

class MyGetFile : public example::RunScriptClientGlobalFunction {
public:
  MyGetFile() : example::RunScriptClientGlobalFunction("GetFile") {
  }

  example::Value execute(example::IRunScriptClient &client, const std::vector<example::Value> &args) override {
    std::string path = args.at(0).as_string();
    std::shared_ptr<example::RunScriptClientObject> file(new MyFile(path));
    int id = client.register_object(file);
    return example::Value(example::ObjectRef(id, "File"));
  }
};

class MyGetTimeNow : public example::RunScriptClientGlobalFunction {
public:
  MyGetTimeNow() : example::RunScriptClientGlobalFunction("GetTimeNow") {
  }

  example::Value execute(example::IRunScriptClient &, const std::vector<example::Value> &) override {
    return example::Value(static_cast<int64_t>(std::time(NULL)));
  }
};

// class MyFileFactory : public example::RunScriptClientObjectType {
// public:
//   MyFileFactory() : example::RunScriptClientObjectType("File") {}
//   std::shared_ptr<example::RunScriptClientObject> create(
//       example::IRunScriptClient &,
//       const std::vector<example::Value> &args) override {
//     return std::shared_ptr<example::RunScriptClientObject>(
//         new MyFile(args.empty() ? "" : args.at(0).as_string()));
//   }
// };

int main(int argc, char **argv) {
  // std::string host = "localhost";
  // auto run_script = example::createRunScriptClient("host", port);
  // run_script->add_global_function();

  // run_script.run("script content", "main", [])

  return 0;
}

/*


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
  */