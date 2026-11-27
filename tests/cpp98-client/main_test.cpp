// Smoke test for the GENERATED legacy C++98 client (Windows/Linux portable).
//
// Usage: ./cpp98-client [host] [port]
#include "d1_s5run_cli_cpp98.h"

#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>

#include <dirent.h>
#include <algorithm>
#include <sstream>

struct test_data {
    std::string name;
    std::string script_js;
    std::string output_console_expected;
    std::string output_return_expected; // empty = no return check
    std::vector<std::string> input_arguments;
};

static std::string read_file(const std::string &path) {
    std::ifstream f(path.c_str());
    if (!f.is_open())
        throw std::runtime_error("cannot find " + path);
    std::string content;
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0)
        content.append(buf, (std::size_t)f.gcount());
    return content;
}

static std::string read_file_optional(const std::string &path) {
    std::ifstream f(path.c_str());
    if (!f.is_open())
        return "";
    std::string content;
    char buf[4096];
    while (f.read(buf, sizeof(buf)) || f.gcount() > 0)
        content.append(buf, (std::size_t)f.gcount());
    return content;
}

// Split text into lines, trimming trailing \r and skipping empty lines.
static std::vector<std::string> split_lines(const std::string &text) {
    std::vector<std::string> out;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (!line.empty())
            out.push_back(line);
    }
    return out;
}

static test_data load_test_data(const std::string &test_name) {
    std::string base_path = "../__test_data/" + test_name;
    test_data ret;
    ret.name = test_name;
    ret.script_js = read_file(base_path + "/script.js");
    ret.output_console_expected = read_file(base_path + "/output_console_expected.txt");
    ret.output_return_expected = read_file_optional(base_path + "/output_return_expected.txt");
    ret.input_arguments = split_lines(read_file(base_path + "/input_arguments.txt"));
    return ret;
}

// Scan ../__test_data/ for client_scenarioX subdirectories.
static std::vector<std::string> list_test_scenarios() {
    const std::string dir = "../__test_data";
    std::vector<std::string> out;
    DIR *d = ::opendir(dir.c_str());
    if (!d)
        throw std::runtime_error("cannot open " + dir);
    struct dirent *entry = 0;
    while ((entry = ::readdir(d)) != 0) {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        if (name.rfind("client_", 0) == 0 && name.find("_scenario") != std::string::npos)
            out.push_back(name);
    }
    ::closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}

// Compare the script return value with output_return_expected.txt:
// both sides use s5run_variable::to_string_s5run() format.
static void check_return_value(const std::string &test_name,
                                const s5run_cli_cpp98::s5run_variable &got,
                                const std::string &expected_raw) {
    std::vector<std::string> expected = split_lines(expected_raw);
    std::vector<std::string> got_lines = split_lines(got.to_string_s5run());
    if (got_lines == expected)
        return;

    std::string details;
    size_t n = got_lines.size() > expected.size() ? got_lines.size() : expected.size();
    for (size_t i = 0; i < n; ++i) {
        std::string got_line = i < got_lines.size() ? got_lines[i] : "<missing>";
        std::string exp_line = i < expected.size() ? expected[i] : "<missing>";
        if (got_line != exp_line) {
            char numbuf[32];
            std::sprintf(numbuf, "%d", (int)(i + 1));
            details += std::string("  line ") + numbuf + ": got \"" + got_line
                       + "\" expected \"" + exp_line + "\"\n";
        }
    }
    throw std::runtime_error(test_name + ": return value mismatch:\n" + details);
}

// Parse a raw argument into a typed variable: int -> bool -> string.
static s5run_cli_cpp98::s5run_variable parse_input_argument(const std::string &raw) {
    if (!raw.empty()) {
        char *end = 0;
        long long v = std::strtoll(raw.c_str(), &end, 10);
        if (end && *end == '\0')
            return s5run_cli_cpp98::s5run_variable((s5run_cli_cpp98::i64)v);
    }
    if (raw == "true")
        return s5run_cli_cpp98::s5run_variable(true);
    if (raw == "false")
        return s5run_cli_cpp98::s5run_variable(false);
    return s5run_cli_cpp98::s5run_variable(raw);
}

