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

#include <map>
#include <string>
#include <vector>
#include <set>
#include "s5run_schema_options.h"

namespace s5run {
namespace generators {

// -----------------------------------------------------------------------
// schema model (parsed from s5run-schema.json)
// -----------------------------------------------------------------------

struct schema_argument {
  std::string name;
  std::string type;
  std::string convert_from; // "" if not a convert-arg, "..." if it is
};

struct schema_method {
  std::string name;
  std::string return_type; // "void" if none
  std::vector<schema_argument> arguments;
};

struct schema_function {
  std::string name;
  std::string return_type;
  std::vector<schema_argument> arguments;
};

struct schema_property {
  std::string name;
  std::string type;
  std::string getter; // "" if none
  std::string setter; // "" if none
};

struct schema_class {
  std::string name;
  std::vector<schema_method> methods;
  std::vector<schema_property> properties;
};

class schema {
public:
  schema();
  std::string client_version = "0.0.0";
  std::set<std::string> known_languages() const;
  std::shared_ptr<schema_options> options(const std::string &lang);
  std::shared_ptr<schema_cpp_options> cpp98() const;
  std::shared_ptr<schema_cpp_options> cpp11() const;
  std::shared_ptr<schema_go_options> golang() const;
  std::shared_ptr<schema_py_options> py() const;
  std::shared_ptr<schema_java_options> options_java(const std::string &lang) const;

  std::vector<schema_function> functions;
  std::vector<schema_class> classes;

private:
  std::map<std::string, std::shared_ptr<schema_options>> m_options;
};

// Parses a schema from a JSON string; returns false and fills `error` on failure.
bool load_schema(const std::string &json_data, schema &out, std::string &error);

// Parses a schema from a JSON file; returns false and fills `error` on failure.
bool load_schema_from_file(const std::string &filepath, schema &out, std::string &error);

} // namespace generators
} // namespace s5run
