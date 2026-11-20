// Raw binary protocol smoke test: spawns the s5run server and runs
// client scenarios against it (registration, exec, callbacks, errors).
//
// Usage: test_protocol_client <path-to-s5run>
#include "internal/s5run_globals.h"
#include "s5run_protocol.h"
#include "value_wire.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

s5run::Value dispatch(const std::string &method, const std::vector<s5run::Value> &args) {
  if (method == "GetTimeNow")
    return s5run::Value((int64_t)1737000000);
  if (method == "GetFile")
    return s5run::Value(s5run::ObjectRef(42, "File"));
  if (method == "GetSize")
    return s5run::Value((int64_t)1024);
  throw std::runtime_error("unknown method " + method);
}

void send_register_func(int fd, const std::string &name) {
  wire::Writer w;
  w.put_string(name);
  s5run::write_message(fd, s5run::M_REGISTER_FUNC, w.data());
}

void send_register_type(int fd, const std::string &name, const std::vector<std::string> &methods) {
  wire::Writer w;
  w.put_string(name);
  w.put_u32((uint32_t)methods.size());
  for (const auto &m : methods)
    w.put_string(m);
  s5run::write_message(fd, s5run::M_REGISTER_TYPE, w.data());
}

void expect_register_ok(int fd, const std::string &what) {
  uint8_t msg_type = 0;
  std::vector<uint8_t> body;
  if (!s5run::read_message(fd, msg_type, body) || msg_type != s5run::M_REGISTER_RESP)
    throw std::runtime_error(what + ": unexpected response");
  wire::Reader r(body.data(), body.size());
  if (r.get_u8() == 0)
    throw std::runtime_error(what + " failed: " + r.get_string());
}

void run_with_dispatch(int fd, bool &ok, s5run::Value &value, std::string &error) {
  while (true) {
    uint8_t msg_type = 0;
    std::vector<uint8_t> body;
    if (!s5run::read_message(fd, msg_type, body))
      throw std::runtime_error("connection closed");
    if (msg_type == s5run::M_CALLBACK_REQ) {
      wire::Reader r(body.data(), body.size());
      int32_t cid = r.get_i32();
      std::string method = r.get_string();
      std::vector<s5run::Value> args;
      uint32_t count = r.get_u32();
      for (uint32_t i = 0; i < count; ++i)
        args.push_back(s5run::value_from_wire(r));

      std::printf("  <- callback %s\n", method.c_str());
      wire::Writer w;
      w.put_i32(cid);
      try {
        s5run::Value result = dispatch(method, args);
        w.put_u8(1);
        s5run::value_to_wire(result, w);
      } catch (const std::exception &e) {
        w.put_u8(0);
        w.put_string(e.what());
      }
      s5run::write_message(fd, s5run::M_CALLBACK_RESP, w.data());
      continue;
    }
    if (msg_type != s5run::M_EXEC_RESP)
      throw std::runtime_error("unexpected message type");
    wire::Reader r(body.data(), body.size());
    ok = r.get_u8() != 0;
    if (ok)
      value = s5run::value_from_wire(r);
    else
      error = r.get_string();
    return;
  }
}

int connect_to(const std::string &host, uint16_t port) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    throw std::runtime_error("socket failed");
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    throw std::runtime_error("bad host");
  if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0)
    throw std::runtime_error("connect failed");
  s5run::set_recv_timeout(fd, 10000);

  // consume the server hello
  uint8_t hello_type = 0;
  std::vector<uint8_t> hello_body;
  if (!s5run::read_message(fd, hello_type, hello_body)
      || hello_type != s5run::M_SERVER_HELLO)
    throw std::runtime_error("expected server hello");
  return fd;
}

