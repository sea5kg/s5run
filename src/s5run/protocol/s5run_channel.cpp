/**********************************************************************************
 *
 *            ░░░░░░░
 *           ░██▓▓▓▓▒
 *  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 *  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 *  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
 *
 * MIT License
 *
 * Copyright (c) 2026 Evgenii Sopov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Original repository: https://github.com/sea5kg/s5run
 *
 ***********************************************************************************/

#include "s5run_channel.h"
#include "s5run_protocol.h"
#include "s5run_globals.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace s5run {

channel::channel(const std::string &host, uint16_t port) {
  connect(host, port);
}

channel::channel(int fd) : m_fd(fd) {
  if (m_fd >= 0)
    set_timeout_ms(m_timeout_ms);
}

channel::channel(channel &&other) noexcept {
  move_from(std::move(other));
}

channel &channel::operator=(channel &&other) noexcept {
  if (this != &other) {
    disconnect();
    move_from(std::move(other));
  }
  return *this;
}

channel::~channel() {
  disconnect();
}

void channel::move_from(channel &&other) {
  m_fd = other.m_fd;
  m_timeout_ms = other.m_timeout_ms;
  m_server_version = std::move(other.m_server_version);
  m_pending = std::move(other.m_pending);
  other.m_fd = -1;
}

void channel::connect(const std::string &host, uint16_t port) {
  disconnect();

  // Resolve the host (numeric IP or domain name) via getaddrinfo.
  addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  addrinfo *result = nullptr;
  if (::getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0 ||
      result == nullptr)
    throw std::runtime_error("channel: cannot resolve host: " + host);

  int fd = -1;
  for (addrinfo *ai = result; ai != nullptr; ai = ai->ai_next) {
    fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0)
      continue;
    if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0)
      break;
    ::close(fd);
    fd = -1;
  }
  ::freeaddrinfo(result);
  if (fd < 0)
    throw std::runtime_error("channel: connect failed to " + host + ":" + std::to_string(port));

  m_fd = fd;
  set_timeout_ms(m_timeout_ms);

  // Disable Nagle to avoid 40ms delays in request-response patterns.
  int nodelay = 1;
  ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

  read_hello();
}

void channel::disconnect() {
  if (m_fd >= 0) {
    ::close(m_fd);
    m_fd = -1;
  }
  m_server_version.clear();
  m_pending.clear();
}

bool channel::is_connected() const {
  return m_fd >= 0;
}

bool channel::send(uint8_t msg_type, const std::vector<uint8_t> &body) {
  if (m_fd < 0)
    return false;
  return s5run::write_message(m_fd, msg_type, body);
}

bool channel::send(uint8_t msg_type, const std::string &body) {
  std::vector<uint8_t> vec(body.begin(), body.end());
  return send(msg_type, vec);
}

bool channel::recv(uint8_t &msg_type, std::vector<uint8_t> &body) {
  // Check pending messages first (from send_and_expect).
  if (pop_pending(msg_type, body))
    return true;

  if (m_fd < 0)
    return false;
  return s5run::read_message(m_fd, msg_type, body);
}

bool channel::send_and_expect(uint8_t msg_type, const std::vector<uint8_t> &body,
                               uint8_t expected_type, std::vector<uint8_t> &resp_body) {
  if (!send(msg_type, body))
    return false;

  // The expected response may already be in the pending queue.
  for (auto it = m_pending.begin(); it != m_pending.end(); ++it) {
    if (it->first == expected_type) {
      resp_body = std::move(it->second);
      m_pending.erase(it);
      return true;
    }
  }

  // Read fresh messages from the socket; stash anything unexpected.
  while (m_fd >= 0) {
    uint8_t rt = 0;
    std::vector<uint8_t> rb;
    if (!s5run::read_message(m_fd, rt, rb))
      return false;
    if (rt == expected_type) {
      resp_body = std::move(rb);
      return true;
    }
    m_pending.emplace_back(rt, std::move(rb));
  }
  return false;
}

void channel::read_hello() {
  uint8_t msg_type = 0;
  std::vector<uint8_t> body;
  if (!s5run::read_message(m_fd, msg_type, body) || msg_type != s5run::M_SERVER_HELLO)
    throw std::runtime_error("channel: expected server hello");

  std::string text(body.begin(), body.end());
  // "s5run <server-version> <protocol-version>"
  if (text.compare(0, 6, "s5run ") != 0)
    throw std::runtime_error("channel: bad server hello: " + text);

  size_t first_space = text.find(' ');
  size_t last_space = text.rfind(' ');
  if (first_space == std::string::npos || last_space == first_space
      || last_space == std::string::npos)
    throw std::runtime_error("channel: bad server hello: " + text);

  m_server_version = text.substr(first_space + 1, last_space - first_space - 1);

  int protocol = std::atoi(text.substr(last_space + 1).c_str());
  if (protocol != 2)
    throw std::runtime_error("channel: server protocol " + std::to_string(protocol)
                             + " not supported (client supports 2)");
}

void channel::write_hello() {
  std::string hello = std::string("s5run ") + WSJCPP_APP_VERSION + " "
                      + std::to_string(S5RUN_PROTOCOL_VERSION);
  std::vector<uint8_t> body(hello.begin(), hello.end());
  s5run::write_message(m_fd, s5run::M_SERVER_HELLO, body);
}

bool channel::pop_pending(uint8_t &msg_type, std::vector<uint8_t> &body) {
  if (m_pending.empty())
    return false;
  msg_type = m_pending.front().first;
  body = std::move(m_pending.front().second);
  m_pending.erase(m_pending.begin());
  return true;
}

int channel::fd() const {
  return m_fd;
}

void channel::set_timeout_ms(int ms) {
  m_timeout_ms = ms;
  if (m_fd >= 0)
    s5run::set_recv_timeout(m_fd, ms);
}

int channel::timeout_ms() const {
  return m_timeout_ms;
}

const std::string &channel::server_version() const {
  return m_server_version;
}

} // namespace s5run
