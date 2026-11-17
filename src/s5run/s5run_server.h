
#pragma once

namespace s5run {

class server {
public:
  server();
  int run(uint16_t listen_port, int thread_count);
};

} // namespace s5run