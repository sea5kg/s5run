/*
 * MIT License
 *
 *           ░░░░░░░
 *          ░██▓▓▓▓▒
 * ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 * ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 * ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
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
 */

#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <iostream>

#include "s5run_server.h"

static void print_usage() {
  std::printf(
    "s5run - remote script execution server\n"
    "\n"
    "Usage:\n"
    "  s5run --listen [host:]port [--threads N]\n"
    "\n"
    "Options:\n"
    "  --listen [host:]port  start the script server, e.g. --listen 0.0.0.0:7777\n"
    "                        (host defaults to 0.0.0.0, so --listen 7777 works too)\n"
    "  --threads N           worker threads (defaults to hardware concurrency)\n"
    "  --queue               enable the job queue (scripts without a client\n"
    "                        context, results stored in sqlite)\n"
    "  --db PATH             job queue database (default s5run-queue.sqlite3)\n"
    "  --ttl SECONDS         how long to keep finished job results (default 3600)\n"
  );
}

int main(int argc, char **argv) {


  std::cout << "           ░░░░░░░                            " << std::endl;
  std::cout << "          ░██▓▓▓▓▒                            " << std::endl;
  std::cout << " ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓  " << std::endl;
  std::cout << " ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒ " << std::endl;
  std::cout << "  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒ " << std::endl;
  std::cout << " ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒ " << std::endl;
  std::cout << "  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░ " << std::endl;


  std::string listen;
  size_t threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 4;

  std::string queue_db;
  int queue_ttl = 3600;
  bool need_help = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      need_help = true;
    } else if (arg == "--listen" && i + 1 < argc) {
      listen = argv[++i];
    } else if (arg == "--threads" && i + 1 < argc) {
      threads = static_cast<size_t>(std::atoi(argv[++i]));
    } else if (arg == "--queue") {
      queue_db = "s5run-queue.sqlite3";
    } else if (arg == "--db" && i + 1 < argc) {
      queue_db = argv[++i];
    } else if (arg == "--ttl" && i + 1 < argc) {
      queue_ttl = std::atoi(argv[++i]);
    } else {
      std::fprintf(stderr, "Unknown argument: %s\n\n", arg.c_str());
      print_usage();
      return 1;
    }
  }

  if (need_help) {
    print_usage();
    return 0;
  }

  if (listen.empty()) {
    print_usage();
    return 1;
  }

  s5run::server s;
  if (!queue_db.empty())
    s.enable_queue(queue_db, queue_ttl);
  return s.run(listen, threads);
}
