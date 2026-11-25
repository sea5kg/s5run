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

#include "s5run_variable_wire.h"

#include <cstring>

namespace s5run {

void value_to_wire(const variable &v, wire::Writer &w) {
  switch (v.kind()) {
  case variable::K_NULL:
    w.put_u8(wire::T_UNDEFINED);
    return;
  case variable::K_BOOL:
    w.put_u8(wire::T_BOOL);
    w.put_u8(v.as_bool() ? 1 : 0);
    return;
  case variable::K_INT: {
    int64_t i = v.as_int();
    if (i >= INT8_MIN && i <= INT8_MAX) {
      w.put_u8(wire::T_INT8);
      w.put_i8((int8_t)i);
    } else if (i >= INT16_MIN && i <= INT16_MAX) {
      w.put_u8(wire::T_INT16);
      w.put_i16((int16_t)i);
    } else if (i >= INT32_MIN && i <= INT32_MAX) {
      w.put_u8(wire::T_INT32);
      w.put_i32((int32_t)i);
    } else {
      w.put_u8(wire::T_INT64);
      w.put_i64(i);
    }
    return;
  }
  case variable::K_DOUBLE:
    w.put_u8(wire::T_FLOAT64);
    w.put_f64(v.as_double());
    return;
  case variable::K_STRING:
    w.put_u8(wire::T_STRING);
    w.put_string(v.as_string());
    return;
  case variable::K_ARRAY: {
    w.put_u8(wire::T_ARRAY);
    const auto &a = v.as_array();
    w.put_u32((uint32_t)a.size());
    for (const auto &e : a)
      value_to_wire(e, w);
    return;
  }
  case variable::K_OBJECT: {
    w.put_u8(wire::T_OBJECT);
    const auto &o = v.as_object();
    w.put_u32((uint32_t)o.size());
    for (const auto &kv : o) {
      w.put_u8(wire::T_STRING);
      w.put_string(kv.first);
      value_to_wire(kv.second, w);
    }
    return;
  }
  case variable::K_REF:
    w.put_u8(wire::T_REF);
    w.put_i32(v.as_ref().this_id);
    w.put_string(v.as_ref().type);
    return;
  }
}

variable value_from_wire(wire::Reader &r) {
  uint8_t tag = r.get_u8();
  switch (tag) {
  case wire::T_UNDEFINED:
    return variable();
  case wire::T_BOOL:
    return variable(r.get_u8() != 0);
  case wire::T_INT8:
    return variable((int64_t)r.get_i8());
  case wire::T_INT16:
    return variable((int64_t)r.get_i16());
  case wire::T_INT32:
    return variable((int64_t)r.get_i32());
  case wire::T_INT64:
    return variable((int64_t)r.get_i64());
  case wire::T_UINT8:
    return variable((int64_t)r.get_u8());
  case wire::T_UINT16:
    return variable((int64_t)r.get_u16());
  case wire::T_UINT32:
    return variable((int64_t)r.get_u32());
  case wire::T_UINT64:
    return variable((int64_t)r.get_u64());
  case wire::T_FLOAT32:
    return variable((double)r.get_f32());
  case wire::T_FLOAT64:
    return variable(r.get_f64());
  case wire::T_STRING:
    return variable(r.get_string());
  case wire::T_ARRAY: {
    uint32_t n = r.get_u32();
    variable_array a;
    a.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
      a.push_back(value_from_wire(r));
    return variable(a);
  }
  case wire::T_OBJECT: {
    uint32_t n = r.get_u32();
    variable_object o;
    for (uint32_t i = 0; i < n; ++i) {
      uint8_t k_tag = r.get_u8();
      if (k_tag != wire::T_STRING)
        throw std::runtime_error("wire: object key must be a string");
      std::string key = r.get_string();
      o[key] = value_from_wire(r);
    }
    return variable(o);
  }
  case wire::T_REF: {
    int32_t this_id = r.get_i32();
    std::string type = r.get_string();
    return variable(ObjectRef(this_id, type));
  }
  default:
    throw std::runtime_error("wire: unknown value tag");
  }
}

} // namespace s5run