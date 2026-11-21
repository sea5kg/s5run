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

#include "s5run_queue.h"

#include "sqlite3.h"

#include <chrono>
#include <cstring>
#include <mutex>
#include <random>
#include <sstream>

namespace s5run {

namespace {

int64_t now_unix() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string generate_uuid() {
  static std::random_device rd;
  static std::mt19937_64 gen(rd());
  static std::uniform_int_distribution<uint64_t> dist;
  uint64_t a = dist(gen);
  uint64_t b = dist(gen);
  char buf[40];
  std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx",
                (uint32_t)(a >> 32), (uint16_t)(a >> 16), (uint16_t)a,
                (uint16_t)(b >> 48), (unsigned long long)(b & 0xFFFFFFFFFFFFULL));
  return buf;
}

std::vector<uint8_t> blob_to_vec(const void *p, int n) {
  if (!p || n <= 0)
    return {};
  const auto *bytes = static_cast<const uint8_t *>(p);
  return std::vector<uint8_t>(bytes, bytes + n);
}

} // namespace

struct job_queue::impl {
  sqlite3 *db = nullptr;
  std::mutex mtx;
  int64_t ttl_seconds = 3600;

  ~impl() {
    if (db)
      sqlite3_close(db);
  }
};

job_queue::job_queue(const std::string &db_path, int64_t ttl_seconds)
    : m_impl(new impl()) {
  m_impl->ttl_seconds = ttl_seconds;
  int rc = sqlite3_open(db_path.c_str(), &m_impl->db);
  if (rc != SQLITE_OK) {
    std::string err = m_impl->db ? sqlite3_errmsg(m_impl->db) : "cannot open";
    throw std::runtime_error("job_queue: open " + db_path + " failed: " + err);
  }
  sqlite3_exec(m_impl->db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
  sqlite3_exec(m_impl->db, "PRAGMA busy_timeout=3000;", nullptr, nullptr, nullptr);

  const char *schema =
      "CREATE TABLE IF NOT EXISTS jobs ("
      "  id TEXT PRIMARY KEY,"
      "  created_at INTEGER NOT NULL,"
      "  status INTEGER NOT NULL,"
      "  script TEXT NOT NULL,"
      "  entry TEXT NOT NULL,"
      "  args BLOB NOT NULL,"
      "  result_ok INTEGER,"
      "  result_value BLOB,"
      "  result_error TEXT,"
      "  done_at INTEGER);"
      "CREATE INDEX IF NOT EXISTS idx_jobs_status ON jobs(status, created_at);";
  char *err = nullptr;
  rc = sqlite3_exec(m_impl->db, schema, nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    std::string msg = err ? err : "unknown";
    sqlite3_free(err);
    throw std::runtime_error("job_queue: schema failed: " + msg);
  }

  // Restart: return interrupted jobs to the queue.
  sqlite3_exec(m_impl->db, "UPDATE jobs SET status=0 WHERE status=1;",
               nullptr, nullptr, nullptr);
}

job_queue::~job_queue() = default;

std::string job_queue::submit(const std::string &script, const std::string &entry,
                              const std::vector<uint8_t> &args) {
  std::lock_guard<std::mutex> lock(m_impl->mtx);
  const std::string id = generate_uuid();
  const char *sql =
      "INSERT INTO jobs(id, created_at, status, script, entry, args)"
      " VALUES(?,?,0,?,?,?);";
  sqlite3_stmt *st = nullptr;
  if (sqlite3_prepare_v2(m_impl->db, sql, -1, &st, nullptr) != SQLITE_OK)
    throw std::runtime_error(std::string("job_queue: submit prepare: ") + sqlite3_errmsg(m_impl->db));
  sqlite3_bind_text(st, 1, id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st, 2, now_unix());
  sqlite3_bind_text(st, 3, script.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(st, 4, entry.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_blob(st, 5, args.empty() ? (const void *)"" : (const void *)args.data(), (int)args.size(), SQLITE_TRANSIENT);
  int rc = sqlite3_step(st);
  sqlite3_finalize(st);
  if (rc != SQLITE_DONE)
    throw std::runtime_error(std::string("job_queue: submit failed: ") + sqlite3_errmsg(m_impl->db));
  return id;
}

bool job_queue::claim_next(job_info &out) {
  std::lock_guard<std::mutex> lock(m_impl->mtx);

  const char *select =
      "SELECT id, script, entry, args FROM jobs WHERE status=0"
      " ORDER BY created_at ASC LIMIT 1;";
  sqlite3_stmt *st = nullptr;
  if (sqlite3_prepare_v2(m_impl->db, select, -1, &st, nullptr) != SQLITE_OK)
    return false;
  int rc = sqlite3_step(st);
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(st);
    return false;
  }
  job_info job;
  job.id = (const char *)sqlite3_column_text(st, 0);
  job.script = (const char *)sqlite3_column_text(st, 1);
  job.entry = (const char *)sqlite3_column_text(st, 2);
  job.args = blob_to_vec(sqlite3_column_blob(st, 3), sqlite3_column_bytes(st, 3));
  sqlite3_finalize(st);

  const char *update = "UPDATE jobs SET status=1 WHERE id=? AND status=0;";
  if (sqlite3_prepare_v2(m_impl->db, update, -1, &st, nullptr) != SQLITE_OK)
    return false;
  sqlite3_bind_text(st, 1, job.id.c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(st);
  sqlite3_finalize(st);
  if (rc != SQLITE_DONE)
    return false; // someone else claimed it

  out = std::move(job);
  return true;
}

void job_queue::store_result(const std::string &id, bool ok,
                             const std::vector<uint8_t> &value, const std::string &error) {
  std::lock_guard<std::mutex> lock(m_impl->mtx);
  const char *sql =
      "UPDATE jobs SET status=?, result_ok=?, result_value=?, result_error=?, done_at=?"
      " WHERE id=?;";
  sqlite3_stmt *st = nullptr;
  if (sqlite3_prepare_v2(m_impl->db, sql, -1, &st, nullptr) != SQLITE_OK)
    return;
  sqlite3_bind_int(st, 1, ok ? 2 : 3);
  sqlite3_bind_int(st, 2, ok ? 1 : 0);
  sqlite3_bind_blob(st, 3, value.empty() ? (const void *)"" : (const void *)value.data(), (int)value.size(), SQLITE_TRANSIENT);
  sqlite3_bind_text(st, 4, error.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st, 5, now_unix());
  sqlite3_bind_text(st, 6, id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_step(st);
  sqlite3_finalize(st);
}

bool job_queue::get(const std::string &id, job_info &out) {
  std::lock_guard<std::mutex> lock(m_impl->mtx);
  const char *sql =
      "SELECT status, script, entry, args, result_ok, result_value, result_error"
      " FROM jobs WHERE id=?;";
  sqlite3_stmt *st = nullptr;
  if (sqlite3_prepare_v2(m_impl->db, sql, -1, &st, nullptr) != SQLITE_OK)
    return false;
  sqlite3_bind_text(st, 1, id.c_str(), -1, SQLITE_TRANSIENT);
  int rc = sqlite3_step(st);
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(st);
    return false;
  }
  out = job_info();
  out.id = id;
  out.state = (status)sqlite3_column_int(st, 0);
  out.script = (const char *)sqlite3_column_text(st, 1);
  out.entry = (const char *)sqlite3_column_text(st, 2);
  out.args = blob_to_vec(sqlite3_column_blob(st, 3), sqlite3_column_bytes(st, 3));
  if (out.state == status::done || out.state == status::failed) {
    out.has_result = true;
    out.result_ok = sqlite3_column_int(st, 4) != 0;
    out.result_value = blob_to_vec(sqlite3_column_blob(st, 5), sqlite3_column_bytes(st, 5));
    const char *err = (const char *)sqlite3_column_text(st, 6);
    out.result_error = err ? err : "";
  }
  sqlite3_finalize(st);
  return true;
}

int job_queue::cleanup() {
  std::lock_guard<std::mutex> lock(m_impl->mtx);
  const char *sql = "DELETE FROM jobs WHERE done_at IS NOT NULL AND done_at < ?;";
  sqlite3_stmt *st = nullptr;
  if (sqlite3_prepare_v2(m_impl->db, sql, -1, &st, nullptr) != SQLITE_OK)
    return 0;
  sqlite3_bind_int64(st, 1, now_unix() - m_impl->ttl_seconds);
  sqlite3_step(st);
  int removed = sqlite3_changes(m_impl->db);
  sqlite3_finalize(st);
  return removed;
}

} // namespace s5run
