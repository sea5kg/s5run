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

#include <cstdio>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "client/s5run_client.h"
#include "generators/s5run_generators.h"
#include "server/s5run_server.h"

#ifndef WSJCPP_APP_VERSION
#define WSJCPP_APP_VERSION "unknown"
#endif

// -----------------------------------------------------------------------
// Command (with aliases)
// -----------------------------------------------------------------------

class s5run_parse_command {
public:
  s5run_parse_command(const std::vector<std::string> &alias_names, const std::string &description);
  std::string name() const;
  std::vector<std::string> names() const;
  std::string description() const;
  bool match(const std::string &arg) const;

private:
  std::vector<std::string> m_alias_names;
  std::string m_description;
};

s5run_parse_command::s5run_parse_command(
  const std::vector<std::string> &alias_names, const std::string &description
)
    : m_alias_names(alias_names), m_description(description) {
}

std::string s5run_parse_command::name() const {
  return m_alias_names[0];
}

std::vector<std::string> s5run_parse_command::names() const {
  return m_alias_names;
}

std::string s5run_parse_command::description() const {
  return m_description;
}

bool s5run_parse_command::match(const std::string &arg) const {
  for (size_t i = 0; i < m_alias_names.size(); ++i) {
    if (m_alias_names[i] == arg)
      return true;
  }
  return false;
}

// -----------------------------------------------------------------------
// Application: parse args and dispatch commands
// -----------------------------------------------------------------------

class s5run_parse_args {
public:
  s5run_parse_args(int argc, char **argv);

  void print_banner() const;
  void print_usage() const;
  int exec();

private:
  void print_server_usage() const;
  void print_generate_usage() const;
  void print_run_usage() const;

  int cmd_version();
  int cmd_help();
  int cmd_server();
  int cmd_generate();
  int cmd_run();

  const std::string *next_value(const std::string &option);

  std::string m_command_exec;
  std::vector<std::string> m_arguments;
  std::string m_version;
  std::string m_bin;
  std::vector<s5run_parse_command> m_commands;
};

s5run_parse_args::s5run_parse_args(int argc, char **argv)
    : m_version(WSJCPP_APP_VERSION), m_bin("s5run") {
  m_commands.push_back(s5run_parse_command({"server"}, "Start the script server"));
  m_commands.push_back(s5run_parse_command({"generate"}, "Generate client or provider code"));
  m_commands.push_back(s5run_parse_command({"run"}, "Run a script against a server"));
  m_commands.push_back(s5run_parse_command({"version", "--version", "-v"}, "Print version"));
  m_commands.push_back(s5run_parse_command({"help", "--help", "-h"}, "Show this help"));

  if (argc > 0 && argv[0] && argv[0][0] != '\0')
    m_bin = argv[0];

  if (argc > 1)
    m_command_exec = std::string(argv[1]);

  for (int i = 2; i < argc; ++i)
    m_arguments.push_back(std::string(argv[i]));
}

// Returns the value following `option` in the arguments, or nullptr.
const std::string *s5run_parse_args::next_value(const std::string &option) {
  for (size_t i = 0; i < m_arguments.size(); ++i) {
    if (m_arguments[i] == option && i + 1 < m_arguments.size())
      return &m_arguments[i + 1];
  }
  return nullptr;
}

void s5run_parse_args::print_banner() const {
  std::cout << std::endl;
  std::cout << "           ░░░░░░░                            " << std::endl;
  std::cout << "          ░██▓▓▓▓▒                            " << std::endl;
  std::cout << " ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓  " << std::endl;
  std::cout << " ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒ " << std::endl;
  std::cout << "  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒ " << std::endl;
  std::cout << " ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒ " << std::endl;
  std::cout << "  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░ " << std::endl;
  std::cout << std::endl;
}

