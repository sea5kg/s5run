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

#pragma once

#include <cstdint>

namespace s5run {

constexpr int S5RUN_PROTOCOL_VERSION = 3;

enum protocol_msg_type : uint8_t {
  M_EXEC_REQ = 0x01,      // client -> server: script, entry, args
  M_EXEC_RESP = 0x02,     // server -> client: ok, value, error
  M_CALLBACK_REQ = 0x03,  // server -> client: id, method, args
  M_CALLBACK_RESP = 0x04, // client -> server: id, ok, value, error
  M_REGISTER_FUNC = 0x05, // client -> server: name
  M_REGISTER_OBJ = 0x06,  // client -> server: name, this_id, type, methods
  M_REGISTER_TYPE = 0x07, // client -> server: name, methods
  M_REGISTER_RESP = 0x08, // server -> client: ok, error
  M_STATS_REQ = 0x09,     // client -> server: empty
  M_STATS_RESP = 0x0A,    // server -> client: threads, busy, queued, clients (u32 x4)
  M_JOB_SUBMIT = 0x0B,      // client -> server: script, entry, u32 count, values[]
  M_JOB_SUBMIT_RESP = 0x0C, // server -> client: u8 ok, uuid | error
  M_JOB_RESULT_REQ = 0x0D,  // client -> server: uuid
  M_JOB_RESULT_RESP = 0x0E, // server -> client: u8 status, [u8 ok, value | error]
  M_SERVER_HELLO = 0x0F,   // server -> client (first frame): plain text
                           // "s5run <server-version> <protocol-version>"

  // ---- provider protocol ----
  // The provider is a backend server: it publishes the schema context
  // metadata and serves API calls (reuses M_CALLBACK_REQ/M_CALLBACK_RESP
  // for invocations: req id, method, [this_id, args...]).

  M_PROVIDER_MOUNT_REQ = 0x12, // client -> server: mount context entries
                               //   from a remote provider:
                               //   str host; u16 port; u32 count; str names[]
  M_PROVIDER_MOUNT_RESP = 0x13, // server -> client: u8 ok, str error

  M_PROVIDER_META_REQ = 0x10, // client -> provider: str name ("" = list all)
  M_PROVIDER_META_RESP = 0x11, // provider -> client:
                              //   u8 count, entries[]:
                              //     u8 kind (1=function, 2=class, 3=global object)
                              //     str name
                              //     [function] str return_type; u32 argc;
                              //                { str arg_name; str arg_type }[]
                              //     [class/object] u32 method_count;
                              //                { str name; str return_type; u32 argc;
                              //                  { str arg_name; str arg_type }[] }[]
                              //                u32 prop_count;
                              //                { str name; str getter; str setter }[]
                              //     [object] i32 this_id
};

} // namespace s5run
