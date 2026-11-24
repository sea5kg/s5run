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

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace s5run {

class variable;
typedef std::vector<variable> variable_array;
typedef std::map<std::string, variable> variable_object;

struct ObjectRef {
  int this_id;
  std::string type;

  ObjectRef() : this_id(0) {
  }
  ObjectRef(int id, const std::string &t) : this_id(id), type(t) {
  }
};

class variable {
public:
  enum Kind {
    K_NULL = 0,
    K_BOOL,
    K_INT,
    K_DOUBLE,
    K_STRING,
    K_ARRAY,
    K_OBJECT,
    K_REF,
  };

  // ---- constructors ----

  variable() : kind_(K_NULL) {
  }
  variable(std::nullptr_t) : kind_(K_NULL) {
  }

  variable(bool b) : kind_(K_BOOL) {
    u_.b = b;
  }

  variable(int32_t i) : kind_(K_INT) {
    u_.i = static_cast<int64_t>(i);
  }
  variable(int64_t i) : kind_(K_INT) {
    u_.i = i;
  }
  variable(uint32_t i) : kind_(K_INT) {
    u_.i = static_cast<int64_t>(i);
  }
  variable(uint64_t i) : kind_(K_INT) {
    u_.i = static_cast<int64_t>(i);
  }

  variable(double d) : kind_(K_DOUBLE) {
    u_.d = d;
  }

  variable(const char *s) : kind_(K_STRING) {
    new (&u_.s) std::string(s);
  }
  variable(const std::string &s) : kind_(K_STRING) {
    new (&u_.s) std::string(s);
  }

  variable(const variable_array &a) : kind_(K_ARRAY) {
    new (&u_.a) variable_array(a);
  }
  variable(const variable_object &o) : kind_(K_OBJECT) {
    new (&u_.o) variable_object(o);
  }
  variable(const ObjectRef &r) : kind_(K_REF) {
    new (&u_.r) ObjectRef(r);
  }

  // Parse a raw command-line argument into a typed variable:
  // int -> bool -> double -> string.
  static variable parse_script_arg(const std::string &raw) {
    // int?
    if (!raw.empty()) {
      char *end = nullptr;
      long long v = std::strtoll(raw.c_str(), &end, 10);
      if (end && *end == '\0')
        return variable(static_cast<int64_t>(v));
    }
    // bool?
    if (raw == "true")
      return variable(true);
    if (raw == "false")
      return variable(false);
    // double?
    if (!raw.empty()) {
      char *end = nullptr;
      double d = std::strtod(raw.c_str(), &end);
      if (end && *end == '\0')
        return variable(d);
    }
    return variable(raw);
  }

  variable(const variable &other) : kind_(other.kind_) {
    copy_from(other);
  }

  ~variable() {
    destroy();
  }

  // ---- assignment ----

  variable &operator=(const variable &other) {
    if (this != &other) {
      destroy();
      kind_ = other.kind_;
      copy_from(other);
    }
    return *this;
  }

  // ---- introspection ----

  Kind kind() const {
    return kind_;
  }

  bool is_null() const {
    return kind_ == K_NULL;
  }
  bool is_bool() const {
    return kind_ == K_BOOL;
  }
  bool is_int() const {
    return kind_ == K_INT;
  }
  bool is_double() const {
    return kind_ == K_DOUBLE;
  }
  bool is_number() const {
    return kind_ == K_INT || kind_ == K_DOUBLE;
  }
  bool is_string() const {
    return kind_ == K_STRING;
  }
  bool is_array() const {
    return kind_ == K_ARRAY;
  }
  bool is_object() const {
    return kind_ == K_OBJECT;
  }
  bool is_ref() const {
    return kind_ == K_REF;
  }

  // ---- accessors (caller must check kind first) ----

  bool as_bool() const {
    return u_.b;
  }
  int64_t as_int() const {
    return u_.i;
  }
  double as_double() const {
    return (kind_ == K_INT) ? static_cast<double>(u_.i) : u_.d;
  }
  const std::string &as_string() const {
    return u_.s;
  }
  const variable_array &as_array() const {
    return u_.a;
  }
  const variable_object &as_object() const {
    return u_.o;
  }
  const ObjectRef &as_ref() const {
    return u_.r;
  }

  variable_array &array() {
    return u_.a;
  }
  variable_object &object() {
    return u_.o;
  }
  std::string &string() {
    return u_.s;
  }

private:
  union Storage {
    bool b;
    int64_t i;
    double d;
    std::string s;
    variable_array a;
    variable_object o;
    ObjectRef r;

    Storage() {
    }
    ~Storage() {
    }
  };

  void destroy() {
    switch (kind_) {
    case K_NULL:
    case K_BOOL:
    case K_INT:
    case K_STRING:
    case K_DOUBLE:
      break;
    // case K_STRING: u_.s.~string(); break;         // std::string
    // case K_STRING: u_.s.~basic_string(); break;         // std::string
    case K_ARRAY:
      u_.a.~variable_array();
      break; // typedef
    case K_OBJECT:
      u_.o.~variable_object();
      break; // typedef
    case K_REF:
      u_.r.~ObjectRef();
      break;
    }
    kind_ = K_NULL;
  }

  void copy_from(const variable &other) {
    switch (other.kind_) {
    case K_NULL:
      break;
    case K_BOOL:
      u_.b = other.u_.b;
      break;
    case K_INT:
      u_.i = other.u_.i;
      break;
    case K_DOUBLE:
      u_.d = other.u_.d;
      break;
    case K_STRING:
      new (&u_.s) std::string(other.u_.s);
      break;
    case K_ARRAY:
      new (&u_.a) variable_array(other.u_.a);
      break;
    case K_OBJECT:
      new (&u_.o) variable_object(other.u_.o);
      break;
    case K_REF:
      new (&u_.r) ObjectRef(other.u_.r);
      break;
    }
  }

  Kind kind_;
  Storage u_;
};

} // namespace s5run
