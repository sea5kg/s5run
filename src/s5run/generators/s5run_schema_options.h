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

#pragma once

#include "nlohmann/json.hpp"
#include <map>
#include <string>
#include <vector>

namespace s5run {
namespace generators {

class schema_options {
public:
  schema_options(const std::string &name);
  std::string get_name();
  bool check_allowed_keys(const nlohmann::json &o, std::vector<std::string> keys, std::string &error);
  virtual bool parse(const nlohmann::json &o, std::string &error) = 0;
  void set_output_dir(const std::string &output_dir);
  std::string get_output_dir() const;
  bool parse_output_dir(const nlohmann::json &o);

  // for dynamic casts
  // bool is_cpp();
  // bool is_go();
  // bool is_java();
  // bool is_java();
protected:
  void set_parsed(bool val);
private:
  bool m_parsed;
  std::string m_name;
  std::string m_output_dir;
};

class schema_options_namespace {
public:
  void set_namespace(const std::string &val);
  std::string get_namespace() const;
  bool parse_namespace(const nlohmann::json &o);

private:
  std::string m_namespace;
};

class schema_options_package_name {
public:
  void set_package_name(const std::string &package_name);
  std::string get_package_name() const;
  bool parse_package_name(const nlohmann::json &o);

private:
  std::string m_package_name;
};

class schema_options_module_name {
public:
  void set_module_name(const std::string &name);
  std::string get_module_name() const;
  bool parse_module_name(const nlohmann::json &o);

private:
  std::string m_module_name;
};

class schema_options_prefix_filename {
public:
  void set_prefix_filename(const std::string &prefix_filename);
  std::string get_prefix_filename() const;
  bool parse_prefix_filename(const nlohmann::json &o);

private:
  std::string m_prefix_filename;
};

class schema_cpp_options : public schema_options,
                           public schema_options_namespace,
                           public schema_options_prefix_filename {
public:
  schema_cpp_options(const std::string &name);
  virtual bool parse(const nlohmann::json &o, std::string &error) override;
};

class schema_go_options : public schema_options, public schema_options_package_name {
public:
  schema_go_options(const std::string &name);
  virtual bool parse(const nlohmann::json &o, std::string &error) override;
};

class schema_py_options : public schema_options, public schema_options_module_name {
public:
  schema_py_options(const std::string &name);
  virtual bool parse(const nlohmann::json &o, std::string &error) override;
};

class schema_java_options : public schema_options, public schema_options_package_name {
public:
  schema_java_options(const std::string &name);
  virtual bool parse(const nlohmann::json &o, std::string &error);
};

} // namespace generators
} // namespace s5run
