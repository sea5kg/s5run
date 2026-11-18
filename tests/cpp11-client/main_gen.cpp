// Smoke test client built on the GENERATED s5run client API.
//
// Usage: ./cpp-gen-test [host] [port]
#include "s5run_cli_example.h"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

using namespace s5run_cli_example;

class MyFile : public File {
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

class MyGetFile : public GlobalFunction_GetFile {
public:
    std::shared_ptr<File> execute(const std::string &filepath) override {
        m_last_path = filepath;
        return std::shared_ptr<File>(new MyFile());
    }

    static std::string m_last_path;
};

std::string MyGetFile::m_last_path;

class MyGetTimeNow : public GlobalFunction_GetTimeNow {
public:
    int64_t execute() override {
        return 1737000000;
    }
};

class MyHasFile : public GlobalFunction_HasFile {
public:
    bool execute(const std::string &filepath) override {
        throw std::runtime_error("boom-hasfile");
    }
};

class MyConsole : public console {
public:
    void log(const std::vector<std::string> &args) override {
        for (size_t i = 0; i < args.size(); ++i)
            m_log += (m_log.empty() ? "" : " ") + args[i];
    }

    void warning(const std::vector<std::string> &args) override {
    }

    void error(const std::vector<std::string> &args) override {
    }

    std::string m_log;
};

static const char *kScript = R"(
function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    var b = f.ReadBytes(3);
    console().log(["hello", path]);
    return { size: f.GetSize(), time: t, bytes: b.length, first: b[0] };
}
)";

int main(int argc, char **argv) {
    std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    uint16_t port = argc > 2 ? (uint16_t)atoi(argv[2]) : 7777;

    char address[64];
    snprintf(address, sizeof(address), "s5run://%s:%u", host.c_str(), (unsigned)port);

    // == happy path
    std::printf("== cpp-gen-client: happy path\n");
    {
        std::shared_ptr<MyConsole> con(new MyConsole());

        std::shared_ptr<client> cli = make_client();
        cli->registry_class<MyFile>();
        cli->registry_global_function<MyGetTimeNow>();
        cli->registry_global_function<MyGetFile>();
        cli->set_server_address(address);
        cli->registry_global_object(con);

        std::vector<value> args;
        args.push_back(value("test.txt"));
        value result = cli->run(kScript, "main", args);

        const std::map<std::string, value> &obj = result.as_object();
        std::printf("  exec result: {size: %lld, time: %lld, bytes: %lld, first: %lld}\n",
                    (long long)obj.at("size").as_int(),
                    (long long)obj.at("time").as_int(),
                    (long long)obj.at("bytes").as_int(),
                    (long long)obj.at("first").as_int());

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
        if (con->m_log != "hello test.txt")
            throw std::runtime_error("unexpected console log: " + con->m_log);
        std::printf("  console.log got: \"%s\"\n", con->m_log.c_str());
        std::printf("  PASS\n");
    }

    // == error path
    std::printf("== cpp-gen-client: error path\n");
    {
        std::shared_ptr<client> cli = make_client();
        cli->registry_global_function<MyHasFile>();
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

    std::printf("cpp-gen-client: all smoke tests passed\n");
    return 0;
}
