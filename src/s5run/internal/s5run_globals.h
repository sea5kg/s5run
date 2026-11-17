#pragma once

#include <cstdint>

namespace s5run {

enum protocol_msg_type : uint8_t {
    M_EXEC_REQ       = 0x01,  // client -> server: script, entry, args
    M_EXEC_RESP      = 0x02,  // server -> client: ok, value, error
    M_CALLBACK_REQ   = 0x03,  // server -> client: id, method, args
    M_CALLBACK_RESP  = 0x04,  // client -> server: id, ok, value, error
    M_REGISTER_FUNC  = 0x05,  // client -> server: name
    M_REGISTER_OBJ   = 0x06,  // client -> server: name, this_id, type, methods
    M_REGISTER_TYPE  = 0x07,  // client -> server: name, methods
    M_REGISTER_RESP  = 0x08,  // server -> client: ok, error
};

} // namespace s5run