class MyFile : public s5run_cli_cpp98::base_context_class_File {
public:
    explicit MyFile(s5run_cli_cpp98::i64 size) : m_size(size), m_buffer_size(4096) {}

    virtual s5run_cli_cpp98::i64 GetSize() { return m_size; }

    virtual std::vector<s5run_cli_cpp98::u8> ReadBytes(s5run_cli_cpp98::i64 readBytes) {
        std::vector<s5run_cli_cpp98::u8> out;
        for (s5run_cli_cpp98::i64 i = 0; i < readBytes; ++i)
            out.push_back((s5run_cli_cpp98::u8)('a' + (int)(i % 26)));
        return out;
    }

    virtual std::string GetName() { return "example.txt"; }

    virtual s5run_cli_cpp98::i64 GetBufferSize() { return m_buffer_size; }

    virtual void SetBufferSize(s5run_cli_cpp98::i64 bufferSize) {
        m_buffer_size = bufferSize;
    }

private:
    s5run_cli_cpp98::i64 m_size;
    s5run_cli_cpp98::i64 m_buffer_size;
public:
    MyFile() : m_buffer_size(4096) {}
};


class MyKeyValueStorage : public s5run_cli_cpp98::base_context_class_KeyValueStorage {
public:
    // 'new KeyValueStorage()' in scripts constructs a new instance
    virtual base_context_class_KeyValueStorage *s5run_construct() {
        return new MyKeyValueStorage();
    }

    virtual void SetValueByKey(const std::string &name,
                               const s5run_cli_cpp98::s5run_variable &value) {
        m_store[name] = value;
    }

    virtual s5run_cli_cpp98::s5run_variable GetValueByKey(const std::string &name) {
        std::map<std::string, s5run_cli_cpp98::s5run_variable>::const_iterator it = m_store.find(name);
        return it == m_store.end() ? s5run_cli_cpp98::s5run_variable() : it->second;
    }

private:
    std::map<std::string, s5run_cli_cpp98::s5run_variable> m_store;
};

class MyGetFile : public s5run_cli_cpp98::base_context_function_GetFile {
public:
    static std::string last_path;

    virtual s5run_cli_cpp98::base_context_class_File *execute(const std::string &filepath) {
        last_path = filepath;
        return new MyFile(1024);
    }
};

std::string MyGetFile::last_path;

class MyGetFileSize : public s5run_cli_cpp98::base_context_function_GetFileSize {
public:
    virtual s5run_cli_cpp98::i64 execute(s5run_cli_cpp98::base_context_class_File *file) {
        return file->GetSize();
    }
};

class MyGetTimeNow : public s5run_cli_cpp98::base_context_function_GetTimeNow {
public:
    virtual s5run_cli_cpp98::i64 execute() { return 1737000000LL; }
};

class MyHasFile : public s5run_cli_cpp98::base_context_function_HasFile {
public:
    virtual bool execute(const std::string &filepath) {
        throw std::runtime_error("boom-hasfile");
    }
};

class MyGetOpenedFiles : public s5run_cli_cpp98::base_context_function_GetOpenedFiles {
public:
    virtual std::vector<s5run_cli_cpp98::base_context_class_File *> execute() {
        std::vector<s5run_cli_cpp98::base_context_class_File *> out;
        out.push_back(new MyFile(100));
        out.push_back(new MyFile(200));
        return out;
    }
};

static std::vector<s5run_cli_cpp98::i64> g_closed_sizes;

class MyCloseFiles : public s5run_cli_cpp98::base_context_function_CloseFiles {
public:
    virtual void execute(const std::vector<s5run_cli_cpp98::base_context_class_File *> &files) {
        for (std::size_t i = 0; i < files.size(); ++i)
            g_closed_sizes.push_back(files[i]->GetSize());
    }
};

