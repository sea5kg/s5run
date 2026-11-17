#pragma once
#include <cctype>
#include <cstdio>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace json {

struct Value;
using Array = std::vector<Value>;
using Object = std::map<std::string, Value>;

struct Value {
  std::variant<std::nullptr_t, bool, double, std::string, Array, Object> v;

  Value() : v(nullptr) {
  }
  Value(bool b) : v(b) {
  }
  Value(double d) : v(d) {
  }
  Value(int i) : v(static_cast<double>(i)) {
  }
  Value(std::string s) : v(std::move(s)) {
  }
  Value(Array a) : v(std::move(a)) {
  }
  Value(Object o) : v(std::move(o)) {
  }

  bool is_null() const {
    return std::holds_alternative<std::nullptr_t>(v);
  }
  bool is_bool() const {
    return std::holds_alternative<bool>(v);
  }
  bool is_num() const {
    return std::holds_alternative<double>(v);
  }
  bool is_str() const {
    return std::holds_alternative<std::string>(v);
  }
  bool is_arr() const {
    return std::holds_alternative<Array>(v);
  }
  bool is_obj() const {
    return std::holds_alternative<Object>(v);
  }

  bool as_bool() const {
    return std::get<bool>(v);
  }
  double as_num() const {
    return std::get<double>(v);
  }
  const std::string &as_str() const {
    return std::get<std::string>(v);
  }
  const Array &as_arr() const {
    return std::get<Array>(v);
  }
  const Object &as_obj() const {
    return std::get<Object>(v);
  }
  Array &arr() {
    return std::get<Array>(v);
  }
  Object &obj() {
    return std::get<Object>(v);
  }

  const Value &operator[](const std::string &k) const {
    return as_obj().at(k);
  }
};

namespace detail {

struct Parser {
  const std::string &s;
  size_t i = 0;
  explicit Parser(const std::string &src) : s(src) {
  }

  void ws() {
    while (i < s.size() && std::isspace((unsigned char)s[i]))
      ++i;
  }
  char peek() {
    ws();
    if (i >= s.size())
      throw std::runtime_error("eof");
    return s[i];
  }
  char get() {
    ws();
    if (i >= s.size())
      throw std::runtime_error("eof");
    return s[i++];
  }
  void expect(char c) {
    if (get() != c)
      throw std::runtime_error(std::string("expect ") + c);
  }

  Value parse() {
    char c = peek();
    if (c == '{')
      return parse_obj();
    if (c == '[')
      return parse_arr();
    if (c == '"')
      return Value(parse_str());
    if (c == 't') {
      i += 4;
      return Value(true);
    }
    if (c == 'f') {
      i += 5;
      return Value(false);
    }
    if (c == 'n') {
      i += 4;
      return Value();
    }
    return parse_num();
  }

  Value parse_obj() {
    Object o;
    expect('{');
    if (peek() == '}') {
      ++i;
      return Value(std::move(o));
    }
    while (true) {
      std::string k = parse_str();
      expect(':');
      o[k] = parse();
      if (peek() == ',') {
        ++i;
        continue;
      }
      expect('}');
      break;
    }
    return Value(std::move(o));
  }

  Value parse_arr() {
    Array a;
    expect('[');
    if (peek() == ']') {
      ++i;
      return Value(std::move(a));
    }
    while (true) {
      a.push_back(parse());
      if (peek() == ',') {
        ++i;
        continue;
      }
      expect(']');
      break;
    }
    return Value(std::move(a));
  }

  std::string parse_str() {
    expect('"');
    std::string r;
    while (true) {
      if (i >= s.size())
        throw std::runtime_error("eof in str");
      char c = s[i++];
      if (c == '"')
        break;
      if (c == '\\') {
        if (i >= s.size())
          throw std::runtime_error("eof in escape");
        char e = s[i++];
        switch (e) {
        case 'n':
          r.push_back('\n');
          break;
        case 't':
          r.push_back('\t');
          break;
        case 'r':
          r.push_back('\r');
          break;
        case 'b':
          r.push_back('\b');
          break;
        case 'f':
          r.push_back('\f');
          break;
        case '"':
          r.push_back('"');
          break;
        case '\\':
          r.push_back('\\');
          break;
        case '/':
          r.push_back('/');
          break;
        case 'u': {
          if (i + 4 > s.size())
            throw std::runtime_error("bad \\u");
          std::string hex = s.substr(i, 4);
          i += 4;
          unsigned cp = std::stoul(hex, nullptr, 16);
          if (cp < 0x80)
            r.push_back(char(cp));
          else if (cp < 0x800) {
            r.push_back(char(0xC0 | (cp >> 6)));
            r.push_back(char(0x80 | (cp & 0x3F)));
          } else {
            r.push_back(char(0xE0 | (cp >> 12)));
            r.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
            r.push_back(char(0x80 | (cp & 0x3F)));
          }
          break;
        }
        default:
          throw std::runtime_error("bad escape");
        }
      } else {
        r.push_back(c);
      }
    }
    return r;
  }

  Value parse_num() {
    size_t start = i;
    if (s[i] == '-')
      ++i;
    while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' ||
                            s[i] == '+' || s[i] == '-'))
      ++i;
    return Value(std::stod(s.substr(start, i - start)));
  }
};

inline void escape(std::ostream &os, const std::string &s) {
  os << '"';
  for (char c : s) {
    switch (c) {
    case '"':
      os << "\\\"";
      break;
    case '\\':
      os << "\\\\";
      break;
    case '\n':
      os << "\\n";
      break;
    case '\t':
      os << "\\t";
      break;
    case '\r':
      os << "\\r";
      break;
    default:
      if ((unsigned char)c < 0x20) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
        os << buf;
      } else
        os << c;
    }
  }
  os << '"';
}

} // namespace detail

inline Value parse(const std::string &s) {
  detail::Parser p(s);
  return p.parse();
}

inline void dump(std::ostream &os, const Value &v);

inline void dump(std::ostream &os, const Value &v) {
  if (v.is_null()) {
    os << "null";
    return;
  }
  if (v.is_bool()) {
    os << (v.as_bool() ? "true" : "false");
    return;
  }
  if (v.is_num()) {
    double d = v.as_num();
    if (d == (long long)d)
      os << (long long)d;
    else
      os << d;
    return;
  }
  if (v.is_str()) {
    detail::escape(os, v.as_str());
    return;
  }
  if (v.is_arr()) {
    os << '[';
    const auto &a = v.as_arr();
    for (size_t i = 0; i < a.size(); ++i) {
      if (i)
        os << ',';
      dump(os, a[i]);
    }
    os << ']';
    return;
  }
  os << '{';
  bool first = true;
  for (const auto &[k, val] : v.as_obj()) {
    if (!first)
      os << ',';
    first = false;
    detail::escape(os, k);
    os << ':';
    dump(os, val);
  }
  os << '}';
}

inline std::string dump(const Value &v) {
  std::ostringstream os;
  dump(os, v);
  return os.str();
}

} // namespace json