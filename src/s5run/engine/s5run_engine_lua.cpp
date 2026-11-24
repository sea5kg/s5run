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

#include "script_engine.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <cstdio>
#include <cstring>

namespace engine {

namespace {

// We only allow safe libraries. We are removing `io`, `os`, `package`, and `debug`.
void open_sandbox(lua_State *L) {
  luaL_requiref(L, "_G", luaopen_base, 1);
  lua_pop(L, 1);
  luaL_requiref(L, "string", luaopen_string, 1);
  lua_pop(L, 1);
  luaL_requiref(L, "table", luaopen_table, 1);
  lua_pop(L, 1);
  luaL_requiref(L, "math", luaopen_math, 1);
  lua_pop(L, 1);
  luaL_requiref(L, "utf8", luaopen_utf8, 1);
  lua_pop(L, 1);
  // Do NOT expose io, os, package, debug, or coroutine (at your discretion)

  // Remove dangerous global functions from `base`, if any exist
  lua_pushnil(L);
  lua_setglobal(L, "dofile");
  lua_pushnil(L);
  lua_setglobal(L, "loadfile");
  lua_pushnil(L);
  lua_setglobal(L, "require");
}

// JSON to Lua conversion
void push_json(lua_State *L, const json::variable &v) {
  if (v.is_null()) {
    lua_pushnil(L);
    return;
  }
  if (v.is_bool()) {
    lua_pushboolean(L, v.as_bool());
    return;
  }
  if (v.is_num()) {
    lua_pushnumber(L, v.as_num());
    return;
  }
  if (v.is_str()) {
    lua_pushlstring(L, v.as_str().data(), v.as_str().size());
    return;
  }
  if (v.is_arr()) {
    const auto &a = v.as_arr();
    lua_createtable(L, (int)a.size(), 0);
    for (size_t i = 0; i < a.size(); ++i) {
      push_json(L, a[i]);
      lua_rawseti(L, -2, (lua_Integer)i + 1);
    }
    return;
  }
  // object
  lua_createtable(L, 0, 0);
  for (const auto &[k, val] : v.as_obj()) {
    lua_pushlstring(L, k.data(), k.size());
    push_json(L, val);
    lua_rawset(L, -3);
  }
}

// Lua to JSON conversion (depth-limited)
json::variable pop_json(lua_State *L, int idx, int depth = 0) {
  if (depth > 32)
    throw std::runtime_error("json: nesting too deep");
  int t = lua_type(L, idx);
  switch (t) {
  case LUA_TNIL:
    return json::variable();
  case LUA_TBOOLEAN:
    return json::variable((bool)lua_toboolean(L, idx));
  case LUA_TNUMBER:
    return json::variable((double)lua_tonumber(L, idx));
  case LUA_TSTRING: {
    size_t len;
    const char *s = lua_tolstring(L, idx, &len);
    return json::variable(std::string(s, len));
  }
  case LUA_TTABLE: {
    // Distinguishing an array from an object: indices 1..n with no gaps and no other keys
    lua_Integer n = lua_rawlen(L, idx);
    bool is_arr = true;
    int count = 0;
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
      ++count;
      if (lua_type(L, -2) != LUA_TNUMBER) {
        is_arr = false;
      }
      lua_pop(L, 1);
      if (!is_arr)
        break;
    }
    if (is_arr && count == (int)n) {
      json::Array a;
      for (lua_Integer i = 1; i <= n; ++i) {
        lua_rawgeti(L, idx, i);
        a.push_back(pop_json(L, -1, depth + 1));
        lua_pop(L, 1);
      }
      return json::variable(std::move(a));
    }
    json::Object o;
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
      // key as a string
      std::string k;
      if (lua_type(L, -2) == LUA_TSTRING) {
        size_t len;
        const char *s = lua_tolstring(L, -2, &len);
        k.assign(s, len);
      } else if (lua_type(L, -2) == LUA_TNUMBER) {
        k = std::to_string((long long)lua_tointeger(L, -2));
      } else {
        lua_pop(L, 1); // value
        lua_pop(L, 1); // key
        continue;
      }
      o[k] = pop_json(L, -1, depth + 1);
      lua_pop(L, 1);
    }
    return json::variable(std::move(o));
  }
  default:
    return json::variable(); // function/userdata -> null
  }
}

std::string lua_err(lua_State *L) {
  const char *s = lua_tostring(L, -1);
  return s ? s : "unknown error";
}

} // namespace

ExecResult execute(const std::string &script, const std::string &entry, const json::Array &args) {
  ExecResult res;
  res.ok = false;

  lua_State *L = luaL_newstate();
  if (!L) {
    res.error = "cannot create lua_State";
    return res;
  }

  // Let’s limit the memory (for example, 16 MiB per state)
  lua_setallocf(
    L,
    [](void *ud, void *ptr, size_t osize, size_t nsize) -> void * {
      auto *used = static_cast<size_t *>(ud);
      if (nsize == 0) {
        *used -= osize;
        free(ptr);
        return nullptr;
      }
      size_t delta = nsize - osize;
      if (delta > 0 && *used + delta > 16 * 1024 * 1024)
        return nullptr;
      void *p = realloc(ptr, nsize);
      if (p)
        *used += delta;
      return p;
    },
    nullptr
  );

  open_sandbox(L);

  if (luaL_loadbufferx(L, script.data(), script.size(), "@script", "t") != LUA_OK) {
    res.error = "compile: " + lua_err(L);
    lua_close(L);
    return res;
  }
  // Execute the script (function definitions)
  if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    res.error = "run: " + lua_err(L);
    lua_close(L);
    return res;
  }

  // We retrieve the entry function
  lua_getglobal(L, entry.c_str());
  if (!lua_isfunction(L, -1)) {
    res.error = "entry not found: " + entry;
    lua_close(L);
    return res;
  }

  // Arguments
  for (const auto &a : args)
    push_json(L, a);

  int nargs = (int)args.size();
  if (lua_pcall(L, nargs, 1, 0) != LUA_OK) {
    res.error = "call: " + lua_err(L);
    lua_close(L);
    return res;
  }

  try {
    res.value = pop_json(L, -1);
    res.ok = true;
  } catch (const std::exception &e) {
    res.error = std::string("serialize: ") + e.what();
  }

  lua_close(L);
  return res;
}

} // namespace engine
