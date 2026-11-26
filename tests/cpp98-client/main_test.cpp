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

static std::string load_base_script() {
    const char *paths[] = {"../__test_data/client_scenario0/script.js"};
    for (int i = 0; i < 2; ++i) {
        std::ifstream f(paths[i]);
        if (f.is_open()) {
            std::string content;
            char buf[4096];
            while (f.read(buf, sizeof(buf)) || f.gcount() > 0) {
                content.append(buf, (std::size_t)f.gcount());
            }
            return content;
        }
    }
    throw std::runtime_error("cannot find tests/__test_data/client_scenario0/script.js");
}

static std::string kScript = load_base_script();

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

        // == happy path
        std::printf("== cpp98-client: happy path\n");
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

            s5run_cli_cpp98::remote_server_info server0;
            server0.set_address(address);
            server0.set_api_key("some api key runner");
            ctx->add_server(&server0);
            ctx->set_script(kScript);

            std::vector<s5run_cli_cpp98::s5run_variable> args;
            args.push_back(s5run_cli_cpp98::s5run_variable("test.txt"));
            s5run_cli_cpp98::run_result result = s5run_cli_cpp98::run_script(ctx, "main", args);
            delete ctx;
            if (result.has_error())
                throw std::runtime_error("unexpected error: " + result.error());
            std::printf("  duration: %lld ms\n", (long long)result.duration());

            const std::map<std::string, s5run_cli_cpp98::s5run_variable> &obj = result.get_value().as_object();
            check(obj.find("size")->second.as_int() == 1024, "size");
            check(obj.find("sizeByRef")->second.as_int() == 1024, "sizeByRef");
            check(obj.find("openedCount")->second.as_int() == 2, "openedCount");
            check(obj.find("openedSum")->second.as_int() == 300, "openedSum");
            check(obj.find("fileListCount")->second.as_int() == 3, "fileListCount");
            check(obj.find("time")->second.as_int() == 1737000000LL, "time");
            check(MyGetFile::last_path == "test.txt", "GetFile path");
            check(g_fs_last_path == "test.txt", "FileSystem.GetList path");
            check(g_logs.size() == 4, "console.log count");
            check(g_logs[0] == "hello test.txt 1 2", "console.log");
            check(g_logs[1] == "name: example.txt", "property Name getter");
            check(g_logs[2] == "bufsize: 4096", "property BufferSize getter");
            check(g_logs[3] == "bufsize-after: 2048", "property BufferSize setter");
            check(g_closed_sizes.size() == 2 && g_closed_sizes[0] == 100
                      && g_closed_sizes[1] == 200, "CloseFiles sizes");
            std::printf("  PASS\n");
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