void s5run_parse_args::print_usage() const {
  std::cout << "s5run " << m_version << " - remote script execution server" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "  " << m_bin << " <command> [options]" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Commands:" << std::endl;

  size_t max_name = 0;
  for (size_t i = 0; i < m_commands.size(); ++i)
    max_name = std::max(max_name, m_commands[i].name().size());
  for (size_t i = 0; i < m_commands.size(); ++i) {
    std::cout << "  " << m_commands[i].name();
    for (size_t p = m_commands[i].name().size(); p < max_name; ++p)
      std::cout << " ";
    std::cout << "    " << m_commands[i].description() << std::endl;
  }

  std::cout << "" << std::endl;
  std::cout << "Run '" << m_bin << " help <command>' for command details." << std::endl;
}

void s5run_parse_args::print_server_usage() const {
  std::cout << "s5run server - start the script server" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "  " << m_bin << " server --listen [host:]port [--threads N] [--queue] [--db PATH] [--ttl SECONDS]"
            << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --listen [host:]port  listen address (default 0.0.0.0, e.g. --listen 7777 or --listen 0.0.0.0:7777)"
            << std::endl;
  std::cout << "  --threads N           worker threads (default: hardware concurrency)" << std::endl;
  std::cout << "  --queue               enable job queue (results stored in sqlite)" << std::endl;
  std::cout << "  --db PATH             job queue database (default s5run-queue.sqlite3)" << std::endl;
  std::cout << "  --ttl SECONDS         how long to keep job results (default 3600)" << std::endl;
}

void s5run_parse_args::print_generate_usage() const {
  std::cout << "s5run generate - generate client or provider code" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "  " << m_bin << " generate client   --language LANG [--output-dir DIR] [--schema FILE]" << std::endl;
  std::cout << "  " << m_bin << " generate provider --language LANG [--output-dir DIR] [--schema FILE]" << std::endl;
  std::cout << "  " << m_bin << " generate --list" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --language LANG  target language (cpp98, cpp11, go, py, java11, java17, java21)" << std::endl;
  std::cout << "  --output-dir DIR where to write generated files (default: options.<lang>.output-dir from schema)"
            << std::endl;
  std::cout << "  --schema FILE    schema file (default s5run-schema.json)" << std::endl;
  std::cout << "  --list           list available generators" << std::endl;
}

void s5run_parse_args::print_run_usage() const {

  std::cout << m_bin << " run - run a script against a server" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Usage:" << std::endl;
  std::cout << "  " << m_bin << " run <host:port> [FILE] [--provider SPEC] [--api-key KEY] [args...]" << std::endl;
  std::cout << "  cat script.js | " << m_bin << " run <host:port> [args...]" << std::endl;
  std::cout << "" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  FILE              script file to execute (default: stdin)" << std::endl;
  std::cout << "  --provider SPEC   remote context provider: \"host:port,name1,name2\"" << std::endl;
  std::cout << "  --api-key KEY     api key for authentication (stub)" << std::endl;
  std::cout << "  args...           arguments passed to the script entry point" << std::endl;
}

// -----------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------

int s5run_parse_args::cmd_version() {
  std::cout << "s5run " << m_version << std::endl;
  return 0;
}

int s5run_parse_args::cmd_help() {
  if (!m_arguments.empty()) {
    const std::string &sub = m_arguments[0];
    if (sub == "server") {
      print_server_usage();
      return 0;
    }
    if (sub == "generate") {
      print_generate_usage();
      return 0;
    }
    if (sub == "run") {
      print_run_usage();
      return 0;
    }
    if (sub == "version") {
      return cmd_version();
    }
  }
  print_usage();
  return 0;
}

