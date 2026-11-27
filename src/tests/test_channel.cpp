// Unit test for the s5run protocol channel class.
// Uses socketpair() — no real server binary needed.

#include "protocol/s5run_channel.h"
#include "protocol/s5run_protocol.h"
#include "s5run_globals.h"
#include "engine/s5run_variable.h"
#include "server/s5run_variable_wire.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#define CHECK(cond) do { \
    if (!(cond)) { \
      std::fprintf(stderr, "CHECK failed: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      throw std::runtime_error("check failed: " #cond); \
    } \
  } while (0)

struct detach_guard {
  std::thread &t;
  ~detach_guard() { if (t.joinable()) t.detach(); }
};

using s5run::channel;
using s5run::variable;
using s5run::variable_array;
using s5run::variable_object;
using s5run::object_ref;

// -----------------------------------------------------------------------
// Test helper: a minimal in-process server on the other end of a socketpair
// -----------------------------------------------------------------------

namespace {

void write_hello(int fd) {
  std::string hello = "s5run v0.2.0 " + std::to_string(s5run::S5RUN_PROTOCOL_VERSION);
  std::vector<uint8_t> body(hello.begin(), hello.end());
  s5run::write_message(fd, s5run::M_SERVER_HELLO, body);
}

void send_register_ok(int fd) {
  wire::Writer w;
  w.put_u8(1);
  w.put_string("");
  s5run::write_message(fd, s5run::M_REGISTER_RESP, w.data());
}

void send_exec_result(int fd, const variable &value) {
  wire::Writer w;
  w.put_u8(1);
  s5run::value_to_wire(value, w);
  s5run::write_message(fd, s5run::M_EXEC_RESP, w.data());
}

void send_exec_error(int fd, const std::string &error) {
  wire::Writer w;
  w.put_u8(0);
  w.put_string(error);
  s5run::write_message(fd, s5run::M_EXEC_RESP, w.data());
}

void send_callback(int fd, int32_t cb_id, const std::string &method,
                   const std::vector<variable> &args) {
  wire::Writer w;
  w.put_i32(cb_id);
  w.put_string(method);
  w.put_u32((uint32_t)args.size());
  for (const auto &a : args)
    s5run::value_to_wire(a, w);
  s5run::write_message(fd, s5run::M_CALLBACK_REQ, w.data());
}

void send_stats(int fd, uint32_t threads, uint32_t busy, uint32_t queued, uint32_t clients) {
  wire::Writer w;
  w.put_u32(threads);
  w.put_u32(busy);
  w.put_u32(queued);
  w.put_u32(clients);
  s5run::write_message(fd, s5run::M_STATS_RESP, w.data());
}

// Read a message from the test-server side, returns false on EOF.
bool server_recv(int fd, uint8_t &msg_type, std::vector<uint8_t> &body) {
  return s5run::read_message(fd, msg_type, body);
}

// Read a register message body
std::string read_register_name(wire::Reader &r) {
  return r.get_string();
}

} // namespace

// -----------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------

static void test_hello_and_version() {
  std::fprintf(stderr, "== channel: hello and version\n");

  int sv[2];
  CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

  // Server thread: sends hello, waits, closes.
  std::thread server([sv]() {
    s5run::set_recv_timeout(sv[0], 10000);
    write_hello(sv[0]);
    // Wait for client to disconnect
    uint8_t t;
    std::vector<uint8_t> b;
    s5run::read_message(sv[0], t, b);
    ::close(sv[0]);
  });
  detach_guard guard{server};

  {
    channel ch(sv[1]);
    ch.read_hello();
    CHECK(ch.is_connected());
    CHECK(ch.server_version() == "v0.2.0");
    ch.disconnect();
    CHECK(!ch.is_connected());
  }

  server.detach();
  std::fprintf(stderr, "  PASS\n");
}

static void test_send_recv() {
  std::fprintf(stderr, "== channel: send/recv\n");

  int sv[2];
  CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

  std::thread server([sv]() {
    s5run::set_recv_timeout(sv[0], 10000);
    write_hello(sv[0]);

    // Receive a register_func
    uint8_t t;
    std::vector<uint8_t> b;
    if (!server_recv(sv[0], t, b) || t != s5run::M_REGISTER_FUNC)
      throw std::runtime_error("expected M_REGISTER_FUNC");
    wire::Reader r(b.data(), b.size());
    std::string name = read_register_name(r);
    send_register_ok(sv[0]);

    // Receive an exec request
    if (!server_recv(sv[0], t, b) || t != s5run::M_EXEC_REQ)
      throw std::runtime_error("expected M_EXEC_REQ");

    // Send a callback
    send_callback(sv[0], 1, "GetSize", {variable((int64_t)42)});
    // Receive the callback response
    if (!server_recv(sv[0], t, b) || t != s5run::M_CALLBACK_RESP)
      throw std::runtime_error("expected M_CALLBACK_RESP");

    // Send the exec result
    variable_object obj;
    obj["answer"] = variable((int64_t)42);
    send_exec_result(sv[0], variable(obj));

    // Read until EOF
    while (server_recv(sv[0], t, b)) {}
    ::close(sv[0]);
  });
  detach_guard guard{server};

  {
    channel ch(sv[1]);
    ch.read_hello();

    // Register a function
    wire::Writer reg;
    reg.put_string("GetSize");
    std::vector<uint8_t> resp;
    CHECK(ch.send_and_expect(s5run::M_REGISTER_FUNC, reg.data(),
                               s5run::M_REGISTER_RESP, resp));

    // Send exec
    wire::Writer exec;
    exec.put_string("function main() { return GetSize(); }");
    exec.put_string("main");
    exec.put_u32(0);
    ch.send(s5run::M_EXEC_REQ, exec.data());

    // Receive callback
    uint8_t t;
    std::vector<uint8_t> body;
    CHECK(ch.recv(t, body));
    CHECK(t == s5run::M_CALLBACK_REQ);
    {
      wire::Reader r(body.data(), body.size());
      int32_t cb_id = r.get_i32();
      std::string method = r.get_string();
      CHECK(method == "GetSize");
      // Send callback response
      wire::Writer cb_resp;
      cb_resp.put_i32(cb_id);
      cb_resp.put_u8(1);
      s5run::value_to_wire(variable((int64_t)42), cb_resp);
      ch.send(s5run::M_CALLBACK_RESP, cb_resp.data());
    }

    // Receive exec result
    CHECK(ch.recv(t, body));
    CHECK(t == s5run::M_EXEC_RESP);
    {
      wire::Reader r(body.data(), body.size());
      CHECK(r.get_u8() == 1);
      variable result = s5run::value_from_wire(r);
      CHECK(result.is_object());
      CHECK(result.as_object().at("answer").as_int() == 42);
    }

    ch.disconnect();
  }

  server.detach();
  std::fprintf(stderr, "  PASS\n");
}

static void test_send_and_expect_skips_callbacks() {
  std::fprintf(stderr, "== channel: send_and_expect skips callbacks\n");

  int sv[2];
  CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

  std::thread server([sv]() {
    s5run::set_recv_timeout(sv[0], 10000);
    write_hello(sv[0]);

    // Receive exec request
    uint8_t t;
    std::vector<uint8_t> b;
    server_recv(sv[0], t, b);

    // Send callback BEFORE the exec result
    send_callback(sv[0], 7, "Log", {variable(std::string("hello"))});

    // Then send the exec result
    send_exec_result(sv[0], variable((int64_t)99));

    while (server_recv(sv[0], t, b)) {}
    ::close(sv[0]);
  });
  detach_guard guard{server};

  {
    channel ch(sv[1]);
    ch.read_hello();

    wire::Writer exec;
    exec.put_string("function main() { return 99; }");
    exec.put_string("main");
    exec.put_u32(0);
    std::vector<uint8_t> resp;
    CHECK(ch.send_and_expect(s5run::M_EXEC_REQ, exec.data(),
                               s5run::M_EXEC_RESP, resp));

    // The callback should be available via pop_pending / recv
    uint8_t t;
    std::vector<uint8_t> body;
    CHECK(ch.recv(t, body));
    CHECK(t == s5run::M_CALLBACK_REQ);
    {
      wire::Reader r(body.data(), body.size());
      CHECK(r.get_i32() == 7);
      CHECK(r.get_string() == "Log");
    }

    // Verify the exec result
    {
      wire::Reader r(resp.data(), resp.size());
      CHECK(r.get_u8() == 1);
      CHECK(s5run::value_from_wire(r).as_int() == 99);
    }

    ch.disconnect();
  }

  server.detach();
  std::fprintf(stderr, "  PASS\n");
}

static void test_error_response() {
  std::fprintf(stderr, "== channel: error response\n");

  int sv[2];
  CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

  std::thread server([sv]() {
    s5run::set_recv_timeout(sv[0], 10000);
    write_hello(sv[0]);
    uint8_t t;
    std::vector<uint8_t> b;
    server_recv(sv[0], t, b);
    send_exec_error(sv[0], "call: something went wrong at <script>:5:10");
    while (server_recv(sv[0], t, b)) {}
    ::close(sv[0]);
  });
  detach_guard guard{server};

  {
    channel ch(sv[1]);
    ch.read_hello();

    wire::Writer exec;
    exec.put_string("function main() { throw 'x'; }");
    exec.put_string("main");
    exec.put_u32(0);
    ch.send(s5run::M_EXEC_REQ, exec.data());

    uint8_t t;
    std::vector<uint8_t> body;
    CHECK(ch.recv(t, body));
    CHECK(t == s5run::M_EXEC_RESP);
    {
      wire::Reader r(body.data(), body.size());
      CHECK(r.get_u8() == 0);
      std::string err = r.get_string();
      CHECK(err.find("something") != std::string::npos);
      CHECK(err.find(":5:") != std::string::npos);
    }

    ch.disconnect();
  }

  server.detach();
  std::fprintf(stderr, "  PASS\n");
}

static void test_stats() {
  std::fprintf(stderr, "== channel: stats\n");

  int sv[2];
  CHECK(::socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == 0);

  std::thread server([sv]() {
    s5run::set_recv_timeout(sv[0], 10000);
    write_hello(sv[0]);
    uint8_t t;
    std::vector<uint8_t> b;
    server_recv(sv[0], t, b); // STATS_REQ
    send_stats(sv[0], 4, 2, 1, 3);
    while (server_recv(sv[0], t, b)) {}
    ::close(sv[0]);
  });
  detach_guard guard{server};

  {
    channel ch(sv[1]);
    ch.read_hello();

    std::vector<uint8_t> resp;
    CHECK(ch.send_and_expect(s5run::M_STATS_REQ, {},
                               s5run::M_STATS_RESP, resp));
    {
      wire::Reader r(resp.data(), resp.size());
      CHECK(r.get_u32() == 4); // threads
      CHECK(r.get_u32() == 2); // busy
      CHECK(r.get_u32() == 1); // queued
      CHECK(r.get_u32() == 3); // clients
    }

    ch.disconnect();
  }

  server.detach();
  std::fprintf(stderr, "  PASS\n");
}

int main() {
  try {
    test_hello_and_version();
    test_send_recv();
    test_send_and_expect_skips_callbacks();
    test_error_response();
    test_stats();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "!!! FAIL: %s\n", e.what());
    return 1;
  }
  std::fprintf(stderr, "test_channel: all tests passed\n");
  return 0;
}
