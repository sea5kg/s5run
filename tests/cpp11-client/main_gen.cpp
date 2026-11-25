// Smoke test client built on the GENERATED s5run client API.
//
// Usage: ./cpp11-client [host] [port]
#include "s5run_cli_example/s5run_cli_example.h"

#include <dirent.h>
#include <algorithm>
#include <sstream>

#include <cstdio>
#include <fstream>
#include <iterator>
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

  std::string GetName() override {
    return "example.txt";
  }

  int64_t GetBufferSize() override {
    return m_buffer_size;
  }

  void SetBufferSize(int64_t bufferSize) override {
    m_buffer_size = bufferSize;
  }

private:
  int64_t m_buffer_size = 4096;
public:
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
  int64_t execute() override {
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
  void clear_logs();

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

void MyConsole::clear_logs() {
  m_logs.clear();
  m_warnings.clear();
  m_errors.clear();
}

std::string MyConsole::to_line(const std::vector<std::string> &args) {
  std::string line;
  for (size_t i = 0; i < args.size(); ++i)
    line += (line.empty() ? "" : " ") + args[i];
  return line;
}

struct test_data {
  std::string name;
  std::string script_js;
  std::string output_console_expected;
  std::string output_return_expected; // optional: empty = no return check
  std::vector<std::string> input_arguments;
};

static std::string read_file(const std::string &path) {
  std::ifstream f(path.c_str());
  if (!f.is_open()) {
    throw std::runtime_error("cannot find " + path);
  }
  std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  return content;
}

static test_data load_test_data(const std::string &test_name) {
  std::string base_path = "../__test_data/" + test_name;
  test_data ret;
  ret.name = test_name;
  ret.script_js = read_file(base_path + "/script.js");
  ret.output_console_expected = read_file(base_path + "/output_console_expected.txt");
  {
    std::ifstream f((base_path + "/output_return_expected.txt").c_str());
    if (f.is_open())
      ret.output_return_expected =
        std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  }
  {
    // one argument per non-empty line
    std::string input_args = read_file(base_path + "/input_arguments.txt");
    std::istringstream iss(input_args);
    std::string line;
    while (std::getline(iss, line)) {
      if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
      if (!line.empty())
        ret.input_arguments.push_back(line);
    }
  }

  return ret;
}

// Scan ../__test_data/ for sessionX_scenarioX subdirectories.
static std::vector<std::string> list_test_scenarios() {
  const std::string dir = "../__test_data";
  std::vector<std::string> out;
  DIR *d = ::opendir(dir.c_str());
  if (!d)
    throw std::runtime_error("cannot open " + dir);
  struct dirent *entry = nullptr;
  while ((entry = ::readdir(d)) != nullptr) {
    std::string name = entry->d_name;
    if (name == "." || name == "..")
      continue;
    if (name.rfind("session", 0) == 0 && name.find("_scenario") != std::string::npos)
      out.push_back(name);
  }
  ::closedir(d);
  std::sort(out.begin(), out.end());
  return out;
}

static std::string trim_cr(std::string line) {
  if (!line.empty() && line[line.size() - 1] == '\r')
    line.erase(line.size() - 1);
  return line;
}

static std::string value_debug(const s5run_cli_example::value &v) {
  char buf[64];
  switch (v.get_kind()) {
  case s5run_cli_example::value::K_INT:
    snprintf(buf, sizeof(buf), "int %lld", (long long)v.as_int());
    return buf;
  case s5run_cli_example::value::K_STRING:
    return "string \"" + v.as_string() + "\"";
  case s5run_cli_example::value::K_BOOL:
    return std::string("bool ") + (v.as_bool() ? "true" : "false");
  case s5run_cli_example::value::K_DOUBLE:
    snprintf(buf, sizeof(buf), "double %g", v.as_double());
    return buf;
  case s5run_cli_example::value::K_NULL:
    return "null";
  case s5run_cli_example::value::K_ARRAY:
    return "array";
  case s5run_cli_example::value::K_OBJECT:
    return "object";
  default:
    return "ref";
  }
}

// Compare the script return value with output_return_expected.txt:
//   AS_INT <n>
//   AS_BOOL true|false
//   AS_DOUBLE <d>
//   AS_STRING <text>
//   AS_OBJECT
//     <key> <type> <value>      (int|bool|double|string)
static void check_return_value(const std::string &test_name,
                                const s5run_cli_example::value &got,
                                const std::string &expected_raw) {
  std::istringstream iss(expected_raw);
  std::string line;
  if (!std::getline(iss, line))
    throw std::runtime_error(test_name + ": empty output_return_expected.txt");
  line = trim_cr(line);
  const s5run_cli_example::value &v = got;
  std::string where = test_name + ": return value mismatch: ";

  if (line.rfind("AS_INT ", 0) == 0) {
    long long expected = strtoll(line.c_str() + 7, nullptr, 10);
    if (v.get_kind() != s5run_cli_example::value::K_INT || v.as_int() != expected)
      throw std::runtime_error(where + "expected int " + std::to_string(expected)
                               + ", got " + value_debug(v));
  } else if (line.rfind("AS_BOOL ", 0) == 0) {
    bool expected = (line.substr(8) == "true");
    if (v.get_kind() != s5run_cli_example::value::K_BOOL || v.as_bool() != expected)
      throw std::runtime_error(where + "expected bool " + std::string(expected ? "true" : "false")
                               + ", got " + value_debug(v));
  } else if (line.rfind("AS_DOUBLE ", 0) == 0) {
    double expected = strtod(line.c_str() + 10, nullptr);
    if (v.get_kind() != s5run_cli_example::value::K_DOUBLE || v.as_double() != expected)
      throw std::runtime_error(where + "expected double " + std::to_string(expected)
                               + ", got " + value_debug(v));
  } else if (line.rfind("AS_STRING ", 0) == 0) {
    std::string expected = line.substr(10);
    if (v.get_kind() != s5run_cli_example::value::K_STRING || v.as_string() != expected)
      throw std::runtime_error(where + "expected string \"" + expected
                               + "\", got " + value_debug(v));
  } else if (line == "AS_OBJECT") {
    if (v.get_kind() != s5run_cli_example::value::K_OBJECT)
      throw std::runtime_error(where + "expected object, got " + value_debug(v));
    const std::map<std::string, s5run_cli_example::value> &obj = v.as_object();
    std::string field_line;
    while (std::getline(iss, field_line)) {
      field_line = trim_cr(field_line);
      if (field_line.empty())
        continue;
      std::istringstream field_iss(field_line);
      std::string key, ftype;
      field_iss >> key >> ftype;
      std::string fval;
      std::getline(field_iss, fval);
      if (!fval.empty() && fval[0] == ' ')
        fval.erase(0, 1);
      fval = trim_cr(fval);
      auto it = obj.find(key);
      if (it == obj.end())
        throw std::runtime_error(where + "expected field '" + key + "' is missing");
      const s5run_cli_example::value &fv = it->second;
      if (ftype == "int") {
        long long expected = strtoll(fval.c_str(), nullptr, 10);
        if (fv.get_kind() != s5run_cli_example::value::K_INT || fv.as_int() != expected)
          throw std::runtime_error(where + "field '" + key + "': expected int "
                                   + std::to_string(expected) + ", got " + value_debug(fv));
      } else if (ftype == "bool") {
        bool expected = (fval == "true");
        if (fv.get_kind() != s5run_cli_example::value::K_BOOL || fv.as_bool() != expected)
          throw std::runtime_error(where + "field '" + key + "': expected bool " + fval
                                   + ", got " + value_debug(fv));
      } else if (ftype == "double") {
        double expected = strtod(fval.c_str(), nullptr);
        if (fv.get_kind() != s5run_cli_example::value::K_DOUBLE || fv.as_double() != expected)
          throw std::runtime_error(where + "field '" + key + "': expected double " + fval
                                   + ", got " + value_debug(fv));
      } else if (ftype == "string") {
        if (fv.get_kind() != s5run_cli_example::value::K_STRING || fv.as_string() != fval)
          throw std::runtime_error(where + "field '" + key + "': expected string \"" + fval
                                   + "\", got " + value_debug(fv));
      } else {
        throw std::runtime_error(test_name + ": unknown field type '" + ftype
                                 + "' in output_return_expected.txt");
      }
    }
  } else {
    throw std::runtime_error(test_name + ": unknown expected kind: " + line);
  }
}

// Parse a raw argument into a typed value: int -> bool -> string.
static s5run_cli_example::value parse_input_argument(const std::string &raw) {
  if (!raw.empty()) {
    char *end = nullptr;
    long long v = std::strtoll(raw.c_str(), &end, 10);
    if (end && *end == '\0')
      return s5run_cli_example::value((int64_t)v);
  }
  if (raw == "true")
    return s5run_cli_example::value(true);
  if (raw == "false")
    return s5run_cli_example::value(false);
  return s5run_cli_example::value(raw);
}

int main(int argc, char **argv) {
  std::string host = argc > 1 ? argv[1] : "127.0.0.1";
  uint16_t port = argc > 2 ? (uint16_t)atoi(argv[2]) : 7777;

  char address[64];
  snprintf(address, sizeof(address), "s5run://%s:%u", host.c_str(), (unsigned)port);

  std::vector<std::string> scenarios = list_test_scenarios();
  if (scenarios.empty())
    throw std::runtime_error("no sessionX_scenarioX found in ../__test_data");
  std::vector<test_data> tests;
  for (const auto &scenario : scenarios)
    tests.push_back(load_test_data(scenario));

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

    for (int i = 0; i < tests.size(); ++i) {
      test_data test = tests[i];
      std::printf("== cpp11-client: %s\n", test.name.c_str());
      ctx->set_script(test.script_js);
      con->clear_logs();
      std::vector<s5run_cli_example::value> args;
      for (const auto &raw : test.input_arguments)
        args.push_back(parse_input_argument(raw));
      std::shared_ptr<s5run_cli_example::run_result> result = s5run_cli_example::run_script(ctx, "main", args);
      if (result->has_error()) {
        throw std::runtime_error("unexpected error: " + result->error());
      }
      std::printf("  duration: %lld ms\n", (long long)result->duration());

      // return value: compare with output_return_expected.txt
      if (!test.output_return_expected.empty()) {
        check_return_value(test.name, result->get_value(), test.output_return_expected);
        std::printf("  return value matches output_return_expected.txt\n");
      } else {
        std::printf("  return value: no output_return_expected.txt, skipped\n");
      }

      // side-effect checks (callback state, to be moved into data files)
      if (test.name == "session0_scenario0") {
        if (MyFileSystem::m_last_path != "test.txt")
          throw std::runtime_error("unexpected FileSystem.GetList path: " + MyFileSystem::m_last_path);
        if (MyCloseFiles::m_closed_sizes.size() != 2
            || MyCloseFiles::m_closed_sizes[0] != 100
            || MyCloseFiles::m_closed_sizes[1] != 200)
          throw std::runtime_error("unexpected closed sizes");
        if (MyGetFile::m_last_path != "test.txt")
          throw std::runtime_error("unexpected filepath: " + MyGetFile::m_last_path);
      }
      // console output: compare with output_console_expected.txt (line per log)
      std::vector<std::string> expected_console;
      {
        std::istringstream expected_iss(test.output_console_expected);
        std::string expected_line;
        while (std::getline(expected_iss, expected_line)) {
          if (!expected_line.empty() && expected_line[expected_line.size() - 1] == '\r')
            expected_line.erase(expected_line.size() - 1);
          if (!expected_line.empty())
            expected_console.push_back(expected_line);
        }
      }
      std::vector<std::string> got_console = con->get_logs();
      if (got_console != expected_console) {
        std::string details;
        size_t n = got_console.size() > expected_console.size()
                     ? got_console.size() : expected_console.size();
        for (size_t j = 0; j < n; ++j) {
          std::string got = j < got_console.size() ? got_console[j] : "<missing>";
          std::string exp = j < expected_console.size() ? expected_console[j] : "<missing>";
          details += "  line " + std::to_string(j + 1) + ": got \"" + got
                     + "\" expected \"" + exp + "\"\n";
        }
        throw std::runtime_error(test.name + ": console output mismatch:\n" + details);
      }
      std::printf("  console output matches %d lines\n", (int)got_console.size());
      std::printf("  PASS\n");
    }
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