int s5run_parse_args::cmd_server() {
  std::string listen;
  size_t threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 4;
  std::string queue_db;
  int queue_ttl = 3600;

  for (size_t i = 0; i < m_arguments.size(); ++i) {
    const std::string &arg = m_arguments[i];
    if (arg == "--listen" && i + 1 < m_arguments.size()) {
      listen = m_arguments[++i];
    } else if (arg == "--threads" && i + 1 < m_arguments.size()) {
      threads = static_cast<size_t>(std::atoi(m_arguments[++i].c_str()));
    } else if (arg == "--queue") {
      queue_db = "srun-queue.sqlite3";
    } else if (arg == "--db" && i + 1 < m_arguments.size()) {
      queue_db = m_arguments[++i];
    } else if (arg == "--ttl" && i + 1 < m_arguments.size()) {
      queue_ttl = std::atoi(m_arguments[++i].c_str());
    } else if (arg == "--help" || arg == "-h") {
      print_server_usage();
      return 0;
    } else {
      std::cerr << "Unknown option: "  << std::endl << std::endl;
      print_server_usage();
      return 1;
    }
  }

  if (listen.empty()) {
    print_server_usage();
    return 1;
  }

  s5run::server s;
  if (!queue_db.empty())
    s.enable_queue(queue_db, queue_ttl);
  return s.run(listen, threads);
}

int s5run_parse_args::cmd_generate() {
  if (m_arguments.empty()) {
    print_generate_usage();
    return 1;
  }

  std::string subcommand = m_arguments[0];
  std::string language;
  std::string output_dir;
  std::string schema_file = "s5run-schema.json";

  if (subcommand == "--list" || subcommand == "list") {
    auto all = s5run::generators::all();
    std::cout << "available generators:" << std::endl;
    for (const auto &kv : all)
      std::cout << "  " << kv.first << std::endl;
    return 0;
  }

  if (subcommand != "client" && subcommand != "provider") {
    std::cerr << "Unknown generate subcommand: " << subcommand << std::endl << std::endl;
    print_generate_usage();
    return 1;
  }

  for (size_t i = 1; i < m_arguments.size(); ++i) {
    const std::string &arg = m_arguments[i];
    if (arg == "--language" && i + 1 < m_arguments.size()) {
      language = m_arguments[++i];
    } else if (arg == "--output-dir" && i + 1 < m_arguments.size()) {
      output_dir = m_arguments[++i];
    } else if (arg == "--schema" && i + 1 < m_arguments.size()) {
      schema_file = m_arguments[++i];
    } else if (arg == "--help" || arg == "-h") {
      print_generate_usage();
      return 0;
    } else {
      std::cerr << "Unknown option: " << arg << std::endl << std::endl;
      print_generate_usage();
      return 1;
    }
  }

  if (language.empty()) {
    std::cerr << "--language is required" << std::endl << std::endl;
    print_generate_usage();
    return 1;
  }

  s5run::generators::schema schema;
  std::string schema_error;
  if (!s5run::generators::load_schema_from_file(schema_file, schema, schema_error)) {
    std::cerr << "schema: " << schema_error << std::endl;
    return 1;
  }
  auto all_generators = s5run::generators::all();

  if (all_generators.count(language) == 0) {
    std::cerr << "unknown generator: " << language << std::endl;
    std::cerr << "run '" << m_bin << " generate --list' for available generators" << std::endl;
    return 1;
  }

  // output-dir: CLI option overrides the schema; fall back to options.<lang>
  if (output_dir.empty()) {
    auto options = schema.options(language);
    if (options)
      output_dir = options->get_output_dir();
  }
  if (output_dir.empty()) {
    std::cerr << "output-dir is not specified: "
      << "use --output-dir or options." << language << ".output-dir in the schema"
      << std::endl<< std::endl;
    print_generate_usage();
    return 1;
  }

  if (subcommand == "provider") {
    std::cerr << "generate provider: not implemented yet" << std::endl << std::endl;
    return 1;
  }

  auto gen = all_generators[language];
  auto files = gen->generate(schema, output_dir);
  for (const auto &f : files)
    std::cout << "generated: " << f << std::endl;
  return 0;
}

