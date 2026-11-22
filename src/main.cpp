/**********************************************************************************
 *
 *            ░░░░░░░
 *           ░██▓▓▓▓▒
 *  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 *  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒█▓   ▓█▒
 *   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 *  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ▒▒░
 *
 * MIT License
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
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "server/s5run_server.h"
#include "generators/s5run_generators.h"
#include "client/s5run_client.h"

static std::string g_bin = "s5run";

static void print_banner() {
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

static void print_usage() {
  std::printf(
    "s5run v0.2.0 - remote script execution server\n"
    "\n"
    "Usage:\n"
    "  %s <command> [options]\n"
    "\n"
    "Commands:\n"
    "  server     Start the script server\n"
    "  generate   Generate client or provider code\n"
    "  run        Run a script against a server\n"
    "  version    Print version\n"
    "  help       Show this help\n"
    "\n"
    "Run '%s help <command>' for command details.\n",
    g_bin.c_str(), g_bin.c_str()
  );
}

static void print_server_usage() {
  std::printf(
    "s5run server - start the script server\n"
    "\n"
    "Usage:\n"
    "  %s server --listen [host:]port [--threads N] [--queue] [--db PATH] [--ttl SECONDS]\n"
    "\n"
    "Options:\n"
    "  --listen [host:]port  listen address (default 0.0.0.0, e.g. --listen 7777 or --listen 0.0.0.0:7777)\n"
    "  --threads N           worker threads (default: hardware concurrency)\n"
    "  --queue               enable job queue (results stored in sqlite)\n"
    "  --db PATH             job queue database (default s5run-queue.sqlite3)\n"
    "  --ttl SECONDS         how long to keep job results (default 3600)\n",
    g_bin.c_str()
  );
}

static void print_generate_usage() {
  std::printf(
    "s5run generate - generate client or provider code\n"
    "\n"
    "Usage:\n"
    "  %s generate client   --language LANG --output-dir DIR [--schema FILE]\n"
    "  %s generate provider --language LANG --output-dir DIR [--schema FILE]\n"
    "  %s generate --list\n"
    "\n"
    "Options:\n"
    "  --language LANG  target language (cpp98, cpp11)\n"
    "  --output-dir DIR where to write generated files\n"
    "  --schema FILE    schema file (default s5run-client.json)\n"
    "  --list           list available generators\n",
    g_bin.c_str(), g_bin.c_str(), g_bin.c_str()
  );
}

static void print_run_usage() {
  std::printf(
    "s5run run - run a script against a server\n"
    "\n"
    "Usage:\n"
    "  %s run <host:port> [--script FILE] [--provider SPEC] [--api-key KEY] [args...]\n"
    "  cat script.js | %s run <host:port> [args...]\n"
    "\n"
    "Options:\n"
    "  --script FILE     script file to execute (default: stdin)\n"
    "  --provider SPEC   remote context provider: \"host:port,name1,name2\"\n"
    "  --api-key KEY     api key for authentication (stub)\n"
    "  args...           arguments passed to the script entry point\n",
    g_bin.c_str(), g_bin.c_str()
  );
}

static int cmd_server(int argc, char **argv) {
  std::string listen;
  size_t threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 4;
  std::string queue_db;
  int queue_ttl = 3600;

  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--listen" && i + 1 < argc) {
      listen = argv[++i];
    } else if (arg == "--threads" && i + 1 < argc) {
      threads = static_cast<size_t>(std::atoi(argv[++i]));
    } else if (arg == "--queue") {
      queue_db = "srun-queue.sqlite3";
    } else if (arg == "--db" && i + 1 < argc) {
      queue_db = argv[++i];
    } else if (arg == "--ttl" && i + 1 < argc) {
      queue_ttl = std::atoi(argv[++i]);
    } else if (arg == "--help" || arg == "-h") {
      print_server_usage();
      return 0;
    } else {
      std::fprintf(stderr, "Unknown option: %s\n\n", arg.c_str());
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

static int cmd_generate(int argc, char **argv) {
  if (argc < 3) {
    print_generate_usage();
    return 1;
  }

  std::string subcommand = argv[2];
  std::string language;
  std::string output_dir;
  std::string schema_file = "s5run-client.json";

  if (subcommand == "--list" || subcommand == "list") {
    auto all = s5run::generators::all();
    std::printf("available generators:\n");
    for (const auto &kv : all)
      std::printf("  %s\n", kv.first.c_str());
    return 0;
  }

  if (subcommand != "client" && subcommand != "provider") {
    std::fprintf(stderr, "Unknown generate subcommand: %s\n\n", subcommand.c_str());
    print_generate_usage();
    return 1;
  }

  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--language" && i + 1 < argc) {
      language = argv[++i];
    } else if (arg == "--output-dir" && i + 1 < argc) {
      output_dir = argv[++i];
    } else if (arg == "--schema" && i + 1 < argc) {
      schema_file = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      print_generate_usage();
      return 0;
    } else {
      std::fprintf(stderr, "Unknown option: %s\n\n", arg.c_str());
      print_generate_usage();
      return 1;
    }
  }

  if (language.empty() || output_dir.empty()) {
    std::fprintf(stderr, "--language and --output-dir are required\n\n");
    print_generate_usage();
    return 1;
  }

  auto schema = s5run::generators::load_schema(schema_file);
  auto all_generators = s5run::generators::all();

  if (all_generators.count(language) == 0) {
    std::fprintf(stderr, "unknown generator: %s\n", language.c_str());
    std::fprintf(stderr, "run '%s generate --list' for available generators\n", g_bin.c_str());
    return 1;
  }

  if (subcommand == "provider") {
    std::fprintf(stderr, "generate provider: not implemented yet\n");
    return 1;
  }

  auto gen = all_generators[language];
  auto files = gen->generate(schema, output_dir);
  for (const auto &f : files)
    std::printf("generated: %s\n", f.c_str());
  return 0;
}

static s5run::Value parse_script_arg(const std::string &raw) {
  // int?
  if (!raw.empty()) {
    char *end = nullptr;
    long long v = std::strtoll(raw.c_str(), &end, 10);
    if (end && *end == '\0')
      return s5run::Value((int64_t)v);
  }
  // bool?
  if (raw == "true")
    return s5run::Value(true);
  if (raw == "false")
    return s5run::Value(false);
  // double?
  if (!raw.empty()) {
    char *end = nullptr;
    double d = std::strtod(raw.c_str(), &end);
    if (end && *end == '\0')
      return s5run::Value(d);
  }
  return s5run::Value(raw);
}

static int cmd_run(int argc, char **argv) {
  if (argc < 3) {
    print_run_usage();
    return 1;
  }

  std::string host_port = argv[2];
  std::string script_file;
  std::string api_key;
  std::vector<std::string> providers;
  std::vector<s5run::Value> script_args;

  for (int i = 3; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--script" && i + 1 < argc) {
      script_file = argv[++i];
    } else if (arg == "--provider" && i + 1 < argc) {
      providers.push_back(argv[++i]);
    } else if (arg == "--api-key" && i + 1 < argc) {
      api_key = argv[++i];
    } else if (arg == "--help" || arg == "-h") {
      print_run_usage();
      return 0;
    } else {
      script_args.push_back(parse_script_arg(arg));
    }
  }

  if (!api_key.empty()) {
    std::fprintf(stderr, "run: --api-key is a stub, not implemented yet\n");
  }

  // Read script from file or stdin
  std::string script;
  if (!script_file.empty()) {
    FILE *f = std::fopen(script_file.c_str(), "rb");
    if (!f) {
      std::fprintf(stderr, "run: cannot open script file: %s\n", script_file.c_str());
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
    std::fprintf(stderr, "run: empty script\n");
    return 1;
  }

  if (!providers.empty()) {
    std::fprintf(stderr, "run: --provider is a stub, not implemented yet\n");
    for (const auto &p : providers)
      std::fprintf(stderr, "  provider: %s\n", p.c_str());
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
    std::fprintf(stderr, "run: invalid server address: %s\n", host_port.c_str());
    return 1;
  }

  s5run::client_context ctx;
  try {
    ctx.connect(host, port);
    ctx.register_console(nullptr);
  } catch (const std::exception &e) {
    std::fprintf(stderr, "run: %s\n", e.what());
    return 1;
  }

  s5run::run_result result = ctx.run(script, "main", script_args);

  if (!result.ok) {
    std::fprintf(stderr, "run error: %s\n", result.error.c_str());
    std::fprintf(stderr, "  duration: %.1f ms\n", result.duration_ms);
    return 1;
  }

  std::printf("%s\n", s5run::value_to_display_string(result.value).c_str());
  std::fprintf(stderr, "  duration: %.1f ms\n", result.duration_ms);
  return 0;
}

int main(int argc, char **argv) {
  if (argc > 0 && argv[0] && argv[0][0] != '\0')
    g_bin = argv[0];
  if (argc < 2) {
    print_banner();
    print_usage();
    return 1;
  }

  std::string command = argv[1];

  if (command == "server") {
    return cmd_server(argc, argv);
  }
  if (command == "generate") {
    return cmd_generate(argc, argv);
  }
  if (command == "run") {
    return cmd_run(argc, argv);
  }
  if (command == "version" || command == "--version" || command == "-v") {
    std::printf("s5run v0.2.0\n");
    return 0;
  }
  if (command == "help" || command == "--help" || command == "-h") {
    if (argc > 2) {
      std::string sub = argv[2];
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
    }
    print_banner();
    print_usage();
    return 0;
  }

  print_banner();
  std::fprintf(stderr, "Unknown command: %s\n\n", command.c_str());
  print_usage();
  return 1;
}
