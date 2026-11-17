#include "internal/s5run_thread_pool.h"

namespace s5run {

thread_pool::thread_pool(size_t n) {
  for (size_t i = 0; i < n; ++i)
    workers_.emplace_back([this] {
      worker_loop();
    });
}

thread_pool::~thread_pool() {
  {
    std::lock_guard lk(m_);
    stop_ = true;
  }
  cv_.notify_all();
  for (auto &t : workers_)
    t.join();
}

void thread_pool::submit(std::shared_ptr<thread_pool_task> task) {
  {
    std::lock_guard lk(m_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

void thread_pool::worker_loop() {
  while (true) {
    std::shared_ptr<thread_pool_task> task;
    {
      std::unique_lock lk(m_);
      cv_.wait(lk, [this] {
        return stop_ || !tasks_.empty();
      });
      if (stop_ && tasks_.empty())
        return;
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    try {
      task->handle();
    } catch (...) { /* log */
    }
  }
}

} // namespace s5run