int s5run_parse_args::cmd_run() {
  if (m_arguments.empty()) {
    print_run_usage();
    return 1;
  }

  std::string host_port = m_arguments[0];
  std::string script_file;
  std::string api_key;
  std::vector<std::string> providers;
  std::vector<s5run::variable> script_args;

  for (size_t i = 1; i < m_arguments.size(); ++i) {
    const std::string &arg = m_arguments[i];
    if (arg == "--script" && i + 1 < m_arguments.size()) {
      script_file = m_arguments[++i];
    } else if (arg == "--provider" && i + 1 < m_arguments.size()) {
      providers.push_back(m_arguments[++i]);
    } else if (arg == "--api-key" && i + 1 < m_arguments.size()) {
      api_key = m_arguments[++i];
    } else if (arg == "--help" || arg == "-h") {
      print_run_usage();
      return 0;
    } else {
      script_args.push_back(s5run::variable::parse_script_arg(arg));
    }
  }

  if (!api_key.empty()) {
    std::cerr << "run: --api-key is a stub, not implemented yet" << std::endl;
  }

  // Read script from file or stdin
  std::string script;
  if (!script_file.empty()) {
    FILE *f = std::fopen(script_file.c_str(), "rb");
    if (!f) {
      std::cerr << "run: cannot open script file: " << script_file << std::endl;
      return 1;
    }
    char buf[4096];
    while (size_t n = std::fread(buf, 1, sizeof(buf), f))
      script.append(buf, n);
    std::fclose(f);
  } else {
    char buf[4096];
    while (size_t n = std::fread(buf, 1, sizeof(buf), stdin))
      script.append(buf, n);
  }

  if (script.empty()) {
    std::cerr << "run: empty script" << std::endl;
    return 1;
  }

  if (!providers.empty()) {
    std::cerr << "run: --provider is a stub, not implemented yet" << std::endl;
    for (const auto &p : providers)
      std::cerr << "  provider: " << p << std::endl;
  }

  // host:port
  std::string host = host_port;
  uint16_t port = 7777;
  size_t colon = host_port.rfind(':');
  if (colon != std::string::npos) {
    host = host_port.substr(0, colon);
    port = (uint16_t)std::atoi(host_port.substr(colon + 1).c_str());
  }
  if (host.empty() || port == 0) {
    std::cerr << "run: invalid server address: " << host_port << std::endl;
    return 1;
  }

  s5run::client_context ctx;
  try {
    ctx.connect(host, port);
    ctx.register_console(nullptr);
  } catch (const std::exception &e) {
    std::cerr << "run: " << e.what() << std::endl;
    return 1;
  }

  s5run::run_result result = ctx.run(script, "main", script_args);

  if (!result.ok) {
    std::cerr << "run error: " << result.error << std::endl;
    std::cerr << "  duration: " << result.duration_ms << "ms" << std::endl;
    return 1;
  }

  
  std::cout << s5run::value_to_display_string(result.value) << std::endl;
  std::cout << "  duration: " << result.duration_ms << "ms" << std::endl;
  return 0;
}

// -----------------------------------------------------------------------
// Dispatch
// -----------------------------------------------------------------------

int s5run_parse_args::exec() {
  if (m_command_exec.empty())
    return -1;

  // version: without banner
  if (m_command_exec == "version" || m_command_exec == "--version" || m_command_exec == "-v")
    return cmd_version();

  // everything else: banner first
  print_banner();

  if (m_command_exec == "help" || m_command_exec == "--help" || m_command_exec == "-h")
    return cmd_help();

  if (m_command_exec == "server")
    return cmd_server();

  if (m_command_exec == "generate")
    return cmd_generate();

  if (m_command_exec == "run")
    return cmd_run();

  std::cerr << "Unknown command: " << m_command_exec << std::endl << std::endl;
  print_usage();
  return 1;
}

int main(int argc, char **argv) {
  s5run_parse_args app(argc, argv);

  if (argc < 2) {
    app.print_banner();
    app.print_usage();
    return 1;
  }

  return app.exec();
}
