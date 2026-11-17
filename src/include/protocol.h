#pragma once

#include "wire.h"

#include <cstdint>
#include <optional>
#include <string>
#include <sys/socket.h>

namespace proto {

constexpr uint32_t kMaxFrame = 8 * 1024 * 1024;

bool read_exact(int fd, void *buf, size_t n);
bool write_all(int fd, const void *buf, size_t n);

std::optional<std::string> read_frame(int fd);
bool write_frame(int fd, const std::string &data);

bool set_recv_timeout(int fd, int ms);

// Write a message: frame_len + msg_type + body.
bool write_message(int fd, uint8_t msg_type, const std::vector<uint8_t> &body);

// Read a message: returns (msg_type, body). nullopt on EOF/error.
bool read_message(int fd, uint8_t &msg_type, std::vector<uint8_t> &body);

} // namespace proto