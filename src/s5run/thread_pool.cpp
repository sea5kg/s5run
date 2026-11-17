#include "thread_pool.h"

ThreadPool::ThreadPool(size_t n) {
  for (size_t i = 0; i < n; ++i)
    workers_.emplace_back([this] {
      worker_loop();
    });
}

ThreadPool::~ThreadPool() {
  {
    std::lock_guard lk(m_);
    stop_ = true;
  }
  cv_.notify_all();
  for (auto &t : workers_)
    t.join();
}

void ThreadPool::submit(std::function<void()> task) {
  {
    std::lock_guard lk(m_);
    tasks_.push(std::move(task));
  }
  cv_.notify_one();
}

void ThreadPool::worker_loop() {
  while (true) {
    std::function<void()> task;
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
      task();
    } catch (...) { /* log */
    }
  }
}