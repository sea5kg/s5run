// Unit test for variable::to_string_json() / from_string_json().

#include "engine/s5run_variable.h"

#include "nlohmann/json.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>

using s5run::ObjectRef;
using s5run::variable;
using s5run::variable_array;
using s5run::variable_object;

#define CHECK(cond) do { \
    if (!(cond)) { \
      std::fprintf(stderr, "CHECK failed: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      throw std::runtime_error("check failed: " #cond); \
    } \
  } while (0)

namespace {

// Roundtrip via JSON: v -> json text -> variable -> s5run text
// must equal v's s5run text.
void check_json_roundtrip(const variable &v) {
  std::string json_text = v.to_string_json();
  // must be valid JSON
  nlohmann::json parsed = nlohmann::json::parse(json_text);
  (void)parsed;
  variable back = variable::from_string_json(json_text);
  if (back.to_string_s5run() != v.to_string_s5run())
    throw std::runtime_error("json roundtrip mismatch:\n s5run: "
                             + v.to_string_s5run() + "\n back: " + back.to_string_s5run());
}

void test_scalars() {
  std::fprintf(stderr, "== variable json: scalars\n");

  CHECK(variable().to_string_json() == "null");
  CHECK(variable(true).to_string_json() == "true");
  CHECK(variable(false).to_string_json() == "false");
  CHECK(variable((int64_t)1024).to_string_json() == "1024");
  CHECK(variable(3.5).to_string_json() == "3.5");
  CHECK(variable(std::string("hello")).to_string_json() == "\"hello\"");
  CHECK(variable(std::string("esc \" \\ \n")).to_string_json()
        == "\"esc \\\" \\\\ \\n\"");

  check_json_roundtrip(variable());
  check_json_roundtrip(variable(true));
  check_json_roundtrip(variable((int64_t)1737000000));
  check_json_roundtrip(variable(3.5));
  check_json_roundtrip(variable(std::string("hello world")));

  std::fprintf(stderr, "  PASS\n");
}

void test_containers() {
  std::fprintf(stderr, "== variable json: arrays and objects\n");

  variable_array arr;
  arr.push_back(variable((int64_t)1));
  arr.push_back(variable(std::string("two")));
  arr.push_back(variable(3.5));
  variable varr(arr);
  CHECK(varr.to_string_json() == "[1,\"two\",3.5]");
  check_json_roundtrip(varr);

  variable_object obj;
  obj["size"] = variable((int64_t)1024);
  obj["ok"] = variable(true);
  variable vobj(obj);
  CHECK(vobj.to_string_json() == "{\"ok\":true,\"size\":1024}"); // nlohmann sorts keys
  check_json_roundtrip(vobj);

  // nested
  variable_object inner;
  inner["x"] = variable((int64_t)1);
  variable_array list;
  list.push_back(variable(inner));
  list.push_back(variable((int64_t)2));
  variable_object outer;
  outer["list"] = variable(list);
  outer["flag"] = variable(false);
  variable vnested(outer);
  check_json_roundtrip(vnested);

  std::fprintf(stderr, "  PASS\n");
}

void test_ref() {
  std::fprintf(stderr, "== variable json: refs\n");

  variable v(ObjectRef(3, "File"));
  CHECK(v.to_string_json() == "{\"__s5run_ref__\":{\"this_id\":3,\"type\":\"File\"}}");
  check_json_roundtrip(v);

  // nested ref inside an object
  variable_object obj;
  obj["f"] = variable(ObjectRef(7, "File"));
  check_json_roundtrip(variable(obj));

  std::fprintf(stderr, "  PASS\n");
}

void test_from_json() {
  std::fprintf(stderr, "== variable json: from_string_json\n");

  variable v = variable::from_string_json("{\"a\": [1, 2.5, \"three\", true, null]}");
  CHECK(v.is_object());
  CHECK(v.as_object().at("a").is_array());
  const variable_array &a = v.as_object().at("a").as_array();
  CHECK(a.size() == 5);
  CHECK(a[0].as_int() == 1);
  CHECK(a[1].as_double() == 2.5);
  CHECK(a[2].as_string() == "three");
  CHECK(a[3].as_bool() == true);
  CHECK(a[4].is_null());

  // invalid json throws
  bool threw = false;
  try {
    variable::from_string_json("{\"a\": ");
  } catch (const std::exception &) {
    threw = true;
  }
  CHECK(threw);

  std::fprintf(stderr, "  PASS\n");
}

} // namespace

int main() {
  try {
    test_scalars();
    test_containers();
    test_ref();
    test_from_json();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "!!! FAIL: %s\n", e.what());
    return 1;
  }
  std::fprintf(stderr, "test_variable_to_json: all tests passed\n");
  return 0;
}
