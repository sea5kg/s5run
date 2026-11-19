// Smoke test client built on the GENERATED s5run client API.
//
// Usage: ./cpp11-client [host] [port]
#include "s5run_cli_example/s5run_cli_example.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

class MyFile : public s5run_cli_example::File {
public:
  explicit MyFile(int64_t size = 1024)
    : m_size(size) {
  }

  int64_t GetSize() override {
    return m_size;
  }

private:
  int64_t m_size;
public:

  std::vector<uint8_t> ReadBytes(int64_t readBytes) override {
    std::vector<uint8_t> out;
    for (int64_t i = 0; i < readBytes; ++i)
      out.push_back((uint8_t)('a' + i % 26));
    return out;
  }
};

class MyGetFile : public s5run_cli_example::GlobalFunction_GetFile {
public:
  std::shared_ptr<s5run_cli_example::File> execute(const std::string &filepath) override {
    m_last_path = filepath;
    return std::shared_ptr<s5run_cli_example::File>(new MyFile());
  }

  static std::string m_last_path;
};

std::string MyGetFile::m_last_path;

class MyGetFileSize : public s5run_cli_example::GlobalFunction_GetFileSize {
public:
  int64_t execute(const std::shared_ptr<s5run_cli_example::File> &file) override {
    return file->GetSize();
  }
};

class MyGetPi : public s5run_cli_example::GlobalFunction_GetPi {
public:
  double execute() override {
    return 3.14;
  }
};

class MyGetServerPort : public s5run_cli_example::GlobalFunction_GetServerPort {
public:
  uint64_t execute() override {
    return 8080;
  }
};

class MyFileSystem : public s5run_cli_example::FileSystem {
public:
  std::vector<std::string> GetList(const std::string &filepath) override {
    m_last_path = filepath;
    return {"a.txt", "b.txt", "c.txt"};
  }

  static std::string m_last_path;
};

std::string MyFileSystem::m_last_path;

class MyGetOpenedFiles : public s5run_cli_example::GlobalFunction_GetOpenedFiles {
public:
  std::vector<std::shared_ptr<s5run_cli_example::File>> execute() override {
    std::vector<std::shared_ptr<s5run_cli_example::File>> out;
    out.push_back(std::make_shared<MyFile>(100));
    out.push_back(std::make_shared<MyFile>(200));
    return out;
  }
};

class MyCloseFiles : public s5run_cli_example::GlobalFunction_CloseFiles {
public:
  void execute(const std::vector<std::shared_ptr<s5run_cli_example::File>> &files) override {
    for (size_t i = 0; i < files.size(); ++i)
      m_closed_sizes.push_back(files[i]->GetSize());
  }

  static std::vector<int64_t> m_closed_sizes;
};

std::vector<int64_t> MyCloseFiles::m_closed_sizes;

class MyGetTimeNow : public s5run_cli_example::GlobalFunction_GetTimeNow {
public:
  int64_t execute() override {
    return 1737000000;
  }
};

class MyHasFile : public s5run_cli_example::GlobalFunction_HasFile {
public:
  bool execute(const std::string &filepath) override {
    throw std::runtime_error("boom-hasfile");
  }
};

class MyConsole : public s5run_cli_example::console {
public:
  virtual void log(const std::vector<std::string> &args) override;
  virtual void warning(const std::vector<std::string> &args) override;
  virtual void error(const std::vector<std::string> &args) override;
  std::vector<std::string> get_logs();

private:
  std::string to_line(const std::vector<std::string> &args);

  std::vector<std::string> m_logs;
  std::vector<std::string> m_warnings;
  std::vector<std::string> m_errors;
};

void MyConsole::log(const std::vector<std::string> &args) {
  m_logs.push_back(to_line(args));
}

void MyConsole::warning(const std::vector<std::string> &args) {
  m_warnings.push_back(to_line(args));
}

void MyConsole::error(const std::vector<std::string> &args) {
  m_errors.push_back(to_line(args));
}

std::vector<std::string> MyConsole::get_logs() {
  return m_logs;
}

std::string MyConsole::to_line(const std::vector<std::string> &args) {
  std::string line;
  for (size_t i = 0; i < args.size(); ++i)
    line += (line.empty() ? "" : " ") + args[i];
  return line;
}

