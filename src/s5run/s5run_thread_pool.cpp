/**********************************************************************************
 *
 *            ░░░░░░░
 *           ░██▓▓▓▓▒
 *  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 *  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 *  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
 *
 * MIT License
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
 ***********************************************************************************/

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
  m_queued.fetch_add(1);
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
      m_queued.fetch_sub(1);
    }
    m_busy.fetch_add(1);
    try {
      task->handle();
    } catch (...) { /* log */
    }
    m_busy.fetch_sub(1);
  }
}

} // namespace s5run