class MyGetPi : public s5run_cli_cpp98::base_context_function_GetPi {
public:
    virtual double execute() { return 3.14; }
};

class MyGetServerPort : public s5run_cli_cpp98::base_context_function_GetServerPort {
public:
    virtual s5run_cli_cpp98::i64 execute() { return 8080; }
};

static std::vector<std::string> g_logs;

class MyConsole : public s5run_cli_cpp98::base_context_class_console {
public:
    virtual void log(const std::vector<std::string> &args) {
        std::string line;
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (!line.empty())
                line += " ";
            line += args[i];
        }
        g_logs.push_back(line);
    }

    virtual void warning(const std::vector<std::string> &args) { (void)args; }
    virtual void error(const std::vector<std::string> &args) { (void)args; }
};

static std::string g_fs_last_path;

class MyFileSystem : public s5run_cli_cpp98::base_context_class_FileSystem {
public:
    virtual std::vector<std::string> GetList(const std::string &filepath) {
        g_fs_last_path = filepath;
        std::vector<std::string> out;
        out.push_back("a.txt");
        out.push_back("b.txt");
        out.push_back("c.txt");
        return out;
    }
};

static void check(bool cond, const char *what) {
    if (!cond)
        throw std::runtime_error(std::string("unexpected ") + what);
}

