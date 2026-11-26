// Smoke test for the GENERATED C++98 PROVIDER (MassageCrew).
//
// Usage: ./cpp98-provider [host] [port]
//
// The provider registers the schema context (getQueue function and the
// MassageCrew_Queue class) and serves script requests against it.

#include "d2_s5run_provider.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

static const char *K_SCRIPT =
    "function main(queue_name) {\n"
    "    var q = getQueue(queue_name);\n"
    "    q.publish(\"m1\", \"hello\");\n"
    "    q.publish(\"m2\", \"world\");\n"
    "    return \"published\";\n"
    "}\n";

// -----------------------------------------------------------------------
// MassageCrew implementations
// -----------------------------------------------------------------------

// "queue_name|msg_id|msg_text"
static std::vector<std::string> g_published;

class MyQueue : public s5run_provider::base_context_class_MassageCrew_Queue {
public:
    explicit MyQueue(const std::string &name) : m_name(name) {}

    virtual void publish(const std::string &msg_id, const std::string &msg_text) {
        g_published.push_back(m_name + "|" + msg_id + "|" + msg_text);
    }

private:
    std::string m_name;
};

static std::string g_last_queue_name;

class MyGetQueue : public s5run_provider::base_context_function_getQueue {
public:
    virtual s5run_provider::base_context_class_MassageCrew_Queue *execute(
        const std::string &queue_name) {
        g_last_queue_name = queue_name;
        return new MyQueue(queue_name);
    }
};

// -----------------------------------------------------------------------

static void check(bool cond, const std::string &what) {
    if (!cond)
        throw std::runtime_error("unexpected " + what);
}

int main(int argc, char **argv) {
    std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? atoi(argv[2]) : 7777;

    char address[64];
    snprintf(address, sizeof(address), "s5run://%s:%d", host.c_str(), port);

    std::printf("== cpp98-provider: MassageCrew\n");

    s5run_provider::context *ctx = s5run_provider::make_context();
    s5run_provider::remote_server_info server0;
    server0.set_address(address);
    ctx->add_server(&server0);
    ctx->add_class(new MyQueue("preload"));
    ctx->add_function(new MyGetQueue());
    ctx->set_script(K_SCRIPT);

    std::vector<s5run_provider::s5run_variable> args;
    args.push_back(s5run_provider::s5run_variable(std::string("orders")));
    s5run_provider::run_result result = s5run_provider::run_script(ctx, "main", args);
    if (result.has_error()) {
        std::printf("!!! FAIL: %s\n", result.error().c_str());
        return 1;
    }
    std::printf("  script result: %s\n", result.get_value().as_string().c_str());

    // checks
    check(result.get_value().as_string() == "published", "script result");
    check(g_last_queue_name == "orders", "getQueue queue_name");
    check(g_published.size() == 2, "publish count");
    check(g_published[0] == "orders|m1|hello", "publish m1");
    check(g_published[1] == "orders|m2|world", "publish m2");
    std::printf("  published: %s, %s\n",
                g_published[0].c_str(),
                g_published[1].c_str());

    delete ctx;
    std::printf("  PASS\n");
    std::printf("cpp98-provider: all smoke tests passed\n");
    return 0;
}
