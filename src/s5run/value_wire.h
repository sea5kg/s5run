#pragma once

#include "value.h"
#include "wire.h"

namespace example {

void value_to_wire(const Value& v, wire::Writer& w);

Value value_from_wire(wire::Reader& r);

inline std::vector<uint8_t> value_encode(const Value& v) {
    wire::Writer w;
    value_to_wire(v, w);
    return w.data();
}

inline Value value_decode(const uint8_t* p, size_t n) {
    wire::Reader r(p, n);
    return value_from_wire(r);
}

} // namespace example
