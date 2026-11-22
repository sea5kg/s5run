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

#include "s5run_schema.h"

#include "nlohmann/json.hpp"

#include <fstream>
#include <set>
#include <sstream>

namespace s5run {
namespace generators {

// -----------------------------------------------------------------------
// Type validation
// -----------------------------------------------------------------------

static bool is_scalar_type(const std::string &t) {
  return t == "bool" || t == "int" || t == "uint" || t == "float" ||
         t == "double" || t == "string" || t == "byte";
}

// Validates a type usage; ref types must be declared in classes.
static bool validate_type(const std::string &t, bool allow_void,
                          const std::set<std::string> &classes,
                          const std::string &context, std::string &error) {
  if (t.empty()) {
    error = context + ": empty type";
    return false;
  }
  if (t == "void") {
    if (!allow_void) {
      error = context + ": 'void' is not allowed here";
      return false;
    }
    return true;
  }
  if (is_scalar_type(t))
    return true;
  if (t.substr(0, 6) == "array<") {
    if (t.size() < 7 || t[t.size() - 1] != '>') {
      error = context + ": malformed array type '" + t + "'";
      return false;
    }
    return validate_type(t.substr(6, t.size() - 7), false, classes,
                         context + " (array element)", error);
  }
  if (classes.count(t) == 0) {
    error = context + ": unknown type '" + t
            + "' (not a scalar and not declared in classes)";
    return false;
  }
  return true;
}

static bool validate_arguments(const std::vector<schema_argument> &args,
                               const std::set<std::string> &classes,
                               const std::string &context, std::string &error) {
  for (const auto &a : args) {
    if (!a.convert_from.empty()) {
      if (a.convert_from != "...") {
        error = context + ": bad convert-from '" + a.convert_from + "' (expected \"...\")";
        return false;
      }
      if (a.type != "array<string>") {
        error = context + ": convert-args type must be array<string>, got '" + a.type + "'";
        return false;
      }
      continue;
    }
    if (!validate_type(a.type, false, classes, context + " argument '" + a.name + "'", error))
      return false;
  }
  return true;
}

static bool validate_schema(const schema &s, std::string &error) {
  std::set<std::string> classes;
  for (const auto &c : s.classes) {
    if (is_scalar_type(c.name) || c.name == "void" || c.name.substr(0, 6) == "array<") {
      error = "class name '" + c.name + "' conflicts with a builtin type";
      return false;
    }
    if (classes.count(c.name) != 0) {
      error = "duplicate class name: " + c.name;
      return false;
    }
    classes.insert(c.name);
  }

  std::set<std::string> functions;
  for (const auto &f : s.functions) {
    if (functions.count(f.name) != 0) {
      error = "duplicate function name: " + f.name;
      return false;
    }
    functions.insert(f.name);
    if (!validate_type(f.return_type, true, classes, "function " + f.name + " return", error))
      return false;
    if (!validate_arguments(f.arguments, classes, "function " + f.name, error))
      return false;
  }

  for (const auto &c : s.classes) {
    std::set<std::string> methods;
    for (const auto &m : c.methods) {
      if (methods.count(m.name) != 0) {
        error = "duplicate method name: " + m.name + " in class " + c.name;
        return false;
      }
      methods.insert(m.name);
      if (!validate_type(m.return_type, true, classes,
                         "class " + c.name + " method " + m.name + " return", error))
        return false;
      if (!validate_arguments(m.arguments, classes, "class " + c.name + " method " + m.name, error))
        return false;
    }
  }
  return true;
}

// -----------------------------------------------------------------------
// Parsing
// -----------------------------------------------------------------------

static bool parse_arguments(const nlohmann::json &j, const std::string &owner,
                            std::vector<schema_argument> &out, std::string &error) {
  if (!j.contains("arguments"))
    return true;
  if (j["arguments"].is_object()) {
    const auto &a = j["arguments"];
    schema_argument arg;
    arg.name = a.value("name", "");
    arg.type = a.value("type", "");
    arg.convert_from = a.value("convert-from", "");
    if (arg.name.empty() || arg.type.empty()) {
      error = "bad convert-args in " + owner;
      return false;
    }
    out.push_back(arg);
    return true;
  }
  for (const auto &a : j["arguments"]) {
    schema_argument arg;
    arg.name = a.value("name", "");
    arg.type = a.value("type", "");
    if (arg.name.empty() || arg.type.empty()) {
      error = "missing name/type in " + owner;
      return false;
    }
    out.push_back(arg);
  }
  return true;
}

static bool parse_schema_json(const nlohmann::json &j, schema &s, std::string &error) {
  s.client_version = j.value("client-version", "0.0.0");

  if (j.contains("options") && j["options"].contains("cpp98")) {
    const auto &o = j["options"]["cpp98"];
    s.cpp98.namespace_ = o.value("namespace", "");
    s.cpp98.output_dir = o.value("output-dir", "");
    s.cpp98.prefix_filename = o.value("prefix-filename", "");
    if (s.cpp98.namespace_.empty() || s.cpp98.output_dir.empty()) {
      error = "options.cpp98 requires namespace and output-dir";
      return false;
    }
  }
  if (j.contains("options") && j["options"].contains("cpp11")) {
    const auto &o = j["options"]["cpp11"];
    s.cpp11.namespace_ = o.value("namespace", "");
    s.cpp11.output_dir = o.value("output-dir", "");
    s.cpp11.prefix_filename = o.value("prefix-filename", "");
    if (s.cpp11.namespace_.empty() || s.cpp11.output_dir.empty()) {
      error = "options.cpp11 requires namespace and output-dir";
      return false;
    }
  }
  if (j.contains("options") && j["options"].contains("go")) {
    const auto &o = j["options"]["go"];
    s.golang.package_ = o.value("package", "");
    s.golang.output_dir = o.value("output-dir", "");
  }
  if (j.contains("functions")) {
    for (const auto &fj : j["functions"]) {
      schema_function fn;
      fn.name = fj.value("name", "");
      fn.return_type = fj.value("return", "void");
      if (fn.name.empty()) {
        error = "function missing name";
        return false;
      }
      if (!parse_arguments(fj, "function " + fn.name, fn.arguments, error))
        return false;
      s.functions.push_back(fn);
    }
  }
  if (j.contains("classes")) {
    for (const auto &cj : j["classes"]) {
      schema_class cls;
      cls.name = cj.value("name", "");
      if (cls.name.empty()) {
        error = "class missing name";
        return false;
      }
      if (cj.contains("methods")) {
        for (const auto &mj : cj["methods"]) {
          schema_method m;
          m.name = mj.value("name", "");
          m.return_type = mj.value("return", "void");
          if (m.name.empty()) {
            error = "method missing name in class " + cls.name;
            return false;
          }
          if (!parse_arguments(mj, "class " + cls.name + " method " + m.name, m.arguments, error))
            return false;
          cls.methods.push_back(m);
        }
      }
      s.classes.push_back(cls);
    }
  }
  return true;
}

bool load_schema(const std::string &json_data, schema &out, std::string &error) {
  nlohmann::json j;
  try {
    j = nlohmann::json::parse(json_data);
  } catch (const std::exception &e) {
    error = std::string("JSON parse error: ") + e.what();
    return false;
  }

  try {
    if (!parse_schema_json(j, out, error))
      return false;
  } catch (const std::exception &e) {
    error = std::string("JSON structure error: ") + e.what();
    return false;
  }

  if (!validate_schema(out, error))
    return false;
  return true;
}

bool load_schema_from_file(const std::string &filepath, schema &out, std::string &error) {
  std::ifstream f(filepath);
  if (!f.is_open()) {
    error = "cannot open " + filepath;
    return false;
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return load_schema(ss.str(), out, error);
}

} // namespace generators
} // namespace s5run
