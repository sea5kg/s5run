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

#include "s5run_schema_options.h"

#include "nlohmann/json.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace s5run {
namespace generators {

schema_options::schema_options(const std::string &name) : m_name(name), m_parsed(false) {

}

std::string schema_options::get_name() {
  return m_name;
}

bool schema_options::check_allowed_keys(const nlohmann::json &o, std::vector<std::string> allowed, std::string &error) {
  for (const auto &kv : o.items()) {
    std::string key = kv.key();
    bool is_allowed_key = std::find(allowed.begin(), allowed.end(), key) != allowed.end();
    if (!is_allowed_key) {
      error = "unknown field '" + key + "' in options." + get_name();
      return false;
    }
  }
  return true;
}

void schema_options::set_output_dir(const std::string &output_dir) {
  m_output_dir = output_dir;
}

std::string schema_options::get_output_dir() const {
  return m_output_dir;
}

bool schema_options::parse_output_dir(const nlohmann::json &o) {
  std::string val = o.value("output-dir", "");
  if (!val.empty()) {
    m_output_dir = val;
    return true;
  }
  return false;
}

void schema_options::set_parsed(bool val) {
  m_parsed = val;
}

void schema_options_namespace::set_namespace(const std::string &val) {
  m_namespace = val;
}

std::string schema_options_namespace::get_namespace() const {
  return m_namespace;
}

bool schema_options_namespace::parse_namespace(const nlohmann::json &o) {
  std::string val = o.value("namespace", "");
  if (!val.empty()) {
    set_namespace(val);
    return true;
  }
  return false;
}

void schema_options_package_name::set_package_name(const std::string &val) {
  m_package_name = val;
}

std::string schema_options_package_name::get_package_name() const {
  return m_package_name;
}

bool schema_options_package_name::parse_package_name(const nlohmann::json &o) {
  std::string val = o.value("package", "");
  if (!val.empty()) {
    set_package_name(val);
    return true;
  }
  return false;
}

void schema_options_module_name::set_module_name(const std::string &val) {
  m_module_name = val;
}

std::string schema_options_module_name::get_module_name() const {
  return m_module_name;
}

bool schema_options_module_name::parse_module_name(const nlohmann::json &o) {
  std::string val = o.value("module", "");
  if (!val.empty()) {
    set_module_name(val);
    return true;
  }
  return false;
}

void schema_options_prefix_filename::set_prefix_filename(const std::string &prefix_filename) {
  m_prefix_filename = prefix_filename;
}

std::string schema_options_prefix_filename::get_prefix_filename() const {
  return m_prefix_filename;
}

bool schema_options_prefix_filename::parse_prefix_filename(const nlohmann::json &o) {
  std::string val = o.value("prefix-filename", "");
  if (!val.empty()) {
    set_prefix_filename(val);
    return true;
  }
  return false;
}

schema_cpp_options::schema_cpp_options(const std::string &name) : schema_options(name) {

}

bool schema_cpp_options::parse(const nlohmann::json &o, std::string &error) {
  std::vector<std::string> allowed = {"output-dir", "namespace", "prefix-filename"};
  if (!check_allowed_keys(o, allowed, error)) {
    return false;
  }
  parse_output_dir(o);
  if (!parse_namespace(o)) {
    error = "options." + get_name() + " required 'namespace'";
    return false;
  }
  parse_prefix_filename(o);

  // TODO check prefix_filename if exists
  // TODO check namespace if exists
  set_parsed(true);
  return true;
}

schema_go_options::schema_go_options(const std::string &name) : schema_options(name) {

}

bool schema_go_options::parse(const nlohmann::json &o, std::string &error) {
  std::vector<std::string> allowed = {"output-dir", "package"};
  if (!check_allowed_keys(o, allowed, error)) {
    return false;
  }
  parse_output_dir(o);
  if (!parse_package_name(o)) {
    error = "options." + get_name() + " required 'package'";
    return false;
  }
  set_parsed(true);
  return true;
}

schema_py_options::schema_py_options(const std::string &name) : schema_options(name) {
}

bool schema_py_options::parse(const nlohmann::json &o, std::string &error) {
  std::vector<std::string> allowed = {"output-dir", "module"};
  if (!check_allowed_keys(o, allowed, error)) {
    return false;
  }
  parse_output_dir(o);
  if (!parse_module_name(o)) {
    error = "options." + get_name() + " required 'module'";
    return false;
  }
  set_parsed(true);
  return true;
}

schema_java_options::schema_java_options(const std::string &name) : schema_options(name) {
}

bool schema_java_options::parse(const nlohmann::json &o, std::string &error) {
  std::vector<std::string> allowed = {"output-dir", "package"};
  if (!check_allowed_keys(o, allowed, error)) {
    return false;
  }
  parse_output_dir(o);
  if (!parse_package_name(o)) {
    error = "options." + get_name() + " required 'package'";
    return false;
  }
  set_parsed(true);
  return true;
}

} // namespace generators
} // namespace s5run


