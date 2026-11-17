#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace s5run {

class thread_pool_task {
public:
  virtual void handle() = 0;
};

class thread_pool {
public:
  explicit thread_pool(size_t n);
  ~thread_pool();

  void submit(std::shared_ptr<thread_pool_task> task);

private:
  void worker_loop();

  std::vector<std::thread> workers_;
  std::queue<std::shared_ptr<thread_pool_task>> tasks_;
  std::mutex m_;
  std::condition_variable cv_;
  bool stop_ = false;
};

} // namespace s5run