int main(int argc, char **argv) {
    std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? std::atoi(argv[2]) : 7777;
    std::string address = "s5run://" + host + ":" + (argc > 2 ? argv[2] : "7777");

    try {
        check(std::string(s5run_cli_cpp98::context::client_version()) == "1.0.0",
              "client version");

        // == data-driven scenarios (like cpp11-client)
        {
            std::vector<std::string> scenarios = list_test_scenarios();
            if (scenarios.empty())
                throw std::runtime_error("no client_scenarioX found in ../__test_data");
            std::vector<test_data> tests;
            for (std::size_t i = 0; i < scenarios.size(); ++i)
                tests.push_back(load_test_data(scenarios[i]));

            for (std::size_t i = 0; i < tests.size(); ++i) {
                const test_data &test = tests[i];
                std::printf("== cpp98-client: %s\n", test.name.c_str());
                {
                    s5run_cli_cpp98::context *ctx = s5run_cli_cpp98::make_context();
                    MyFile sample(1024);
                    ctx->add_class(&sample);
                    ctx->add_function(new MyGetFile());
                    ctx->add_function(new MyGetFileSize());
                    ctx->add_function(new MyGetTimeNow());
                    ctx->add_function(new MyGetOpenedFiles());
                    ctx->add_function(new MyCloseFiles());
                    ctx->add_function(new MyGetPi());
                    ctx->add_function(new MyGetServerPort());
                    MyFileSystem fs;
                    MyConsole con;
                    ctx->add_object(&fs);
                    ctx->add_object(&con);
                    MyKeyValueStorage kvs;
                    ctx->add_class(&kvs);

                    s5run_cli_cpp98::remote_server_info server0;
                    server0.set_address(address);
                    server0.set_api_key("some api key runner");
                    ctx->add_server(&server0);
                    ctx->set_script(test.script_js);

                    g_logs.clear();
                    std::vector<s5run_cli_cpp98::s5run_variable> args;
                    for (std::size_t j = 0; j < test.input_arguments.size(); ++j)
                        args.push_back(parse_input_argument(test.input_arguments[j]));
                    s5run_cli_cpp98::run_result result = s5run_cli_cpp98::run_script(ctx, "main", args);
                    delete ctx;
                    if (result.has_error()) {
                        std::printf("  SCRIPT ERROR: %s\n", result.error().c_str());
                        std::fflush(stdout);
                        throw std::runtime_error(test.name + ": script error: " + result.error());
                    }
                    std::printf("  duration: %lld ms\n", (long long)result.duration());

                    // return value: compare with output_return_expected.txt
                    if (!test.output_return_expected.empty()) {
                        check_return_value(test.name, result.get_value(), test.output_return_expected);
                        std::printf("  return value: %s\n", result.get_value().to_string_s5run().c_str());
                        std::printf("  return json:  %s\n", result.get_value().to_string_json().c_str());
                        std::printf("  return value matches output_return_expected.txt\n");
                    } else {
                        std::printf("  return value: no output_return_expected.txt, skipped\n");
                    }

                    // console output: compare with output_console_expected.txt (line per log)
                    {
                        std::vector<std::string> expected_console = split_lines(test.output_console_expected);
                        if (g_logs != expected_console) {
                            std::string details;
                            size_t n = g_logs.size() > expected_console.size()
                                         ? g_logs.size() : expected_console.size();
                            for (size_t j = 0; j < n; ++j) {
                                std::string got_line = j < g_logs.size() ? g_logs[j] : "<missing>";
                                std::string exp_line = j < expected_console.size() ? expected_console[j] : "<missing>";
                                if (got_line != exp_line) {
                                    char numbuf[32];
                                    std::sprintf(numbuf, "%d", (int)(j + 1));
                                    details += std::string("  line ") + numbuf + ": got \"" + got_line
                                               + "\" expected \"" + exp_line + "\"\n";
                                }
                            }
                            throw std::runtime_error(test.name + ": console output mismatch:\n" + details);
                        }
                        std::printf("  console output matches %d lines\n", (int)g_logs.size());
                    }

                    // side-effect checks (callback state, to be moved into data files)
                    if (test.name == "client_scenario0") {
                        check(MyGetFile::last_path == "test.txt", "GetFile path");
                        check(g_fs_last_path == "test.txt", "FileSystem.GetList path");
                        check(g_closed_sizes.size() == 2 && g_closed_sizes[0] == 100
                                  && g_closed_sizes[1] == 200, "CloseFiles sizes");
                    }
                    std::printf("  PASS\n");
                }
            }
        }

        // == primitives (two servers, load balancing)
        std::printf("== cpp98-client: primitives (two servers)\n");
        {
            s5run_cli_cpp98::context *ctx = s5run_cli_cpp98::make_context();
            ctx->add_function(new MyGetPi());
            ctx->add_function(new MyGetServerPort());
            s5run_cli_cpp98::remote_server_info s0;
            s0.set_address(address);
            s5run_cli_cpp98::remote_server_info s1;
            s1.set_address(address);
            ctx->add_server(&s0);
            ctx->add_server(&s1);
            ctx->set_script("function main() { return { pi: GetPi(), port: GetServerPort() }; }");

            s5run_cli_cpp98::run_result result = s5run_cli_cpp98::run_script(ctx, "main");
            delete ctx;
            if (result.has_error())
                throw std::runtime_error("unexpected error: " + result.error());
            const std::map<std::string, s5run_cli_cpp98::s5run_variable> &obj = result.get_value().as_object();
            double pi = obj.find("pi")->second.as_double();
            check(pi > 3.13 && pi < 3.15, "pi");
            check(obj.find("port")->second.as_int() == 8080, "port");
            std::printf("  PASS\n");
        }

        // == error path
        std::printf("== cpp98-client: error path\n");
        {
            s5run_cli_cpp98::context *ctx = s5run_cli_cpp98::make_context();
            ctx->add_function(new MyHasFile());
            s5run_cli_cpp98::remote_server_info s0;
            s0.set_address(address);
            ctx->add_server(&s0);
            ctx->set_script("function main() {\n  return HasFile(\"x\");\n}");

            s5run_cli_cpp98::run_result result = s5run_cli_cpp98::run_script(ctx, "main");
            delete ctx;
            check(result.has_error(), "expected error");
            std::printf("  error: %s\n", result.error().substr(0, result.error().find('\n')).c_str());
            std::printf("  error line: %d\n", result.error_line());
            check(result.error().find("boom-hasfile") != std::string::npos, "boom-hasfile");
            check(result.error_line() == 2, "error line");
            std::printf("  PASS\n");
        }

        std::printf("cpp98-client: all smoke tests passed\n");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "!!! FAIL: %s\n", e.what());
        return 1;
    }
}
