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

#include "engine/s5run_value.h"
#include "server/s5run_value_wire.h"

#include <cassert>
#include <cstdio>

using s5run::ObjectRef;
using s5run::Value;
using s5run::ValueArray;
using s5run::ValueObject;

static s5run::Value roundtrip(const s5run::Value &v) {
  std::vector<uint8_t> buf = s5run::value_encode(v);
  return s5run::value_decode(buf.data(), buf.size());
}

int main() {
  // null
  assert(roundtrip(Value()).is_null());

  // bool
  assert(roundtrip(Value(true)).as_bool() == true);
  assert(roundtrip(Value(false)).as_bool() == false);

  // ints — all sizes
  assert(roundtrip(Value((int64_t)0)).as_int() == 0);
  assert(roundtrip(Value((int64_t)127)).as_int() == 127);
  assert(roundtrip(Value((int64_t)-128)).as_int() == -128);
  assert(roundtrip(Value((int64_t)32767)).as_int() == 32767);
  assert(roundtrip(Value((int64_t)-32768)).as_int() == -32768);
  assert(roundtrip(Value((int64_t)2147483647)).as_int() == 2147483647);
  assert(roundtrip(Value((int64_t)-2147483648LL)).as_int() == -2147483648LL);
  assert(roundtrip(Value((int64_t)9000000000LL)).as_int() == 9000000000LL);

  // double
  assert(roundtrip(Value(3.14)).as_double() == 3.14);

  // string — short, medium, long
  assert(roundtrip(Value("")).as_string() == "");
  assert(roundtrip(Value("hello")).as_string() == "hello");
  std::string big(300, 'x');
  assert(roundtrip(Value(big)).as_string() == big);

  // array
  ValueArray arr;
  arr.push_back(Value((int64_t)1));
  arr.push_back(Value("two"));
  arr.push_back(Value(3.0));
  Value ra = roundtrip(Value(arr));
  assert(ra.is_array());
  assert(ra.as_array().size() == 3);
  assert(ra.as_array()[0].as_int() == 1);
  assert(ra.as_array()[1].as_string() == "two");
  assert(ra.as_array()[2].as_double() == 3.0);

  // object
  ValueObject obj;
  obj["x"] = Value((int64_t)42);
  obj["name"] = Value("file.txt");
  Value ro = roundtrip(Value(obj));
  assert(ro.is_object());
  assert(ro.as_object().at("x").as_int() == 42);
  assert(ro.as_object().at("name").as_string() == "file.txt");

  // ref
  Value rr = roundtrip(Value(s5run::ObjectRef(123, "File")));
  assert(rr.is_ref());
  assert(rr.as_ref().this_id == 123);
  assert(rr.as_ref().type == "File");

  // nested
  ValueObject inner;
  inner["size"] = Value((int64_t)1024);
  ValueArray nested;
  nested.push_back(Value(inner));
  nested.push_back(Value("tail"));
  Value rn = roundtrip(Value(nested));
  assert(rn.is_array());
  assert(rn.as_array()[0].is_object());
  assert(rn.as_array()[0].as_object().at("size").as_int() == 1024);
  assert(rn.as_array()[1].as_string() == "tail");

  std::printf("value_wire: all tests passed\n");
  return 0;
}