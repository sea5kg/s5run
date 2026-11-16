#include "protocol.h"

#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace proto {

bool read_exact(int fd, void *buf, size_t n) {
  auto *p = static_cast<char *>(buf);
  while (n > 0) {
    ssize_t r = ::recv(fd, p, n, 0);
    if (r == 0)
      return false; // EOF
    if (r < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return false; // timeout
      return false;
    }
    p += r;
    n -= static_cast<size_t>(r);
  }
  return true;
}

std::optional<std::string> read_frame(int fd) {
  uint32_t net_len = 0;
  if (!read_exact(fd, &net_len, sizeof(net_len)))
    return std::nullopt;
  uint32_t len = ntohl(net_len);
  if (len > kMaxFrame)
    return std::nullopt;
  std::string out(len, '\0');
  if (len && !read_exact(fd, out.data(), len))
    return std::nullopt;
  return out;
}

bool write_frame(int fd, const std::string &data) {
  uint32_t net_len = htonl(static_cast<uint32_t>(data.size()));
  if (::send(fd, &net_len, sizeof(net_len), MSG_NOSIGNAL) != sizeof(net_len))
    return false;
  size_t sent = 0;
  while (sent < data.size()) {
    ssize_t w = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
    if (w <= 0)
      return false;
    sent += static_cast<size_t>(w);
  }
  return true;
}

bool set_recv_timeout(int fd, int ms) {
  timeval tv{};
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  return ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
}

} // namespace proto