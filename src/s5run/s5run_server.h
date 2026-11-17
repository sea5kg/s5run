
#pragma once

namespace s5run {

class server {
public:
  server();
  static void on_signal(int);

  int run(uint16_t listen_port, int thread_count);

private:

};

} // namespace s5run