static const char *kScript = R"(
function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    var b = f.ReadBytes(3);
    console.log("hello", path, 1, 2);
    var size_by_ref = GetFileSize(f);
    var opened = GetOpenedFiles();
    var opened_sum = 0;
    for (var i = 0; i < opened.length; ++i)
        opened_sum += opened[i].GetSize();
    CloseFiles(opened);
    var file_list = FileSystem.GetList(path);
    return {
        file_list_count: file_list.length,
        size: f.GetSize(),
        size_by_ref: size_by_ref,
        opened_count: opened.length,
        opened_sum: opened_sum,
        time: t,
        bytes: b.length,
        first: b[0]
    };
}
)";

int main(int argc, char **argv) {
  std::string host = argc > 1 ? argv[1] : "127.0.0.1";
  uint16_t port = argc > 2 ? (uint16_t)atoi(argv[2]) : 7777;

  char address[64];
  snprintf(address, sizeof(address), "s5run://%s:%u", host.c_str(), (unsigned)port);

  // == happy path
  std::printf("== cpp11-client: happy path\n");
  if (std::string(s5run_cli_example::context::client_version()) != "1.0.0")
    throw std::runtime_error("unexpected client version: "
                             + std::string(s5run_cli_example::context::client_version()));
  {
    std::shared_ptr<MyConsole> con = std::make_shared<MyConsole>();

    std::shared_ptr<s5run_cli_example::context> ctx = s5run_cli_example::make_context();
    ctx->add_class(std::make_shared<MyFile>());
    ctx->add_function(std::make_shared<MyGetTimeNow>());
    ctx->add_function(std::make_shared<MyGetFileSize>());
    ctx->add_function(std::make_shared<MyGetOpenedFiles>());
    ctx->add_function(std::make_shared<MyCloseFiles>());
    ctx->add_function(std::make_shared<MyGetPi>());
    ctx->add_function(std::make_shared<MyGetServerPort>());
    ctx->add_object(std::make_shared<MyFileSystem>());
    ctx->add_function(std::make_shared<MyGetFile>());
    std::shared_ptr<s5run_cli_example::remote_server_info> server0 =
      std::make_shared<s5run_cli_example::remote_server_info>();
    server0->set_address(address);
    server0->set_api_key("some api key runner");
    ctx->add_server(server0);
    ctx->add_object(con);

    ctx->set_script(kScript);
    std::vector<s5run_cli_example::value> args;
    args.push_back(s5run_cli_example::value("test.txt"));
    std::shared_ptr<s5run_cli_example::run_result> result = s5run_cli_example::run_script(ctx, "main", args);
    if (result->has_error()) {
      throw std::runtime_error("unexpected error: " + result->error());
    }
    std::printf("  duration: %lld ms\n", (long long)result->duration());

    // checks

    const std::map<std::string, s5run_cli_example::value> &obj = result->get_value().as_object();
    std::printf(
      "  exec result: {size: %lld, time: %lld, bytes: %lld, first: %lld}\n",
      (long long)obj.at("size").as_int(),
      (long long)obj.at("time").as_int(),
      (long long)obj.at("bytes").as_int(),
      (long long)obj.at("first").as_int()
    );

    if (obj.at("size").as_int() != 1024)
      throw std::runtime_error("unexpected size");
    if (obj.at("size_by_ref").as_int() != 1024)
      throw std::runtime_error("unexpected size_by_ref");
    if (obj.at("opened_count").as_int() != 2)
      throw std::runtime_error("unexpected opened_count");
    if (obj.at("opened_sum").as_int() != 300)
      throw std::runtime_error("unexpected opened_sum");
    if (obj.at("file_list_count").as_int() != 3)
      throw std::runtime_error("unexpected file_list_count");
    if (MyFileSystem::m_last_path != "test.txt")
      throw std::runtime_error("unexpected FileSystem.GetList path: " + MyFileSystem::m_last_path);
    if (MyCloseFiles::m_closed_sizes.size() != 2
        || MyCloseFiles::m_closed_sizes[0] != 100
        || MyCloseFiles::m_closed_sizes[1] != 200)
      throw std::runtime_error("unexpected closed sizes");
    if (obj.at("time").as_int() != 1737000000)
      throw std::runtime_error("unexpected time");
    if (obj.at("bytes").as_int() != 3)
      throw std::runtime_error("unexpected bytes");
    if (obj.at("first").as_int() != 'a')
      throw std::runtime_error("unexpected first byte");
    if (MyGetFile::m_last_path != "test.txt")
      throw std::runtime_error("unexpected filepath: " + MyGetFile::m_last_path);
    if (con->get_logs().size() == 0) {
      throw std::runtime_error("unexpected console log is empty");
    } else {
      std::string log_message = con->get_logs()[0];
      if (log_message != "hello test.txt 1 2") {
        throw std::runtime_error("unexpected console log: " + log_message);
      }
      std::printf("  console.log got: \"%s\"\n", log_message);
    }
    std::printf("  PASS\n");
  }

  // == primitives (float/uint via separate run)
  {
    std::shared_ptr<s5run_cli_example::context> ctx = s5run_cli_example::make_context();
    ctx->add_function(std::make_shared<MyGetPi>());
    ctx->add_function(std::make_shared<MyGetServerPort>());
    std::shared_ptr<s5run_cli_example::remote_server_info> server0 =
      std::make_shared<s5run_cli_example::remote_server_info>();
    server0->set_address(address);
    server0->set_api_key("some api key runner");
    std::shared_ptr<s5run_cli_example::remote_server_info> server1 =
      std::make_shared<s5run_cli_example::remote_server_info>();
    server1->set_address(address);
    ctx->add_server(server0);
    ctx->add_server(server1); // same address: exercises the load-balancing probe
    ctx->set_script("function main() { return { pi: GetPi(), port: GetServerPort() }; }");
    std::shared_ptr<s5run_cli_example::run_result> result = s5run_cli_example::run_script(ctx, "main");
    if (result->has_error())
      throw std::runtime_error("unexpected error: " + result->error());
    const std::map<std::string, s5run_cli_example::value> &obj = result->get_value().as_object();
    std::printf(
      "== cpp11-client: primitives\n  pi: %f, port: %lld\n",
      obj.at("pi").as_double(),
      (long long)obj.at("port").as_int()
    );
    if (obj.at("pi").as_double() < 3.13 || obj.at("pi").as_double() > 3.15)
      throw std::runtime_error("unexpected pi");
    if (obj.at("port").as_int() != 8080)
      throw std::runtime_error("unexpected port");
    std::printf("  PASS\n");
  }

  // == add_context_provider stub
  std::printf("== cpp11-client: add_context_provider stub\n");
  {
    std::shared_ptr<s5run_cli_example::context> ctx = s5run_cli_example::make_context();
    std::vector<std::string> contexts;
    contexts.push_back("Storage");
    bool thrown = false;
    try {
      ctx->add_context_provider("s5run://localhost:7775", "some_api_key", contexts);
    } catch (const std::exception &e) {
      std::printf("  error: %s\n", e.what());
      thrown = std::string(e.what()).find("not implemented") != std::string::npos;
    }
    if (!thrown)
      throw std::runtime_error("expected 'not implemented yet' from add_context_provider");
    std::printf("  PASS\n");
  }

  // == error path
  std::printf("== cpp11-client: error path\n");
  {
    std::shared_ptr<s5run_cli_example::context> ctx = s5run_cli_example::make_context();
    ctx->add_function(std::make_shared<MyHasFile>());
    std::shared_ptr<s5run_cli_example::remote_server_info> server0 =
      std::make_shared<s5run_cli_example::remote_server_info>();
    server0->set_address(address);
    server0->set_api_key("some api key runner");
    ctx->add_server(server0);
    ctx->set_script("function main() {\n  return HasFile(\"x\");\n}");
    std::shared_ptr<s5run_cli_example::run_result> result = s5run_cli_example::run_script(ctx, "main");
    if (!result->has_error())
      throw std::runtime_error("expected 'boom-hasfile' error");
    std::printf("  exec error: %s\n", result->error().substr(0, result->error().find('\n')).c_str());
    std::printf("  error line: %d\n", result->error_line());
    if (result->error().find("boom-hasfile") == std::string::npos)
      throw std::runtime_error("expected 'boom-hasfile' in error");
    if (result->error_line() != 2)
      throw std::runtime_error("unexpected error line");
    std::printf("  PASS\n");
  }

  std::printf("cpp11-client: all smoke tests passed\n");
  return 0;
}
