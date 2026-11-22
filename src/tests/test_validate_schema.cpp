// Unit test for schema parsing and type validation (s5run_schema.cpp).
// load_schema parses JSON strings directly - no files needed.

#include "generators/s5run_schema.h"

#include <cstdio>
#include <stdexcept>
#include <string>

using s5run::generators::load_schema;
using s5run::generators::load_schema_from_file;
using s5run::generators::schema;

#define CHECK(cond) do { \
    if (!(cond)) { \
      std::fprintf(stderr, "CHECK failed: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      throw std::runtime_error("check failed: " #cond); \
    } \
  } while (0)

namespace {

// Parses json and expects failure with an error containing `needle`.
void expect_fail(const std::string &name, const std::string &json,
                 const std::string &needle) {
  schema s;
  std::string error;
  if (load_schema(json, s, error)) {
    throw std::runtime_error(name + ": expected failure, but schema passed");
  }
  if (error.find(needle) == std::string::npos)
    throw std::runtime_error(name + ": unexpected error: " + error);
}

// Parses json, must succeed; returns the parsed schema for further checks.
schema expect_ok(const std::string &name, const std::string &json) {
  schema s;
  std::string error;
  if (!load_schema(json, s, error))
    throw std::runtime_error(name + ": expected success, got error: " + error);
  return s;
}

// -----------------------------------------------------------------------

void test_valid_schemas() {
  std::fprintf(stderr, "== schema: valid schemas\n");

  // full-featured schema: scalars, refs, arrays, convert-args
  schema s = expect_ok("ok_full", R"JSON({
    "client-version": "1.0.0",
    "functions": [
      {"name": "GetFile", "return": "File",
       "arguments": [{"name": "filepath", "type": "string"}]},
      {"name": "GetTimeNow", "return": "int", "arguments": []},
      {"name": "HasFile", "return": "bool",
       "arguments": [{"name": "filepath", "type": "string"}]},
      {"name": "GetFileList", "return": "array<string>",
       "arguments": [{"name": "path", "type": "string"}]},
      {"name": "CloseFiles",
       "arguments": [{"name": "files", "type": "array<File>"}]},
      {"name": "Ping", "return": "void", "arguments": []}
    ],
    "classes": [
      {"name": "File", "methods": [
        {"name": "GetSize", "return": "int", "arguments": []},
        {"name": "ReadBytes", "return": "array<byte>",
         "arguments": [{"name": "n", "type": "int"}]}
      ]},
      {"name": "console", "methods": [
        {"name": "log",
         "arguments": {"convert-from": "...", "type": "array<string>", "name": "args"}}
      ]}
    ]
  })JSON");
  CHECK(s.client_version == "1.0.0");
  CHECK(s.functions.size() == 6);
  CHECK(s.classes.size() == 2);
  CHECK(s.functions[0].return_type == "File");
  CHECK(s.classes[0].methods[0].name == "GetSize");
  CHECK(s.classes[1].methods[0].arguments[0].convert_from == "...");

  // nested arrays of scalars
  expect_ok("ok_nested_array", R"JSON({
    "functions": [{"name": "F", "return": "array<array<int>>", "arguments": []}]
  })JSON");

  // uint/float/double/byte variants
  expect_ok("ok_scalars", R"JSON({
    "functions": [
      {"name": "F", "return": "uint",
       "arguments": [
         {"name": "a", "type": "float"},
         {"name": "b", "type": "double"},
         {"name": "c", "type": "byte"}
       ]}
    ]
  })JSON");

  // empty schema is valid
  expect_ok("ok_empty", "{}");

  std::fprintf(stderr, "  PASS\n");
}

void test_unknown_ref_types() {
  std::fprintf(stderr, "== schema: unknown ref types\n");

  expect_fail("bad_ref_return", R"JSON({
    "functions": [{"name": "F", "return": "File", "arguments": []}]
  })JSON", "unknown type 'File'");

  expect_fail("bad_ref_arg", R"JSON({
    "classes": [{"name": "File", "methods": []}],
    "functions": [
      {"name": "F", "return": "void",
       "arguments": [{"name": "x", "type": "Storage"}]}
    ]
  })JSON", "unknown type 'Storage'");

  expect_fail("bad_ref_in_array", R"JSON({
    "functions": [{"name": "F", "return": "array<Widget>", "arguments": []}]
  })JSON", "unknown type 'Widget'");

  expect_fail("bad_ref_method", R"JSON({
    "classes": [{"name": "A", "methods": [
      {"name": "m", "return": "File", "arguments": []}
    ]}]
  })JSON", "unknown type 'File'");

  std::fprintf(stderr, "  PASS\n");
}

