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

#include "s5run_variable.h"

#include "nlohmann/json.hpp"

#include <cstdio>
#include <cstdlib>
#include <new>
#include <sstream>
#include <stdexcept>

namespace s5run {

// -----------------------------------------------------------------------
// object_ref
// -----------------------------------------------------------------------

object_ref::object_ref() : this_id(0) {
}

object_ref::object_ref(int id, const std::string &t) : this_id(id), type(t) {
}

// -----------------------------------------------------------------------
// variable: construction / copying / destruction
// -----------------------------------------------------------------------

variable::Storage::Storage() {
}

variable::Storage::~Storage() {
}

variable::variable() : kind_(K_NULL) {
}

variable::variable(std::nullptr_t) : kind_(K_NULL) {
}

variable::variable(bool b) : kind_(K_BOOL) {
  u_.b = b;
}

variable::variable(int32_t i) : kind_(K_INT) {
  u_.i = static_cast<int64_t>(i);
}

variable::variable(int64_t i) : kind_(K_INT) {
  u_.i = i;
}

variable::variable(uint32_t i) : kind_(K_INT) {
  u_.i = static_cast<int64_t>(i);
}

variable::variable(uint64_t i) : kind_(K_INT) {
  u_.i = static_cast<int64_t>(i);
}

variable::variable(double d) : kind_(K_DOUBLE) {
  u_.d = d;
}

variable::variable(const char *s) : kind_(K_STRING) {
  new (&u_.s) std::string(s);
}

variable::variable(const std::string &s) : kind_(K_STRING) {
  new (&u_.s) std::string(s);
}

variable::variable(const variable_array &a) : kind_(K_ARRAY) {
  new (&u_.a) variable_array(a);
}

variable::variable(const variable_object &o) : kind_(K_OBJECT) {
  new (&u_.o) variable_object(o);
}

variable::variable(const object_ref &r) : kind_(K_REF) {
  new (&u_.r) object_ref(r);
}

variable::variable(const variable &other) : kind_(other.kind_) {
  copy_from(other);
}

variable::~variable() {
  destroy();
}

variable &variable::operator=(const variable &other) {
  if (this != &other) {
    destroy();
    kind_ = other.kind_;
    copy_from(other);
  }
  return *this;
}

// -----------------------------------------------------------------------
// variable: introspection / accessors
// -----------------------------------------------------------------------

variable::Kind variable::kind() const {
  return kind_;
}

bool variable::is_null() const {
  return kind_ == K_NULL;
}

bool variable::is_bool() const {
  return kind_ == K_BOOL;
}

bool variable::is_int() const {
  return kind_ == K_INT;
}

bool variable::is_double() const {
  return kind_ == K_DOUBLE;
}

bool variable::is_number() const {
  return kind_ == K_INT || kind_ == K_DOUBLE;
}

bool variable::is_string() const {
  return kind_ == K_STRING;
}

bool variable::is_array() const {
  return kind_ == K_ARRAY;
}

bool variable::is_object() const {
  return kind_ == K_OBJECT;
}

bool variable::is_ref() const {
  return kind_ == K_REF;
}

bool variable::as_bool() const {
  return u_.b;
}

int64_t variable::as_int() const {
  return u_.i;
}

double variable::as_double() const {
  return (kind_ == K_INT) ? static_cast<double>(u_.i) : u_.d;
}

const std::string &variable::as_string() const {
  return u_.s;
}

const variable_array &variable::as_array() const {
  return u_.a;
}

const variable_object &variable::as_object() const {
  return u_.o;
}

const object_ref &variable::as_ref() const {
  return u_.r;
}

variable_array &variable::array() {
  return u_.a;
}

variable_object &variable::object() {
  return u_.o;
}

std::string &variable::string() {
  return u_.s;
}

// -----------------------------------------------------------------------
// variable: destruction / copying internals
// -----------------------------------------------------------------------

namespace {

// Calls the real destructor of a union member; the pseudo-destructor
// syntax u_.s.~string() is not portable across compilers.
template <typename T>
void s5run_destroy_at(T *p) {
  p->~T();
}

} // namespace

void variable::destroy() {
  switch (kind_) {
  case K_STRING:
    s5run_destroy_at(&u_.s);
    break;
  case K_ARRAY:
    s5run_destroy_at(&u_.a);
    break;
  case K_OBJECT:
    s5run_destroy_at(&u_.o);
    break;
  case K_REF:
    s5run_destroy_at(&u_.r);
    break;
  case K_NULL:
  case K_BOOL:
  case K_INT:
  case K_DOUBLE:
  default:
    break;
  }
  kind_ = K_NULL;
}

void variable::copy_from(const variable &other) {
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
    new (&u_.r) object_ref(other.u_.r);
    break;
  }
}

