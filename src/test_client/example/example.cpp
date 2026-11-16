// AUTO-GENERATED. DO NOT EDIT.
#include "example.h"
#include <cstring>

namespace example {

namespace wire {
void put_u8(std::vector<uint8_t>& b, uint8_t v)  { b.push_back(v); }
void put_bool(std::vector<uint8_t>& b, bool v)    { b.push_back(v ? 1 : 0); }
void put_u32(std::vector<uint8_t>& b, uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back((v >> (8*i)) & 0xFF);
}
void put_u64(std::vector<uint8_t>& b, uint64_t v) {
    for (int i = 0; i < 8; ++i) b.push_back((v >> (8*i)) & 0xFF);
}
void put_f64(std::vector<uint8_t>& b, double v) {
    uint64_t u; std::memcpy(&u, &v, 8); put_u64(b, u);
}
void put_str(std::vector<uint8_t>& b, const std::string& s) {
    put_u32(b, (uint32_t)s.size());
    b.insert(b.end(), s.begin(), s.end());
}

uint8_t  get_u8 (const uint8_t*& p) { return *p++; }
bool     get_bool(const uint8_t*& p) { return *p++ != 0; }
uint32_t get_u32(const uint8_t*& p) {
    uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= (uint32_t)p[i] << (8*i);
    p += 4; return v;
}
uint64_t get_u64(const uint8_t*& p) {
    uint64_t v = 0; for (int i = 0; i < 8; ++i) v |= (uint64_t)p[i] << (8*i);
    p += 8; return v;
}
double   get_f64(const uint8_t*& p) {
    uint64_t u = get_u64(p); double d; std::memcpy(&d, &u, 8); return d;
}
std::string get_str(const uint8_t*& p) {
    uint32_t n = get_u32(p); std::string s((const char*)p, n); p += n; return s;
}
} // namespace wire

std::vector<uint8_t> GetFileRequest::encode() const {
    std::vector<uint8_t> b;
    wire::put_str(b, this->filepath);
    return b;
}

GetFileRequest GetFileRequest::decode(const std::vector<uint8_t>& b) {
    GetFileRequest r;
    const uint8_t* p = b.data();
    r.filepath = wire::get_str(p);
    return r;
}

std::vector<uint8_t> GetTimeNowRequest::encode() const {
    std::vector<uint8_t> b;
    return b;
}

GetTimeNowRequest GetTimeNowRequest::decode(const std::vector<uint8_t>& b) {
    GetTimeNowRequest r;
    const uint8_t* p = b.data();
    return r;
}

std::vector<uint8_t> GetTimeNowResponse::encode() const {
    std::vector<uint8_t> b;
    wire::put_u64(b, this->value);
    return b;
}

GetTimeNowResponse GetTimeNowResponse::decode(const std::vector<uint8_t>& b) {
    GetTimeNowResponse r;
    const uint8_t* p = b.data();
    r.value = wire::get_u64(p);
    return r;
}

std::vector<uint8_t> HasFileRequest::encode() const {
    std::vector<uint8_t> b;
    wire::put_str(b, this->filepath);
    return b;
}

HasFileRequest HasFileRequest::decode(const std::vector<uint8_t>& b) {
    HasFileRequest r;
    const uint8_t* p = b.data();
    r.filepath = wire::get_str(p);
    return r;
}

std::vector<uint8_t> HasFileResponse::encode() const {
    std::vector<uint8_t> b;
    wire::put_bool(b, this->value);
    return b;
}

HasFileResponse HasFileResponse::decode(const std::vector<uint8_t>& b) {
    HasFileResponse r;
    const uint8_t* p = b.data();
    r.value = wire::get_bool(p);
    return r;
}

std::vector<uint8_t> GetFileListRequest::encode() const {
    std::vector<uint8_t> b;
    wire::put_str(b, this->filepath);
    return b;
}

GetFileListRequest GetFileListRequest::decode(const std::vector<uint8_t>& b) {
    GetFileListRequest r;
    const uint8_t* p = b.data();
    r.filepath = wire::get_str(p);
    return r;
}

std::vector<uint8_t> GetFileListResponse::encode() const {
    std::vector<uint8_t> b;
    wire::put_u32(b, (uint32_t)this->value.size());
    for (const auto& _e : this->value) {
        wire::put_str(b, _e);
    }
    return b;
}

GetFileListResponse GetFileListResponse::decode(const std::vector<uint8_t>& b) {
    GetFileListResponse r;
    const uint8_t* p = b.data();
    {
        uint32_t _n = wire::get_u32(p);
        r.value.reserve(_n);
        for (uint32_t _i = 0; _i < _n; ++_i) {
            std::string _e{};
            _e = wire::get_str(p);
            r.value.push_back(std::move(_e));
        }
    }
    return r;
}

std::vector<uint8_t> GetSizeRequest::encode() const {
    std::vector<uint8_t> b;
    wire::put_u64(b, this_id);
    return b;
}

GetSizeRequest GetSizeRequest::decode(const std::vector<uint8_t>& b) {
    GetSizeRequest r;
    const uint8_t* p = b.data();
    r.this_id = wire::get_u64(p);
    return r;
}

std::vector<uint8_t> GetSizeResponse::encode() const {
    std::vector<uint8_t> b;
    wire::put_u64(b, this->value);
    return b;
}

GetSizeResponse GetSizeResponse::decode(const std::vector<uint8_t>& b) {
    GetSizeResponse r;
    const uint8_t* p = b.data();
    r.value = wire::get_u64(p);
    return r;
}

std::vector<uint8_t> ReadBytesRequest::encode() const {
    std::vector<uint8_t> b;
    wire::put_u64(b, this_id);
    wire::put_u64(b, this->readBytes);
    return b;
}

ReadBytesRequest ReadBytesRequest::decode(const std::vector<uint8_t>& b) {
    ReadBytesRequest r;
    const uint8_t* p = b.data();
    r.this_id = wire::get_u64(p);
    r.readBytes = wire::get_u64(p);
    return r;
}

std::vector<uint8_t> ReadBytesResponse::encode() const {
    std::vector<uint8_t> b;
    wire::put_u32(b, (uint32_t)this->value.size());
    for (const auto& _e : this->value) {
        wire::put_u8(b, _e);
    }
    return b;
}

ReadBytesResponse ReadBytesResponse::decode(const std::vector<uint8_t>& b) {
    ReadBytesResponse r;
    const uint8_t* p = b.data();
    {
        uint32_t _n = wire::get_u32(p);
        r.value.reserve(_n);
        for (uint32_t _i = 0; _i < _n; ++_i) {
            uint8_t _e{};
            _e = wire::get_u8(p);
            r.value.push_back(std::move(_e));
        }
    }
    return r;
}

} // namespace example