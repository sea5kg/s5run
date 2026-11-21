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

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sys/stat.h>

namespace s5run {
namespace generators {

static std::vector<schema_argument> parse_arguments(const nlohmann::json &j,
                                                    const std::string &owner) {
  std::vector<schema_argument> out;
  if (!j.contains("arguments"))
    return out;
  if (j["arguments"].is_object()) {
    const auto &a = j["arguments"];
    schema_argument arg;
    arg.name = a.value("name", "");
    arg.type = a.value("type", "");
    arg.convert_from = a.value("convert-from", "");
    if (arg.name.empty() || arg.type.empty()) {
      std::fprintf(stderr, "schema: bad convert-args in %s\n", owner.c_str());
      std::exit(1);
    }
    out.push_back(arg);
    return out;
  }
  for (const auto &a : j["arguments"]) {
    schema_argument arg;
    arg.name = a.value("name", "");
    arg.type = a.value("type", "");
    if (arg.name.empty() || arg.type.empty()) {
      std::fprintf(stderr, "schema: missing name/type in %s\n", owner.c_str());
      std::exit(1);
    }
    out.push_back(arg);
  }
  return out;
}

schema load_schema(const std::string &filepath) {
  std::ifstream f(filepath);
  if (!f.is_open()) {
    std::fprintf(stderr, "schema: cannot open %s\n", filepath.c_str());
    std::exit(1);
  }
  nlohmann::json j;
  try {
    f >> j;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "schema: JSON parse error: %s\n", e.what());
    std::exit(1);
  }

  schema s;
  s.client_version = j.value("client-version", "0.0.0");

  if (j.contains("options") && j["options"].contains("cpp98")) {
    const auto &o = j["options"]["cpp98"];
    s.cpp98.namespace_ = o.value("namespace", "");
    s.cpp98.output_dir = o.value("output-dir", "");
    if (s.cpp98.namespace_.empty() || s.cpp98.output_dir.empty()) {
      std::fprintf(stderr, "schema: options.cpp98 requires namespace and output-dir\n");
      std::exit(1);
    }
  }
  if (j.contains("options") && j["options"].contains("cpp11")) {
    const auto &o = j["options"]["cpp11"];
    s.cpp11.namespace_ = o.value("namespace", "");
    s.cpp11.output_dir = o.value("output-dir", "");
    if (s.cpp11.namespace_.empty() || s.cpp11.output_dir.empty()) {
      std::fprintf(stderr, "schema: options.cpp11 requires namespace and output-dir\n");
      std::exit(1);
    }
  }
  if (j.contains("functions")) {
    for (const auto &fj : j["functions"]) {
      schema_function fn;
      fn.name = fj.value("name", "");
      fn.return_type = fj.value("return", "void");
      if (fn.name.empty()) {
        std::fprintf(stderr, "schema: function missing name\n");
        std::exit(1);
      }
      fn.arguments = parse_arguments(fj, "function " + fn.name);
      s.functions.push_back(fn);
    }
  }
  if (j.contains("classes")) {
    for (const auto &cj : j["classes"]) {
      schema_class cls;
      cls.name = cj.value("name", "");
      if (cls.name.empty()) {
        std::fprintf(stderr, "schema: class missing name\n");
        std::exit(1);
      }
      if (cj.contains("methods")) {
        for (const auto &mj : cj["methods"]) {
          schema_method m;
          m.name = mj.value("name", "");
          m.return_type = mj.value("return", "void");
          if (m.name.empty()) {
            std::fprintf(stderr, "schema: method missing name in class %s\n", cls.name.c_str());
            std::exit(1);
          }
          m.arguments = parse_arguments(mj, "class " + cls.name + " method " + m.name);
          cls.methods.push_back(m);
        }
      }
      s.classes.push_back(cls);
    }
  }
  return s;
}

} // namespace generators
} // namespace s5run
