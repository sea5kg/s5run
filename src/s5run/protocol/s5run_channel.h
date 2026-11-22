/*
 * MIT License
 *
 *           ░░░░░░
 *          ░██▓▓▓▓▒
 * ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 * ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 * ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ▒▒░
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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace s5run {

// Bidirectional protocol channel over a TCP socket (or socketpair for tests).
// Handles framing, the hello handshake, and message send/recv.
class channel {
public:
  channel() = default;

  // Connects to a server and performs the hello handshake.
  channel(const std::string &host, uint16_t port);

  // Takes ownership of an existing fd (for socketpair-based tests, proxies).
  explicit channel(int fd);

  channel(const channel &) = delete;
  channel &operator=(const channel &) = delete;

  channel(channel &&other) noexcept;
  channel &operator=(channel &&other) noexcept;

  ~channel();

  void connect(const std::string &host, uint16_t port);
  void disconnect();
  bool is_connected() const;

  // Send a framed message.
  bool send(uint8_t msg_type, const std::vector<uint8_t> &body);
  bool send(uint8_t msg_type, const std::string &body);

  // Receive a framed message (blocking, respects timeout).
  // Returns false on EOF/error/timeout.
  bool recv(uint8_t &msg_type, std::vector<uint8_t> &body);

  // Send a message and wait for a specific response type.
  // Skips interleaved messages (e.g. callbacks) by storing them in `pending`.
  bool send_and_expect(uint8_t msg_type, const std::vector<uint8_t> &body,
                       uint8_t expected_type, std::vector<uint8_t> &resp_body);

  // Perform the hello handshake ("srun <version> <protocol>").
  void read_hello();
  void write_hello();

  // Message that was received but not consumed (from send_and_expect).
  bool pop_pending(uint8_t &msg_type, std::vector<uint8_t> &body);

  // Raw fd for select/poll integration (proxies).
  int fd() const;

  // Socket timeout in milliseconds.
  void set_timeout_ms(int ms);
  int timeout_ms() const;

  // Server version from the hello handshake (empty if not connected).
  const std::string &server_version() const;

private:
  void move_from(channel &&other);

  int m_fd = -1;
  int m_timeout_ms = 10000;
  std::string m_server_version;

  // Messages received but not consumed (e.g. callbacks during send_and_expect).
  std::vector<std::pair<uint8_t, std::vector<uint8_t>>> m_pending;
};

} // namespace s5run
