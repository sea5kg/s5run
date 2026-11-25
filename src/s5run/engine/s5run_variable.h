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
#include <map>
#include <string>
#include <vector>

namespace s5run {

class variable;
typedef std::vector<variable> variable_array;
typedef std::map<std::string, variable> variable_object;

struct object_ref {
  int this_id;
  std::string type;

  object_ref();
  object_ref(int id, const std::string &t);
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

  variable();
  variable(std::nullptr_t);
  variable(bool b);
  variable(int32_t i);
  variable(int64_t i);
  variable(uint32_t i);
  variable(uint64_t i);
  variable(double d);
  variable(const char *s);
  variable(const std::string &s);
  variable(const variable_array &a);
  variable(const variable_object &o);
  variable(const object_ref &r);
  variable(const variable &other);
  ~variable();

  // ---- assignment ----

  variable &operator=(const variable &other);

  // Parse a raw command-line argument into a typed variable:
  // int -> bool -> double -> string.
  static variable parse_script_arg(const std::string &raw);

  // ---- introspection ----

  Kind kind() const;
  bool is_null() const;
  bool is_bool() const;
  bool is_int() const;
  bool is_double() const;
  bool is_number() const;
  bool is_string() const;
  bool is_array() const;
  bool is_object() const;
  bool is_ref() const;

  // ---- accessors (caller must check kind first) ----

  bool as_bool() const;
  int64_t as_int() const;
  double as_double() const;
  const std::string &as_string() const;
  const variable_array &as_array() const;
  const variable_object &as_object() const;
  const object_ref &as_ref() const;

  variable_array &array();
  variable_object &object();
  std::string &string();

  // ---- serialization ----

  // Structured multi-line representation, handy for tests and debugging:
  //   K_INT 1024
  //   K_STRING "text\nline"
  //   K_OBJECT
  //   BEGIN
  //     size K_INT 1024
  //   END
  // Object fields are sorted by name.
  std::string to_string_s5run() const;

  // JSON representation (nlohmann/json). A reference is encoded as
  // {"__s5run_ref__": {"this_id": N, "type": "..."}}.
  std::string to_string_json() const;

  // Parse the structured format back; throws std::runtime_error
  // on a malformed input.
  static variable from_string_s5run(const std::string &text);

  // Parse JSON back (nlohmann/json); throws on invalid JSON or on
  // non-representable values.
  static variable from_string_json(const std::string &text);

private:
  union Storage {
    bool b;
    int64_t i;
    double d;
    std::string s;
    variable_array a;
    variable_object o;
    object_ref r;

    Storage();
    ~Storage();
  };

  void destroy();
  void copy_from(const variable &other);

  Kind kind_;
  Storage u_;
};

} // namespace s5run
