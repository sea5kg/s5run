#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <sys/socket.h>

namespace proto {

constexpr uint32_t kMaxFrame = 8 * 1024 * 1024; // 8 MiB

// Reads exactly n bytes. Returns false on EOF or error.
bool read_exact(int fd, void *buf, size_t n);

// Reads one length-prefixed frame. nullopt means the connection is closed.
std::optional<std::string> read_frame(int fd);

// Writes one length-prefixed frame.
bool write_frame(int fd, const std::string &data);

// Sets a receive timeout in milliseconds. 0 disables the timeout.
bool set_recv_timeout(int fd, int ms);

} // namespace proto