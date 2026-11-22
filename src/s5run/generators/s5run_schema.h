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

namespace s5run {
namespace generators {

// -----------------------------------------------------------------------
// schema model (parsed from s5run-client.json)
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

struct schema_class {
  std::string name;
  std::vector<schema_method> methods;
};

struct schema_options {
  std::string namespace_;
  std::string output_dir;
};

struct schema {
  std::string client_version = "0.0.0";
  schema_options cpp98;
  schema_options cpp11;
  std::vector<schema_function> functions;
  std::vector<schema_class> classes;
};

// Parses a schema from a JSON string; returns false and fills `error` on failure.
bool load_schema(const std::string &json_data, schema &out, std::string &error);

// Parses a schema from a JSON file; returns false and fills `error` on failure.
bool load_schema_from_file(const std::string &filepath, schema &out, std::string &error);

} // namespace generators
} // namespace s5run
