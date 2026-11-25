// Unit test for variable::to_string_s5run() / from_string_s5run().

#include "engine/s5run_variable.h"

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

void test_scalars() {
  std::fprintf(stderr, "== variable s5run: scalars\n");

  CHECK(variable().to_string_s5run() == "K_NULL");
  CHECK(variable(true).to_string_s5run() == "K_BOOL true");
  CHECK(variable(false).to_string_s5run() == "K_BOOL false");
  CHECK(variable((int64_t)1024).to_string_s5run() == "K_INT 1024");
  CHECK(variable((int64_t)-7).to_string_s5run() == "K_INT -7");
  CHECK(variable(3.5).to_string_s5run() == "K_DOUBLE 3.5");
  CHECK(variable(std::string("hello")).to_string_s5run() == "K_STRING \"hello\"");
  CHECK(variable(std::string("line1\nline2\ttab \"q\"")).to_string_s5run()
        == "K_STRING \"line1\\nline2\\ttab \\\"q\\\"\"");
  CHECK(variable(ObjectRef(3, "File")).to_string_s5run() == "K_REF 3 \"File\"");

  std::fprintf(stderr, "  PASS\n");
}

void test_containers() {
  std::fprintf(stderr, "== variable s5run: arrays and objects\n");

  variable_array arr;
  arr.push_back(variable((int64_t)1));
  arr.push_back(variable(std::string("two")));
  arr.push_back(variable(3.5));
  CHECK(variable(arr).to_string_s5run() ==
        "K_ARRAY\n"
        "BEGIN\n"
        "  K_INT 1\n"
        "  K_STRING \"two\"\n"
        "  K_DOUBLE 3.5\n"
        "END");

  variable_object obj;
  obj["size"] = variable((int64_t)1024);
  obj["ok"] = variable(true);
  CHECK(variable(obj).to_string_s5run() ==
        "K_OBJECT\n"
        "BEGIN\n"
        "  ok K_BOOL true\n"
        "  size K_INT 1024\n"
        "END");

  // nested: object with an array of objects (fields sorted by name)
  variable_object inner;
  inner["x"] = variable((int64_t)1);
  variable_array list;
  list.push_back(variable(inner));
  list.push_back(variable((int64_t)2));
  variable_object outer;
  outer["list"] = variable(list);
  outer["flag"] = variable(false);
  CHECK(variable(outer).to_string_s5run() ==
        "K_OBJECT\n"
        "BEGIN\n"
        "  flag K_BOOL false\n"
        "  list K_ARRAY\n"
        "  BEGIN\n"
        "    K_OBJECT\n"
        "    BEGIN\n"
        "      x K_INT 1\n"
        "    END\n"
        "    K_INT 2\n"
        "  END\n"
        "END");

  std::fprintf(stderr, "  PASS\n");
}

void test_roundtrip() {
  std::fprintf(stderr, "== variable s5run: roundtrip\n");

  const char *cases[] = {
    "K_NULL",
    "K_BOOL true",
    "K_BOOL false",
    "K_INT 1737000000",
    "K_DOUBLE 3.5",
    "K_STRING \"hello\"",
    "K_STRING \"esc \\\" \\\\ \\n \\t\"",
    "K_REF 7 \"File\"",
    "K_ARRAY\nBEGIN\n  K_INT 1\n  K_STRING \"two\"\nEND",
    "K_OBJECT\nBEGIN\n  ok K_BOOL true\n  size K_INT 1024\nEND",
    "K_OBJECT\nBEGIN\n  nested K_OBJECT\n  BEGIN\n    x K_INT 1\n  END\n  y K_ARRAY\n  BEGIN\n"
    "    K_INT 1\n    K_INT 2\n  END\nEND",
  };
  for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    variable v = variable::from_string_s5run(cases[i]);
    std::string back = v.to_string_s5run();
    CHECK(back == cases[i] || back + "\n" == cases[i]
          ? true : back == cases[i]);
    if (back != cases[i])
      throw std::runtime_error(std::string("roundtrip mismatch for: ") + cases[i]
                               + "\n got: " + back);
  }

  std::fprintf(stderr, "  PASS\n");
}

void test_parse_errors() {
  std::fprintf(stderr, "== variable s5run: parse errors\n");

  const char *bad[] = {
    "",
    "K_INT",
    "K_INT abc",
    "K_UNKNOWN 1",
    "K_STRING \"unterminated",
    "K_OBJECT\nBEGIN\n  a K_INT 1\n",           // missing END
    "K_ARRAY\nBEGIN\n  K_INT 1\n",               // missing END
    "K_OBJECT\n  a K_INT 1\nEND",                // missing BEGIN
    "K_INT 5\nK_INT 6",                          // trailing lines
  };
  for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
    bool threw = false;
    try {
      variable::from_string_s5run(bad[i]);
    } catch (const std::exception &) {
      threw = true;
    }
    CHECK(threw);
  }

  std::fprintf(stderr, "  PASS\n");
}

} // namespace

int main() {
  try {
    test_scalars();
    test_containers();
    test_roundtrip();
    test_parse_errors();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "!!! FAIL: %s\n", e.what());
    return 1;
  }
  std::fprintf(stderr, "test_variable_to_s5run: all tests passed\n");
  return 0;
}
