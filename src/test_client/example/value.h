#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace example {

class Value;
typedef std::vector<Value> ValueArray;
typedef std::map<std::string, Value> ValueObject;

struct ObjectRef {
  int this_id;
  std::string type;

  ObjectRef() : this_id(0) {
  }
  ObjectRef(int id, const std::string &t) : this_id(id), type(t) {
  }
};

class Value {
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

  Value() : kind_(K_NULL) {
  }
  Value(std::nullptr_t) : kind_(K_NULL) {
  }

  Value(bool b) : kind_(K_BOOL) {
    u_.b = b;
  }

  Value(int32_t i) : kind_(K_INT) {
    u_.i = static_cast<int64_t>(i);
  }
  Value(int64_t i) : kind_(K_INT) {
    u_.i = i;
  }
  Value(uint32_t i) : kind_(K_INT) {
    u_.i = static_cast<int64_t>(i);
  }
  Value(uint64_t i) : kind_(K_INT) {
    u_.i = static_cast<int64_t>(i);
  }

  Value(double d) : kind_(K_DOUBLE) {
    u_.d = d;
  }

  Value(const char *s) : kind_(K_STRING) {
    new (&u_.s) std::string(s);
  }
  Value(const std::string &s) : kind_(K_STRING) {
    new (&u_.s) std::string(s);
  }

  Value(const ValueArray &a) : kind_(K_ARRAY) {
    new (&u_.a) ValueArray(a);
  }
  Value(const ValueObject &o) : kind_(K_OBJECT) {
    new (&u_.o) ValueObject(o);
  }
  Value(const ObjectRef &r) : kind_(K_REF) {
    new (&u_.r) ObjectRef(r);
  }

  Value(const Value &other) : kind_(other.kind_) {
    copy_from(other);
  }

  ~Value() {
    destroy();
  }

  // ---- assignment ----

  Value &operator=(const Value &other) {
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
  const ValueArray &as_array() const {
    return u_.a;
  }
  const ValueObject &as_object() const {
    return u_.o;
  }
  const ObjectRef &as_ref() const {
    return u_.r;
  }

  ValueArray &array() {
    return u_.a;
  }
  ValueObject &object() {
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
    ValueArray a;
    ValueObject o;
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
      u_.a.~ValueArray();
      break; // typedef
    case K_OBJECT:
      u_.o.~ValueObject();
      break; // typedef
    case K_REF:
      u_.r.~ObjectRef();
      break;
    }
    kind_ = K_NULL;
  }

  void copy_from(const Value &other) {
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
      new (&u_.a) ValueArray(other.u_.a);
      break;
    case K_OBJECT:
      new (&u_.o) ValueObject(other.u_.o);
      break;
    case K_REF:
      new (&u_.r) ObjectRef(other.u_.r);
      break;
    }
  }

  Kind kind_;
  Storage u_;
};

} // namespace example