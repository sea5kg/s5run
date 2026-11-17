#include <thread>
#include <cstdlib>

#include "s5run_server.h"

int main(int argc, char **argv) {
  uint16_t port = 7777;
  size_t threads = std::thread::hardware_concurrency();
  if (threads == 0)
    threads = 4;
  if (argc > 1)
    port = static_cast<uint16_t>(std::atoi(argv[1]));
  if (argc > 2)
    threads = static_cast<size_t>(std::atoi(argv[2]));

  s5run::server s;
  return s.run(7777, threads);
}