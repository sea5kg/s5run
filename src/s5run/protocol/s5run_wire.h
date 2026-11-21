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

#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace wire {

// Tags for the binary protocol.
enum Tag : uint8_t {
  T_UNDEFINED = 0x00,
  T_BOOL = 0x01,
  T_INT8 = 0x02,
  T_INT16 = 0x03,
  T_INT32 = 0x04,
  T_INT64 = 0x05,
  T_UINT8 = 0x06,
  T_UINT16 = 0x07,
  T_UINT32 = 0x08,
  T_UINT64 = 0x09,
  T_FLOAT32 = 0x0A,
  T_FLOAT64 = 0x0B,
  T_STRING = 0x0C,
  T_ARRAY = 0x0D,
  T_OBJECT = 0x0E,
  T_REF = 0x0F,
};

// String length-prefix sizes.
enum StrLenSize : uint8_t {
  S_LEN_1 = 0x00,
  S_LEN_2 = 0x01,
  S_LEN_4 = 0x02,
};

// ---- low-level writer ----

class Writer {
public:
  Writer() = default;

  const std::vector<uint8_t> &data() const {
    return buf_;
  }
  std::vector<uint8_t> &data() {
    return buf_;
  }

  void put_u8(uint8_t v) {
    buf_.push_back(v);
  }
  void put_i8(int8_t v) {
    buf_.push_back(static_cast<uint8_t>(v));
  }

  void put_u16(uint16_t v) {
    buf_.push_back(v & 0xFF);
    buf_.push_back((v >> 8) & 0xFF);
  }
  void put_u32(uint32_t v) {
    for (int i = 0; i < 4; ++i)
      buf_.push_back((v >> (8 * i)) & 0xFF);
  }
  void put_u64(uint64_t v) {
    for (int i = 0; i < 8; ++i)
      buf_.push_back((v >> (8 * i)) & 0xFF);
  }
  void put_i16(int16_t v) {
    put_u16(static_cast<uint16_t>(v));
  }
  void put_i32(int32_t v) {
    put_u32(static_cast<uint32_t>(v));
  }
  void put_i64(int64_t v) {
    put_u64(static_cast<uint64_t>(v));
  }

  void put_f32(float v) {
    uint32_t u;
    std::memcpy(&u, &v, 4);
    put_u32(u);
  }
  void put_f64(double v) {
    uint64_t u;
    std::memcpy(&u, &v, 8);
    put_u64(u);
  }

  void put_bytes(const uint8_t *p, size_t n) {
    buf_.insert(buf_.end(), p, p + n);
  }

  // STRING with length-prefix by size.
  void put_string(const std::string &s) {
    size_t n = s.size();
    if (n <= 0xFF) {
      put_u8(S_LEN_1);
      put_u8(static_cast<uint8_t>(n));
    } else if (n <= 0xFFFF) {
      put_u8(S_LEN_2);
      put_u16(static_cast<uint16_t>(n));
    } else {
      put_u8(S_LEN_4);
      put_u32(static_cast<uint32_t>(n));
    }
    put_bytes(reinterpret_cast<const uint8_t *>(s.data()), n);
  }

private:
  std::vector<uint8_t> buf_;
};

// ---- low-level reader ----

class Reader {
public:
  Reader(const uint8_t *p, size_t n) : p_(p), end_(p + n) {
  }

  bool eof() const {
    return p_ >= end_;
  }

  uint8_t get_u8() {
    check(1);
    return *p_++;
  }
  int8_t get_i8() {
    return static_cast<int8_t>(get_u8());
  }

  uint16_t get_u16() {
    check(2);
    uint16_t v = p_[0] | (p_[1] << 8);
    p_ += 2;
    return v;
  }
  uint32_t get_u32() {
    check(4);
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i)
      v |= (uint32_t)p_[i] << (8 * i);
    p_ += 4;
    return v;
  }
  uint64_t get_u64() {
    check(8);
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
      v |= (uint64_t)p_[i] << (8 * i);
    p_ += 8;
    return v;
  }
  int16_t get_i16() {
    return static_cast<int16_t>(get_u16());
  }
  int32_t get_i32() {
    return static_cast<int32_t>(get_u32());
  }
  int64_t get_i64() {
    return static_cast<int64_t>(get_u64());
  }

  float get_f32() {
    uint32_t u = get_u32();
    float f;
    std::memcpy(&f, &u, 4);
    return f;
  }
  double get_f64() {
    uint64_t u = get_u64();
    double d;
    std::memcpy(&d, &u, 8);
    return d;
  }

  std::string get_string() {
    uint8_t len_size = get_u8();
    size_t n = 0;
    if (len_size == S_LEN_1)
      n = get_u8();
    else if (len_size == S_LEN_2)
      n = get_u16();
    else if (len_size == S_LEN_4)
      n = get_u32();
    else
      throw std::runtime_error("wire: bad string length size");
    check(n);
    std::string s(reinterpret_cast<const char *>(p_), n);
    p_ += n;
    return s;
  }

  const uint8_t *raw() const {
    return p_;
  }
  size_t remaining() const {
    return end_ - p_;
  }

private:
  void check(size_t n) const {
    if (p_ + n > end_)
      throw std::runtime_error("wire: unexpected end of buffer");
  }

  const uint8_t *p_;
  const uint8_t *end_;
};

} // namespace wire