// -----------------------------------------------------------------------
// variable: parse_script_arg
// -----------------------------------------------------------------------

variable variable::parse_script_arg(const std::string &raw) {
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

namespace {

// Split text into lines, trimming trailing \r and skipping empty lines.
std::vector<std::string> split_lines(const std::string &text) {
  std::vector<std::string> out;
  std::istringstream iss(text);
  std::string line;
  while (std::getline(iss, line)) {
    if (!line.empty() && line[line.size() - 1] == '\r')
      line.erase(line.size() - 1);
    if (!line.empty())
      out.push_back(line);
  }
  return out;
}

std::string trim_spaces(const std::string &s) {
  size_t b = s.find_first_not_of(" \t");
  if (b == std::string::npos)
    return "";
  size_t e = s.find_last_not_of(" \t");
  return s.substr(b, e - b + 1);
}

// JSON-escaped quoted string (via nlohmann).
std::string json_quote(const std::string &s) {
  return nlohmann::json(s).dump();
}

// Parse a JSON-escaped quoted string (via nlohmann).
std::string json_unquote(const std::string &quoted) {
  nlohmann::json j = nlohmann::json::parse(quoted);
  if (!j.is_string())
    throw std::runtime_error("s5run variable: expected a quoted string");
  return j.get<std::string>();
}

// ---- structured (s5run) format ----

void append_debug(const variable &v, std::string &out, int indent) {
  char buf[64];
  std::string pad(indent, ' ');
  std::string pad_in(indent + 2, ' ');
  switch (v.kind()) {
  case variable::K_NULL:
    out += "K_NULL\n";
    break;
  case variable::K_BOOL:
    out += v.as_bool() ? "K_BOOL true\n" : "K_BOOL false\n";
    break;
  case variable::K_INT:
    snprintf(buf, sizeof(buf), "K_INT %lld\n", (long long)v.as_int());
    out += buf;
    break;
  case variable::K_DOUBLE:
    snprintf(buf, sizeof(buf), "K_DOUBLE %g\n", v.as_double());
    out += buf;
    break;
  case variable::K_STRING:
    out += "K_STRING " + json_quote(v.as_string()) + "\n";
    break;
  case variable::K_ARRAY:
    out += "K_ARRAY\n";
    out += pad + "BEGIN\n";
    for (const auto &e : v.as_array()) {
      out += pad_in;
      append_debug(e, out, indent + 2);
    }
    out += pad + "END\n";
    break;
  case variable::K_OBJECT:
    // std::map iterates sorted by name
    out += "K_OBJECT\n";
    out += pad + "BEGIN\n";
    for (const auto &kv : v.as_object()) {
      out += pad_in + kv.first + " ";
      append_debug(kv.second, out, indent + 2);
    }
    out += pad + "END\n";
    break;
  case variable::K_REF:
    out += "K_REF " + std::to_string(v.as_ref().this_id) + " "
           + json_quote(v.as_ref().type) + "\n";
    break;
  }
}

variable parse_scalar_line(const std::string &line) {
  if (line == "K_NULL")
    return variable();
  if (line == "K_BOOL true")
    return variable(true);
  if (line == "K_BOOL false")
    return variable(false);
  if (line.rfind("K_INT ", 0) == 0) {
    char *end = nullptr;
    long long v = std::strtoll(line.c_str() + 6, &end, 10);
    if (!end || *end != '\0')
      throw std::runtime_error("s5run variable: bad K_INT value: " + line);
    return variable((int64_t)v);
  }
  if (line.rfind("K_DOUBLE ", 0) == 0) {
    char *end = nullptr;
    double d = std::strtod(line.c_str() + 9, &end);
    if (!end || *end != '\0')
      throw std::runtime_error("s5run variable: bad K_DOUBLE value: " + line);
    return variable(d);
  }
  if (line.rfind("K_STRING ", 0) == 0)
    return variable(json_unquote(line.substr(9)));
  if (line.rfind("K_REF ", 0) == 0) {
    size_t sp = line.find(' ', 6);
    if (sp == std::string::npos)
      throw std::runtime_error("s5run variable: bad K_REF value: " + line);
    char *end = nullptr;
    long long id = std::strtoll(line.c_str() + 6, &end, 10);
    if (!end || end != line.c_str() + sp)
      throw std::runtime_error("s5run variable: bad K_REF id: " + line);
    return variable(object_ref((int)id, json_unquote(line.substr(sp + 1))));
  }
  throw std::runtime_error("s5run variable: unknown value line: " + line);
}

// lines[pos] is expected to be the BEGIN of a container described by header.
variable parse_container(const std::string &header, const std::vector<std::string> &lines,
                         size_t &pos) {
  if (pos >= lines.size() || trim_spaces(lines[pos]) != "BEGIN")
    throw std::runtime_error("s5run variable: expected BEGIN");
  pos++;

  if (header == "K_OBJECT") {
    variable_object out;
    while (pos < lines.size() && trim_spaces(lines[pos]) != "END") {
      std::string fline = trim_spaces(lines[pos++]);
      size_t sp = fline.find(' ');
      if (sp == std::string::npos)
        throw std::runtime_error("s5run variable: bad field line: " + fline);
      std::string key = fline.substr(0, sp);
      std::string rest = fline.substr(sp + 1);
      if (rest == "K_OBJECT" || rest == "K_ARRAY")
        out[key] = parse_container(rest, lines, pos);
      else
        out[key] = parse_scalar_line(rest);
    }
    if (pos >= lines.size())
      throw std::runtime_error("s5run variable: missing END");
    pos++; // END
    return variable(out);
  }

  // K_ARRAY
  variable_array out;
  while (pos < lines.size() && trim_spaces(lines[pos]) != "END") {
    std::string eline = trim_spaces(lines[pos++]);
    if (eline == "K_OBJECT" || eline == "K_ARRAY")
      out.push_back(parse_container(eline, lines, pos));
    else
      out.push_back(parse_scalar_line(eline));
  }
  if (pos >= lines.size())
    throw std::runtime_error("s5run variable: missing END");
  pos++; // END
  return variable(out);
}

// ---- JSON (nlohmann) ----

nlohmann::json variable_to_json(const variable &v) {
  switch (v.kind()) {
  case variable::K_NULL:
    return nullptr;
  case variable::K_BOOL:
    return v.as_bool();
  case variable::K_INT:
    return v.as_int();
  case variable::K_DOUBLE:
    return v.as_double();
  case variable::K_STRING:
    return v.as_string();
  case variable::K_ARRAY: {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &e : v.as_array())
      arr.push_back(variable_to_json(e));
    return arr;
  }
  case variable::K_OBJECT: {
    nlohmann::json obj = nlohmann::json::object();
    for (const auto &kv : v.as_object())
      obj[kv.first] = variable_to_json(kv.second);
    return obj;
  }
  case variable::K_REF:
    return nlohmann::json{{"__s5run_ref__", {{"this_id", v.as_ref().this_id},
                                             {"type", v.as_ref().type}}}};
  }
  throw std::runtime_error("s5run variable: unknown kind");
}

variable json_to_variable(const nlohmann::json &j) {
  if (j.is_null())
    return variable();
  if (j.is_boolean())
    return variable(j.get<bool>());
  if (j.is_number_integer())
    return variable(j.get<int64_t>());
  if (j.is_number_float())
    return variable(j.get<double>());
  if (j.is_string())
    return variable(j.get<std::string>());
  if (j.is_array()) {
    variable_array out;
    for (const auto &e : j)
      out.push_back(json_to_variable(e));
    return variable(out);
  }
  if (j.is_object()) {
    // reference marker?
    if (j.size() == 1 && j.contains("__s5run_ref__")) {
      const auto &r = j["__s5run_ref__"];
      if (!r.is_object() || !r.contains("this_id") || !r.contains("type"))
        throw std::runtime_error("s5run variable: bad __s5run_ref__");
      return variable(object_ref(r["this_id"].get<int>(), r["type"].get<std::string>()));
    }
    variable_object out;
    for (auto it = j.begin(); it != j.end(); ++it)
      out[it.key()] = json_to_variable(it.value());
    return variable(out);
  }
  throw std::runtime_error("s5run variable: unsupported json value");
}

} // namespace

std::string variable::to_string_s5run() const {
  std::string out;
  append_debug(*this, out, 0);
  if (!out.empty() && out[out.size() - 1] == '\n')
    out.erase(out.size() - 1);
  return out;
}

std::string variable::to_string_json() const {
  return variable_to_json(*this).dump();
}

variable variable::from_string_s5run(const std::string &text) {
  std::vector<std::string> lines = split_lines(text);
  if (lines.empty())
    throw std::runtime_error("s5run variable: empty input");
  size_t pos = 0;
  std::string header = trim_spaces(lines[pos++]);
  if (header == "K_OBJECT" || header == "K_ARRAY")
    return parse_container(header, lines, pos);
  if (pos != lines.size())
    throw std::runtime_error("s5run variable: trailing lines after a scalar");
  return parse_scalar_line(header);
}

variable variable::from_string_json(const std::string &text) {
  nlohmann::json j = nlohmann::json::parse(text);
  return json_to_variable(j);
}

} // namespace s5run
