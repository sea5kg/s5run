/*
 * MIT License
 *
 *           ░░░░░░░
 *          ░██▓▓▓▓▒
 * ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 * ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 * ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ▒▒▒▒ ▒▒░ ░▒▒   ▒▒░
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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace s5run {

class job_queue {
public:
  enum class status : uint8_t {
    pending = 0,
    running = 1,
    done = 2,
    failed = 3,
  };

  struct job_info {
    std::string id;
    status state = status::pending;
    std::string script;
    std::string entry;
    std::vector<uint8_t> args;       // wire-encoded values
    bool has_result = false;
    bool result_ok = false;
    std::vector<uint8_t> result_value; // wire-encoded value
    std::string result_error;
  };

  job_queue(const std::string &db_path, int64_t ttl_seconds);
  ~job_queue();

  // Inserts a job and returns its uuid.
  std::string submit(const std::string &script, const std::string &entry,
                     const std::vector<uint8_t> &args);

  // Claims the oldest pending job; returns false when the queue is empty.
  bool claim_next(job_info &out);

  // Saves the execution result for a claimed job.
  void store_result(const std::string &id, bool ok,
                    const std::vector<uint8_t> &value, const std::string &error);

  // Looks a job up by id.
  bool get(const std::string &id, job_info &out);

  // Deletes finished jobs older than the ttl. Returns removed count.
  int cleanup();

private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

} // namespace s5run