void test_happy_path(const std::string &host, uint16_t port) {
  std::printf("== protocol: happy path\n");
  int fd = connect_to(host, port);

  send_register_type(fd, "File", {"GetSize", "ReadBytes"});
  expect_register_ok(fd, "register_type(File)");
  send_register_func(fd, "GetFile");
  expect_register_ok(fd, "register_func(GetFile)");
  send_register_func(fd, "GetTimeNow");
  expect_register_ok(fd, "register_func(GetTimeNow)");
  std::printf("  registered type 'File', functions GetFile/GetTimeNow\n");

  std::string script = R"(
function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    return { size: f.GetSize(), time: t };
}
)";
  wire::Writer w;
  w.put_string(script);
  w.put_string("main");
  w.put_u32(1);
  s5run::value_to_wire(s5run::Value(std::string("test.txt")), w);
  s5run::write_message(fd, s5run::M_EXEC_REQ, w.data());

  bool ok = false;
  s5run::Value value;
  std::string error;
  run_with_dispatch(fd, ok, value, error);
  if (!ok)
    throw std::runtime_error("exec failed: " + error);

  const s5run::ValueObject &obj = value.as_object();
  if (obj.at("size").as_int() != 1024 || obj.at("time").as_int() != 1737000000)
    throw std::runtime_error("unexpected result");
  std::printf(
    "  exec result: {size: %lld, time: %lld}\n", (long long)obj.at("size").as_int(), (long long)obj.at("time").as_int()
  );
  ::close(fd);
  std::printf("  PASS\n");
}

void test_callback_error(const std::string &host, uint16_t port) {
  std::printf("== protocol: callback error path\n");
  int fd = connect_to(host, port);

  send_register_func(fd, "Fail");
  expect_register_ok(fd, "register_func(Fail)");

  wire::Writer w;
  w.put_string("function main() { return Fail(); }");
  w.put_string("main");
  w.put_u32(0);
  s5run::write_message(fd, s5run::M_EXEC_REQ, w.data());

  uint8_t msg_type = 0;
  std::vector<uint8_t> body;
  if (!s5run::read_message(fd, msg_type, body) || msg_type != s5run::M_CALLBACK_REQ)
    throw std::runtime_error("expected callback");
  wire::Reader r(body.data(), body.size());
  int32_t cid = r.get_i32();
  std::string method = r.get_string();
  if (method != "Fail")
    throw std::runtime_error("unexpected callback: " + method);
  std::printf("  <- callback %s\n", method.c_str());

  wire::Writer resp;
  resp.put_i32(cid);
  resp.put_u8(0);
  resp.put_string("boom");
  s5run::write_message(fd, s5run::M_CALLBACK_RESP, resp.data());

  bool ok = false;
  s5run::Value value;
  std::string error;
  run_with_dispatch(fd, ok, value, error);
  if (ok)
    throw std::runtime_error("expected failure");
  if (error.find("boom") == std::string::npos)
    throw std::runtime_error("error without 'boom': " + error);
  std::printf("  exec error: %s\n", error.substr(0, error.find('\n')).c_str());
  ::close(fd);
  std::printf("  PASS\n");
}

uint16_t free_port() {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    throw std::runtime_error("socket failed");
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ::bind(fd, (sockaddr *)&addr, sizeof(addr));
  sockaddr_in bound{};
  socklen_t len = sizeof(bound);
  ::getsockname(fd, (sockaddr *)&bound, &len);
  uint16_t port = ntohs(bound.sin_port);
  ::close(fd);
  return port;
}

pid_t spawn_server(const std::string &server_bin, uint16_t port) {
  pid_t pid = ::fork();
  if (pid < 0)
    throw std::runtime_error("fork failed");
  if (pid == 0) {
    int devnull = ::open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      ::dup2(devnull, STDOUT_FILENO);
      ::dup2(devnull, STDERR_FILENO);
    }
    std::string listen_addr = "127.0.0.1:" + std::to_string(port);
    ::execl(server_bin.c_str(), server_bin.c_str(), "--listen", listen_addr.c_str(), "--threads", "2", (char *)NULL);
    ::_exit(127);
  }
  return pid;
}

void wait_server_up(uint16_t port) {
  for (int i = 0; i < 100; ++i) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) == 0) {
      ::close(fd);
      return;
    }
    ::close(fd);
    usleep(50000);
  }
  throw std::runtime_error("server did not start");
}

} // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <path-to-s5run>\n", argv[0]);
    return 2;
  }
  std::string server_bin = argv[1];
  std::signal(SIGPIPE, SIG_IGN);

  try {
    uint16_t port = free_port();
    pid_t server = spawn_server(server_bin, port);
    int rc = 0;
    try {
      wait_server_up(port);
      test_happy_path("127.0.0.1", port);
      test_callback_error("127.0.0.1", port);
    } catch (...) {
      ::kill(server, SIGKILL);
      ::waitpid(server, NULL, 0);
      throw;
    }
    ::kill(server, SIGTERM);
    ::waitpid(server, NULL, 0);
    std::printf("protocol client: all tests passed\n");
    return rc;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "!!! FAIL: %s\n", e.what());
    return 1;
  }
}
