#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

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
      "  --threads N           worker threads (defaults to hardware concurrency)\n");
}

int main(int argc, char **argv) {
  std::string listen;
  size_t threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 4;

  bool need_help = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      need_help = true;
    } else if (arg == "--listen" && i + 1 < argc) {
      listen = argv[++i];
    } else if (arg == "--threads" && i + 1 < argc) {
      threads = static_cast<size_t>(std::atoi(argv[++i]));
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
  return s.run(listen, threads);
}
