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
  int64_t GetSize() override {
    return 1024;
  }

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
    return { size: f.GetSize(), time: t, bytes: b.length, first: b[0] };
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

    std::shared_ptr<s5run_cli_example::client> cli = s5run_cli_example::make_client();
    cli->add_class(std::make_shared<MyFile>());
    cli->add_function(std::make_shared<MyGetTimeNow>());
    cli->add_function(std::make_shared<MyGetFile>());
    cli->set_server_address(address);
    cli->add_object(con);

    std::vector<s5run_cli_example::value> args;
    args.push_back(s5run_cli_example::value("test.txt"));
    s5run_cli_example::value result = cli->run(kScript, "main", args);


    // checks

    const std::map<std::string, s5run_cli_example::value> &obj = result.as_object();
    std::printf(
      "  exec result: {size: %lld, time: %lld, bytes: %lld, first: %lld}\n",
      (long long)obj.at("size").as_int(),
      (long long)obj.at("time").as_int(),
      (long long)obj.at("bytes").as_int(),
      (long long)obj.at("first").as_int()
    );

    if (obj.at("size").as_int() != 1024)
      throw std::runtime_error("unexpected size");
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

  // == add_context_provider stub
  std::printf("== cpp11-client: add_context_provider stub\n");
  {
    std::shared_ptr<s5run_cli_example::client> cli = s5run_cli_example::make_client();
    std::vector<std::string> contexts;
    contexts.push_back("Storage");
    bool thrown = false;
    try {
      cli->add_context_provider("s5run://localhost:7775", "some_api_key", contexts);
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
    std::shared_ptr<s5run_cli_example::client> cli = s5run_cli_example::make_client();
    cli->add_function(std::make_shared<MyHasFile>());
    cli->set_server_address(address);
    bool thrown = false;
    try {
      cli->run("function main() { return HasFile(\"x\"); }", "main");
    } catch (const std::exception &e) {
      std::string err = e.what();
      std::printf("  exec error: %s\n", err.substr(0, err.find('\n')).c_str());
      thrown = err.find("boom-hasfile") != std::string::npos;
    }
    if (!thrown)
      throw std::runtime_error("expected 'boom-hasfile' error");
    std::printf("  PASS\n");
  }

  std::printf("cpp11-client: all smoke tests passed\n");
  return 0;
}
