/*
 * MIT License
 *
 *           ░░░░░░░
 *          ░██▓▓▓▓▒
 * ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 * ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 * ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
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
 */

#include "s5run_protocol.h"

#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace s5run {

bool read_exact(int fd, void *buf, size_t n) {
  auto *p = static_cast<char *>(buf);
  while (n > 0) {
    ssize_t r = ::recv(fd, p, n, 0);
    if (r == 0)
      return false;
    if (r < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    p += r;
    n -= static_cast<size_t>(r);
  }
  return true;
}

bool write_all(int fd, const void *buf, size_t n) {
  auto *p = static_cast<const char *>(buf);
  while (n > 0) {
    ssize_t w = ::send(fd, p, n, MSG_NOSIGNAL);
    if (w <= 0)
      return false;
    p += w;
    n -= static_cast<size_t>(w);
  }
  return true;
}

std::optional<std::string> read_frame(int fd) {
  uint32_t net_len = 0;
  if (!read_exact(fd, &net_len, sizeof(net_len)))
    return std::nullopt;
  uint32_t len = ntohl(net_len);
  if (len > s5run::kMaxFrame)
    return std::nullopt;
  std::string out(len, '\0');
  if (len && !read_exact(fd, out.data(), len))
    return std::nullopt;
  return out;
}

bool write_frame(int fd, const std::string &data) {
  uint32_t net_len = htonl(static_cast<uint32_t>(data.size()));
  if (!write_all(fd, &net_len, sizeof(net_len)))
    return false;
  return write_all(fd, data.data(), data.size());
}

bool set_recv_timeout(int fd, int ms) {
  timeval tv{};
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  return ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
}

bool write_message(int fd, uint8_t msg_type, const std::vector<uint8_t> &body) {
  std::vector<uint8_t> buf;
  buf.reserve(1 + body.size());
  buf.push_back(msg_type);
  buf.insert(buf.end(), body.begin(), body.end());
  return write_frame(fd, std::string(reinterpret_cast<const char *>(buf.data()), buf.size()));
}

bool read_message(int fd, uint8_t &msg_type, std::vector<uint8_t> &body) {
  auto frame = read_frame(fd);
  if (!frame || frame->empty())
    return false;
  msg_type = static_cast<uint8_t>((*frame)[0]);
  body.assign(frame->begin() + 1, frame->end());
  return true;
}

} // namespace s5run