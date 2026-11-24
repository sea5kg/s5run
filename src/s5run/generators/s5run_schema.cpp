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
#include <iostream>

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
    std::set<std::string> props;
    for (const auto &p : c.properties) {
      if (props.count(p.name) != 0) {
        error = "duplicate property name: " + p.name + " in class " + c.name;
        return false;
      }
      props.insert(p.name);
      if (!validate_type(p.type, false, classes,
                         "class " + c.name + " property '" + p.name + "'", error))
        return false;
    }
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

// Checks that a json object contains only known fields; on an unknown
// field reports what is allowed in this section.
static bool check_unknown_fields(const nlohmann::json &j,
                                 std::initializer_list<const char *> allowed,
                                 const std::string &where, std::string &error) {
  if (!j.is_object())
    return true;
  for (const auto &kv : j.items()) {
    bool known = false;
    for (const auto *a : allowed) {
      if (kv.key() == a) {
        known = true;
        break;
      }
    }
    if (!known) {
      std::string list;
      for (const auto *a : allowed) {
        if (!list.empty())
          list += ", ";
        list += a;
      }
      error = "unknown field '" + kv.key() + "' in " + where + " (allowed: " + list + ")";
      return false;
    }
  }
  return true;
}

static bool parse_arguments(const nlohmann::json &j, const std::string &owner,
                            std::vector<schema_argument> &out, std::string &error) {
  if (!j.contains("arguments"))
    return true;
  if (!check_unknown_fields(j, {"name", "return", "arguments"}, owner, error))
    return false;
  if (j["arguments"].is_object()) {
    const auto &a = j["arguments"];
    if (!check_unknown_fields(a, {"name", "type", "convert-from"}, owner + " arguments", error))
      return false;
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
    if (!check_unknown_fields(a, {"name", "type"}, owner + " arguments", error))
      return false;
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
  if (!check_unknown_fields(j, {"client-version", "version", "options", "functions", "classes"},
                            "schema root", error))
    return false;
  s.client_version = j.value("client-version", "0.0.0");
  const std::set<std::string> s_langs = s.known_languages();
  std::string allowed_langs = "";
  for (auto it_lang = s_langs.begin(); it_lang != s_langs.end(); it_lang++) {
    allowed_langs += *it_lang + ", ";
  }

  if (j.contains("options")) {
    const auto &opts = j["options"];

    for (const auto &kv : opts.items()) {
      if (s_langs.count(kv.key()) == 0) {
        error = "unknown options section '" + kv.key()
                + "' (allowed: " + allowed_langs + ")";
        return false;
      }
    }

    for (auto it_lang = s_langs.begin(); it_lang != s_langs.end(); it_lang++) {
      std::string lang = *it_lang;
      auto opt_lang = s.options(lang);
      if (!opt_lang) {
        error = "not a found definition options for a lang: " + lang;
        return false;
      }
      if (j["options"].contains(lang)) {
        if (!opt_lang->parse(j["options"][lang], error)) {
          return false;
        }
      }
    }
  }

  if (j.contains("functions")) {
    for (const auto &fj : j["functions"]) {
      if (!check_unknown_fields(fj, {"name", "return", "arguments"}, "functions[]", error))
        return false;
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
      if (!check_unknown_fields(cj, {"name", "methods", "properties"}, "classes[]", error))
        return false;
      schema_class cls;
      cls.name = cj.value("name", "");
      if (cls.name.empty()) {
        error = "class missing name";
        return false;
      }
      if (cj.contains("properties")) {
        for (const auto &pj : cj["properties"]) {
          std::string where = "class " + cls.name + " properties[]";
          if (!check_unknown_fields(pj, {"name", "type", "getter", "setter"}, where, error))
            return false;
          schema_property prop;
          prop.name = pj.value("name", "");
          prop.type = pj.value("type", "");
          if (prop.name.empty() || prop.type.empty()) {
            error = where + ": missing name/type";
            return false;
          }
          prop.getter = pj.value("getter", "");
          prop.setter = pj.value("setter", "");
          cls.properties.push_back(prop);
        }
      }
      if (cj.contains("methods")) {
        for (const auto &mj : cj["methods"]) {
          if (!check_unknown_fields(mj, {"name", "return", "arguments"},
                                    "class " + cls.name + " methods[]", error))
            return false;
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

schema::schema() {
  m_options["cpp98"] = std::make_shared<schema_cpp_options>("cpp98");
  m_options["cpp11"] = std::make_shared<schema_cpp_options>("cpp11");
  m_options["go"] = std::make_shared<schema_go_options>("go");
  m_options["py"] = std::make_shared<schema_py_options>("py");
  m_options["java11"] = std::make_shared<schema_java_options>("java11");
  m_options["java17"] = std::make_shared<schema_java_options>("java17");
  m_options["java21"] = std::make_shared<schema_java_options>("java21");
}

std::set<std::string> schema::known_languages() const {
  std::set<std::string> ret;
  for (auto it = m_options.begin(); it != m_options.end(); ++it)
    ret.insert(it->first);
  return ret;
}

std::shared_ptr<schema_options> schema::options(const std::string &lang) {
  return m_options.at(lang);
}

std::shared_ptr<schema_cpp_options> schema::cpp98() const {
  std::shared_ptr<schema_options> o = m_options.at("cpp98");
  return std::static_pointer_cast<schema_cpp_options>(o);
}

std::shared_ptr<schema_cpp_options> schema::cpp11() const {
  std::shared_ptr<schema_options> o = m_options.at("cpp11");
  return std::static_pointer_cast<schema_cpp_options>(o);
}

std::shared_ptr<schema_go_options> schema::golang() const {
  std::shared_ptr<schema_options> o = m_options.at("go");
  return std::static_pointer_cast<schema_go_options>(o);
}

std::shared_ptr<schema_py_options> schema::py() const {
  std::shared_ptr<schema_options> o = m_options.at("py");
  return std::static_pointer_cast<schema_py_options>(o);
}

std::shared_ptr<schema_java_options> schema::options_java(const std::string &lang) const {
  std::shared_ptr<schema_options> o = m_options.at(lang);
  return std::static_pointer_cast<schema_java_options>(o);
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
