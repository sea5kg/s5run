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

#include "engine/s5run_variable.h"
#include "server/s5run_variable_wire.h"

#include <cassert>
#include <cstdio>

using s5run::ObjectRef;
using s5run::variable;
using s5run::variable_array;
using s5run::variable_object;

static s5run::variable roundtrip(const s5run::variable &v) {
  std::vector<uint8_t> buf = s5run::value_encode(v);
  return s5run::value_decode(buf.data(), buf.size());
}

int main() {
  // null
  assert(roundtrip(variable()).is_null());

  // bool
  assert(roundtrip(variable(true)).as_bool() == true);
  assert(roundtrip(variable(false)).as_bool() == false);

  // ints — all sizes
  assert(roundtrip(variable((int64_t)0)).as_int() == 0);
  assert(roundtrip(variable((int64_t)127)).as_int() == 127);
  assert(roundtrip(variable((int64_t)-128)).as_int() == -128);
  assert(roundtrip(variable((int64_t)32767)).as_int() == 32767);
  assert(roundtrip(variable((int64_t)-32768)).as_int() == -32768);
  assert(roundtrip(variable((int64_t)2147483647)).as_int() == 2147483647);
  assert(roundtrip(variable((int64_t)-2147483648LL)).as_int() == -2147483648LL);
  assert(roundtrip(variable((int64_t)9000000000LL)).as_int() == 9000000000LL);

  // double
  assert(roundtrip(variable(3.14)).as_double() == 3.14);

  // string — short, medium, long
  assert(roundtrip(variable("")).as_string() == "");
  assert(roundtrip(variable("hello")).as_string() == "hello");
  std::string big(300, 'x');
  assert(roundtrip(variable(big)).as_string() == big);

  // array
  variable_array arr;
  arr.push_back(variable((int64_t)1));
  arr.push_back(variable("two"));
  arr.push_back(variable(3.0));
  variable ra = roundtrip(variable(arr));
  assert(ra.is_array());
  assert(ra.as_array().size() == 3);
  assert(ra.as_array()[0].as_int() == 1);
  assert(ra.as_array()[1].as_string() == "two");
  assert(ra.as_array()[2].as_double() == 3.0);

  // object
  variable_object obj;
  obj["x"] = variable((int64_t)42);
  obj["name"] = variable("file.txt");
  variable ro = roundtrip(variable(obj));
  assert(ro.is_object());
  assert(ro.as_object().at("x").as_int() == 42);
  assert(ro.as_object().at("name").as_string() == "file.txt");

  // ref
  variable rr = roundtrip(variable(s5run::ObjectRef(123, "File")));
  assert(rr.is_ref());
  assert(rr.as_ref().this_id == 123);
  assert(rr.as_ref().type == "File");

  // nested
  variable_object inner;
  inner["size"] = variable((int64_t)1024);
  variable_array nested;
  nested.push_back(variable(inner));
  nested.push_back(variable("tail"));
  variable rn = roundtrip(variable(nested));
  assert(rn.is_array());
  assert(rn.as_array()[0].is_object());
  assert(rn.as_array()[0].as_object().at("size").as_int() == 1024);
  assert(rn.as_array()[1].as_string() == "tail");

  std::printf("value_wire: all tests passed\n");
  return 0;
}