void test_void_misuse() {
  std::fprintf(stderr, "== schema: void misuse\n");

  expect_fail("void_arg", R"JSON({
    "functions": [
      {"name": "F", "return": "void",
       "arguments": [{"name": "x", "type": "void"}]}
    ]
  })JSON", "'void' is not allowed here");

  expect_fail("void_array", R"JSON({
    "functions": [{"name": "F", "return": "array<void>", "arguments": []}]
  })JSON", "'void' is not allowed here");

  std::fprintf(stderr, "  PASS\n");
}

void test_convert_args() {
  std::fprintf(stderr, "== schema: convert-args\n");

  // convert-args type must be array<string>
  expect_fail("convert_bad_type", R"JSON({
    "classes": [{"name": "console", "methods": [
      {"name": "log",
       "arguments": {"convert-from": "...", "type": "array<int>", "name": "args"}}
    ]}]
  })JSON", "convert-args type must be array<string>");

  // convert-from must be "..."
  expect_fail("convert_bad_from", R"JSON({
    "classes": [{"name": "console", "methods": [
      {"name": "log",
       "arguments": {"convert-from": "args", "type": "array<string>", "name": "args"}}
    ]}]
  })JSON", "bad convert-from 'args'");

  std::fprintf(stderr, "  PASS\n");
}

void test_duplicates() {
  std::fprintf(stderr, "== schema: duplicates\n");

  expect_fail("dup_class", R"JSON({
    "classes": [{"name": "A", "methods": []}, {"name": "A", "methods": []}]
  })JSON", "duplicate class name: A");

  expect_fail("dup_func", R"JSON({
    "functions": [{"name": "F", "arguments": []}, {"name": "F", "arguments": []}]
  })JSON", "duplicate function name: F");

  expect_fail("dup_method", R"JSON({
    "classes": [{"name": "A", "methods": [
      {"name": "m", "return": "int", "arguments": []},
      {"name": "m", "return": "int", "arguments": []}
    ]}]
  })JSON", "duplicate method name: m in class A");

  std::fprintf(stderr, "  PASS\n");
}

void test_builtin_shadowing() {
  std::fprintf(stderr, "== schema: builtin shadowing\n");

  expect_fail("class_string", R"JSON({
    "classes": [{"name": "string", "methods": []}]
  })JSON", "conflicts with a builtin type");

  expect_fail("class_void", R"JSON({
    "classes": [{"name": "void", "methods": []}]
  })JSON", "conflicts with a builtin type");

  std::fprintf(stderr, "  PASS\n");
}

void test_bad_array_syntax() {
  std::fprintf(stderr, "== schema: bad array syntax\n");

  expect_fail("array_no_close", R"JSON({
    "functions": [{"name": "F", "return": "array<int", "arguments": []}]
  })JSON", "malformed array type 'array<int'");

  expect_fail("array_empty", R"JSON({
    "functions": [{"name": "F", "return": "array<>", "arguments": []}]
  })JSON", "empty type");

  std::fprintf(stderr, "  PASS\n");
}

void test_parse_errors() {
  std::fprintf(stderr, "== schema: parse errors\n");

  expect_fail("missing_name", R"JSON({
    "functions": [{"return": "int", "arguments": []}]
  })JSON", "function missing name");

  expect_fail("missing_arg_type", R"JSON({
    "functions": [
      {"name": "F", "return": "int",
       "arguments": [{"name": "x"}]}
    ]
  })JSON", "missing name/type");

  // broken json syntax
  expect_fail("broken_json", R"JSON({"functions": [)JSON", "JSON parse error");

  // wrong type of a field (functions is not an array)
  expect_fail("functions_not_array", R"JSON({
    "functions": 42
  })JSON", "JSON structure error");

  // load_schema_from_file: cannot open
  {
    schema s;
    std::string error;
    CHECK(!load_schema_from_file("/tmp/opencode/no-such-schema-file.json", s, error));
    CHECK(error.find("cannot open") != std::string::npos);
  }

  std::fprintf(stderr, "  PASS\n");
}

} // namespace

int main() {
  try {
    test_valid_schemas();
    test_unknown_ref_types();
    test_void_misuse();
    test_convert_args();
    test_duplicates();
    test_builtin_shadowing();
    test_bad_array_syntax();
    test_parse_errors();
  } catch (const std::exception &e) {
    std::fprintf(stderr, "!!! FAIL: %s\n", e.what());
    return 1;
  }
  std::fprintf(stderr, "test_validate_schema: all tests passed\n");
  return 0;
}
