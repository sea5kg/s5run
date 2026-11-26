// Smoke test for the GENERATED C++98 PROVIDER (MassageCrew).
//
// Usage: ./cpp98-provider <host:port>   (required)
//
// The provider registers the schema context (classes, functions and
// global objects), starts a server on host:port, waits for a client,
// serves metadata requests and API calls until the client disconnects.

#include "d2_s5run_provider.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <host:port>" << std::endl;
        return 1;
    }

    // host:port (required)
    std::string host = "127.0.0.1";
    int port = 7777;
    {
        std::string host_port = argv[1];
        std::string::size_type colon = host_port.rfind(':');
        if (colon == std::string::npos) {
            std::cerr << "Usage: " << argv[0] << " <host:port>" << std::endl;
            return 1;
        }
        host = host_port.substr(0, colon);
        port = atoi(host_port.substr(colon + 1).c_str());
    }

    std::cout << "== cpp98-provider: MassageCrew on " << host << ":" << port << std::endl;

    // 2.1 register the context (classes, functions, global objects)
    s5run_provider::context *ctx = s5run_provider::make_context();
    ctx->add_class(new MyQueue("preload"));
    ctx->add_function(new MyGetQueue());

    // 2.2 start the server and wait for a client; serve until it disconnects
    if (!s5run_provider::serve(ctx, host, port)) {
        std::cout << "!!! FAIL: cannot serve on " << host << ":" << port << std::endl;
        return 1;
    }
    delete ctx;

    // checks: the client published two messages via the API
    check(g_last_queue_name == "orders", "getQueue queue_name");
    check(g_published.size() == 2, "publish count");
    check(g_published[0] == "orders|m1|hello", "publish m1");
    check(g_published[1] == "orders|m2|world", "publish m2");
    std::cout << "  published: " << g_published[0] << ", " << g_published[1] << std::endl;

    std::cout << "  PASS" << std::endl;
    std::cout << "cpp98-provider: all smoke tests passed" << std::endl;
    return 0;
}
