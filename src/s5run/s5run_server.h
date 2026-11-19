
#pragma once

namespace s5run {

class server {
public:
  server();
  static void on_signal(int);

  // listen_address format: [host:]port
  int run(const std::string &listen_address, int thread_count);

private:

};

} // namespace s5run
