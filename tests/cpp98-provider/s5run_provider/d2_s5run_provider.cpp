// AUTO-GENERATED. DO NOT EDIT.
#include "d2_s5run_provider.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

// Prevent Win32 API macros (windows.h) from renaming schema methods
// (e.g. #define SetProp SetPropW breaks method declarations/calls).
#undef SetProp
#undef GetProp
#undef RemoveProp
#undef CreateFile
#undef DeleteFile
#undef CopyFile
#undef MoveFile
#undef GetObject
#undef LoadLibrary
#undef FreeLibrary
#undef DrawText
#undef LoadIcon
#undef LoadCursor
#undef MessageBox
#undef SendMessage
#undef PostMessage
#undef SetFocus
#undef GetFocus
#undef ShowWindow
#undef UpdateWindow
#undef FindWindow
#undef GetTickCount

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <stdexcept>

namespace s5run_provider {

namespace {

#ifdef _WIN32
typedef SOCKET sock_t;
#define S5RUN_BAD_SOCKET INVALID_SOCKET

static bool s5run_sockets_init() {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}
static void s5run_sockets_shutdown() { WSACleanup(); }
static void s5run_close_socket(sock_t s) { closesocket(s); }
static void s5run_set_nodelay(sock_t s) {
    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag));
}
static void s5run_set_recv_timeout(sock_t s, int ms) {
    DWORD t = (DWORD)ms;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&t, sizeof(t));
}
static bool s5run_send_all(sock_t s, const char *p, std::size_t n) {
    while (n > 0) {
        int sent = send(s, p, (int)n, 0);
        if (sent <= 0) return false;
        p += sent; n -= (std::size_t)sent;
    }
    return true;
}
static bool s5run_recv_exact(sock_t s, char *p, std::size_t n) {
    while (n > 0) {
        int got = recv(s, p, (int)n, 0);
        if (got == 0) return false;
        if (got < 0) return false;
        p += got; n -= (std::size_t)got;
    }
    return true;
}
static sock_t s5run_connect_socket(const std::string &host, int port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16)port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) return S5RUN_BAD_SOCKET;
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == S5RUN_BAD_SOCKET) return S5RUN_BAD_SOCKET;
    if (connect(s, (sockaddr *)&addr, sizeof(addr)) != 0) {
        s5run_close_socket(s);
        return S5RUN_BAD_SOCKET;
    }
    return s;
}

#else
typedef int sock_t;
#define S5RUN_BAD_SOCKET (-1)

static bool s5run_sockets_init() { return true; }
static void s5run_sockets_shutdown() {}
static void s5run_close_socket(sock_t s) { ::close(s); }
static void s5run_set_nodelay(sock_t s) {
    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}
static void s5run_set_recv_timeout(sock_t s, int ms) {
    timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}
static bool s5run_send_all(sock_t s, const char *p, std::size_t n) {
    while (n > 0) {
        ssize_t sent = ::send(s, p, n, MSG_NOSIGNAL);
        if (sent <= 0) {
            if (sent < 0 && errno == EINTR) continue;
            return false;
        }
        p += sent; n -= (std::size_t)sent;
    }
    return true;
}
static bool s5run_recv_exact(sock_t s, char *p, std::size_t n) {
    while (n > 0) {
        ssize_t got = ::recv(s, p, n, 0);
        if (got == 0) return false;
        if (got < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += got; n -= (std::size_t)got;
    }
    return true;
}
static sock_t s5run_connect_socket(const std::string &host, int port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16)port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) return S5RUN_BAD_SOCKET;
    sock_t s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == S5RUN_BAD_SOCKET) return S5RUN_BAD_SOCKET;
    if (::connect(s, (sockaddr *)&addr, sizeof(addr)) != 0) {
        s5run_close_socket(s);
        return S5RUN_BAD_SOCKET;
    }
    return s;
}
#endif

static std::string num_to_str(long long v) {
    char buf[32]; std::sprintf(buf, "%lld", v); return std::string(buf);
}
static std::string num_to_str(int v) {
    char buf[32]; std::sprintf(buf, "%d", v); return std::string(buf);
}
static std::string num_to_str(u32 v) {
    char buf[32]; std::sprintf(buf, "%u", v); return std::string(buf);
}

} // namespace

const s5run_class_metadata &base_context_class_MassageCrew_Queue::metadata() {
    static s5run_class_metadata meta;
    if (meta.name.empty()) {
        meta.name = "MassageCrew_Queue";
        {
            s5run_method_metadata mm;
            mm.name = "publish";
            mm.return_type = "void";
            mm.args.push_back(s5run_arg_metadata("msg_id", "string"));
            mm.args.push_back(s5run_arg_metadata("msg_text", "string"));
            meta.methods.push_back(mm);
        }
        {
            s5run_method_metadata mm;
            mm.name = "consume";
            mm.return_type = "var";
            meta.methods.push_back(mm);
        }
    }
    return meta;
}
const s5run_class_metadata *base_context_class_MassageCrew_Queue::s5run_metadata() const { return &metadata(); }

const s5run_function_metadata &base_context_function_getQueue::metadata() {
    static s5run_function_metadata meta;
    if (meta.name.empty()) {
        meta.name = "getQueue";
        meta.return_type = "MassageCrew_Queue";
        meta.args.push_back(s5run_arg_metadata("queue_name", "string"));
    }
    return meta;
}
const s5run_function_metadata *base_context_function_getQueue::s5run_metadata() const { return &metadata(); }

namespace {

enum {
    T_UNDEFINED = 0x00, T_BOOL = 0x01, T_INT8 = 0x02, T_INT16 = 0x03,
    T_INT32 = 0x04, T_INT64 = 0x05, T_UINT8 = 0x06, T_UINT16 = 0x07,
    T_UINT32 = 0x08, T_UINT64 = 0x09, T_FLOAT32 = 0x0A, T_FLOAT64 = 0x0B,
    T_STRING = 0x0C, T_ARRAY = 0x0D, T_OBJECT = 0x0E, T_REF = 0x0F
};
enum {
    S_LEN_1 = 0x00, S_LEN_2 = 0x01, S_LEN_4 = 0x02
};
enum {
    M_EXEC_REQ = 0x01, M_EXEC_RESP = 0x02, M_CALLBACK_REQ = 0x03,
    M_CALLBACK_RESP = 0x04, M_REGISTER_FUNC = 0x05, M_REGISTER_OBJ = 0x06,
    M_REGISTER_TYPE = 0x07, M_REGISTER_RESP = 0x08,
    M_STATS_REQ = 0x09, M_STATS_RESP = 0x0A, M_SERVER_HELLO = 0x0F
};

class Writer {
public:
    const std::vector<u8> &data() const { return m_buf; }
    void put_u8(u8 v) { m_buf.push_back(v); }
    void put_u16(u16 v) {
        m_buf.push_back((u8)(v & 0xFF));
        m_buf.push_back((u8)((v >> 8) & 0xFF));
    }
    void put_u32(u32 v) {
        for (int i = 0; i < 4; ++i) m_buf.push_back((u8)((v >> (8 * i)) & 0xFF));
    }
    void put_u64(u64 v) {
        for (int i = 0; i < 8; ++i) m_buf.push_back((u8)((v >> (8 * i)) & 0xFF));
    }
    void put_i8(i8 v) { put_u8((u8)v); }
    void put_i16(i16 v) { put_u16((u16)v); }
    void put_i32(i32 v) { put_u32((u32)v); }
    void put_i64(i64 v) { put_u64((u64)v); }
    void put_f64(double v) {
        u64 u; std::memcpy(&u, &v, 8); put_u64(u);
    }
    void put_string(const std::string &s) {
        std::size_t n = s.size();
        if (n <= 0xFF) { put_u8(S_LEN_1); put_u8((u8)n); }
        else if (n <= 0xFFFF) { put_u8(S_LEN_2); put_u16((u16)n); }
        else { put_u8(S_LEN_4); put_u32((u32)n); }
        for (std::size_t i = 0; i < n; ++i) m_buf.push_back((u8)s[i]);
    }
private:
    std::vector<u8> m_buf;
};

class Reader {
public:
    Reader(const u8 *p, std::size_t n) : m_p(p), m_end(p + n) {}
    u8 get_u8() { check(1); return *m_p++; }
    u16 get_u16() {
        check(2);
        u16 v = (u16)(m_p[0] | (m_p[1] << 8)); m_p += 2; return v;
    }
    u32 get_u32() {
        check(4);
        u32 v = 0;
        for (int i = 0; i < 4; ++i) v |= (u32)m_p[i] << (8 * i);
        m_p += 4; return v;
    }
    u64 get_u64() {
        check(8);
        u64 v = 0;
        for (int i = 0; i < 8; ++i) v |= (u64)m_p[i] << (8 * i);
        m_p += 8; return v;
    }
    i8 get_i8() { return (i8)get_u8(); }
    i16 get_i16() { return (i16)get_u16(); }
    i32 get_i32() { return (i32)get_u32(); }
    i64 get_i64() { return (i64)get_u64(); }
    double get_f64() {
        u64 u = get_u64();
        double d; std::memcpy(&d, &u, 8); return d;
    }
    std::string get_string() {
        u8 len_size = get_u8();
        std::size_t n = 0;
        if (len_size == S_LEN_1) n = get_u8();
        else if (len_size == S_LEN_2) n = get_u16();
        else if (len_size == S_LEN_4) n = get_u32();
        else throw std::runtime_error("wire: bad string length size");
        check(n);
        std::string s(reinterpret_cast<const char *>(m_p), n);
        m_p += n; return s;
    }
private:
    void check(std::size_t n) const {
        if (m_p + n > m_end) throw std::runtime_error("wire: unexpected end of buffer");
    }
    const u8 *m_p;
    const u8 *m_end;
};

i64 read_int(Reader &r) {
    u8 tag = r.get_u8();
    switch (tag) {
    case T_INT8: return (i64)r.get_i8();
    case T_INT16: return (i64)r.get_i16();
    case T_INT32: return (i64)r.get_i32();
    case T_INT64: return r.get_i64();
    case T_UINT8: return (i64)r.get_u8();
    case T_UINT16: return (i64)r.get_u16();
    case T_UINT32: return (i64)r.get_u32();
    case T_UINT64: return (i64)r.get_u64();
    default: throw std::runtime_error("wire: expected an integer");
    }
}

void write_int(Writer &w, i64 v) {
    if (v >= -128 && v <= 127) { w.put_u8(T_INT8); w.put_i8((i8)v); }
    else if (v >= -32768 && v <= 32767) { w.put_u8(T_INT16); w.put_i16((i16)v); }
    else if (v >= -2147483648LL && v <= 2147483647LL) { w.put_u8(T_INT32); w.put_i32((i32)v); }
    else { w.put_u8(T_INT64); w.put_i64(v); }
}

i32 read_ref(Reader &r) {
    if (r.get_u8() != T_REF) throw std::runtime_error("wire: expected an object reference");
    i32 this_id = r.get_i32();
    r.get_string(); // skip the type name
    return this_id;
}

} // namespace

static std::string read_string(Reader &r) {
    if (r.get_u8() != T_STRING) throw std::runtime_error("wire: expected a string");
    return r.get_string();
}

static void write_string(Writer &w, const std::string &v) {
    w.put_u8(T_STRING); w.put_string(v);
}


static void encode_value(Writer &w, const s5run_variable &v) {
    switch (v.get_kind()) {
    case s5run_variable::K_UNDEFINED:
    case s5run_variable::K_NULL: w.put_u8(T_UNDEFINED); break;
    case s5run_variable::K_BOOL: w.put_u8(T_BOOL); w.put_u8(v.as_bool() ? 1 : 0); break;
    case s5run_variable::K_INT: write_int(w, v.as_int()); break;
    case s5run_variable::K_DOUBLE: w.put_u8(T_FLOAT64); w.put_f64(v.as_double()); break;
    case s5run_variable::K_STRING: w.put_u8(T_STRING); w.put_string(v.as_string()); break;
    case s5run_variable::K_ARRAY: {
        w.put_u8(T_ARRAY);
        const std::vector<s5run_variable> &a = v.as_array();
        w.put_u32((u32)a.size());
        for (std::size_t i = 0; i < a.size(); ++i) encode_value(w, a[i]);
        break;
    }
    case s5run_variable::K_OBJECT: {
        w.put_u8(T_OBJECT);
        const std::map<std::string, s5run_variable> &o = v.as_object();
        w.put_u32((u32)o.size());
        for (std::map<std::string, s5run_variable>::const_iterator it = o.begin(); it != o.end(); ++it) {
            w.put_u8(T_STRING); w.put_string(it->first);
            encode_value(w, it->second);
        }
        break;
    }
    case s5run_variable::K_REF: {
        w.put_u8(T_REF); w.put_i32(v.ref_this_id()); w.put_string(v.ref_type());
        break;
    }
    }
}

static s5run_variable decode_value(Reader &r) {
    u8 tag = r.get_u8();
    switch (tag) {
    case T_UNDEFINED: return s5run_variable();
    case T_BOOL: return s5run_variable(r.get_u8() != 0);
    case T_INT8: return s5run_variable((i64)r.get_i8());
    case T_INT16: return s5run_variable((i64)r.get_i16());
    case T_INT32: return s5run_variable((i64)r.get_i32());
    case T_INT64: return s5run_variable(r.get_i64());
    case T_UINT8: return s5run_variable((i64)r.get_u8());
    case T_UINT16: return s5run_variable((i64)r.get_u16());
    case T_UINT32: return s5run_variable((i64)r.get_u32());
    case T_UINT64: return s5run_variable((i64)r.get_u64());
    case T_FLOAT64: return s5run_variable(r.get_f64());
    case T_STRING: return s5run_variable(r.get_string());
    case T_ARRAY: {
        u32 n = r.get_u32();
        std::vector<s5run_variable> a; a.reserve(n);
        for (u32 i = 0; i < n; ++i) a.push_back(decode_value(r));
        return s5run_variable(a);
    }
    case T_OBJECT: {
        u32 n = r.get_u32();
        std::map<std::string, s5run_variable> o;
        for (u32 i = 0; i < n; ++i) {
            if (r.get_u8() != T_STRING) throw std::runtime_error("wire: expected a string key");
            std::string key = r.get_string();
            o[key] = decode_value(r);
        }
        return s5run_variable(o);
    }
    case T_REF: {
        i32 this_id = r.get_i32();
        std::string type = r.get_string();
        return s5run_variable::make_ref(this_id, type);
    }
    default: throw std::runtime_error("wire: unknown value tag");
    }
}

static std::string _value_to_string(const s5run_variable &v) {
    switch (v.get_kind()) {
    case s5run_variable::K_NULL: return "null";
    case s5run_variable::K_UNDEFINED: return "undefined";
    case s5run_variable::K_BOOL: return v.as_bool() ? "true" : "false";
    case s5run_variable::K_INT: return num_to_str(v.as_int());
    case s5run_variable::K_DOUBLE: {
        char buf[64]; std::sprintf(buf, "%g", v.as_double());
        return std::string(buf);
    }
    case s5run_variable::K_STRING: return v.as_string();
    default: throw std::runtime_error("wire: expected a scalar argument");
    }
}

s5run_variable::s5run_variable() : m_kind(K_UNDEFINED), m_b(false), m_i(0), m_d(0.0), m_a(0), m_o(0) {}
s5run_variable::s5run_variable(bool b) : m_kind(K_BOOL), m_b(b), m_i(0), m_d(0.0), m_a(0), m_o(0) {}
s5run_variable::s5run_variable(i64 i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0), m_a(0), m_o(0) {}
s5run_variable::s5run_variable(double d) : m_kind(K_DOUBLE), m_b(false), m_i(0), m_d(d), m_a(0), m_o(0) {}
s5run_variable::s5run_variable(const char *s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s), m_a(0), m_o(0) {}
s5run_variable::s5run_variable(const std::string &s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s), m_a(0), m_o(0) {}
s5run_variable::s5run_variable(const std::vector<s5run_variable> &a) : m_kind(K_ARRAY), m_b(false), m_i(0), m_d(0.0), m_a(new std::vector<s5run_variable>(a)), m_o(0) {}
s5run_variable::s5run_variable(const std::map<std::string, s5run_variable> &o) : m_kind(K_OBJECT), m_b(false), m_i(0), m_d(0.0), m_a(0), m_o(new std::map<std::string, s5run_variable>(o)) {}
s5run_variable::s5run_variable(const s5run_variable &other) : m_kind(K_UNDEFINED), m_b(false), m_i(0), m_d(0.0), m_a(0), m_o(0) { copy_from(other); }
s5run_variable &s5run_variable::operator=(const s5run_variable &other) {
    if (this != &other) { destroy(); copy_from(other); }
    return *this;
}
s5run_variable::~s5run_variable() { destroy(); }
s5run_variable s5run_variable::make_ref(int this_id, const std::string &type) {
    s5run_variable v; v.m_kind = K_REF; v.m_i = this_id; v.m_s = type; return v;
}
s5run_variable::kind s5run_variable::get_kind() const { return m_kind; }
bool s5run_variable::is_undefined() const { return m_kind == K_UNDEFINED; }
bool s5run_variable::is_null() const { return m_kind == K_NULL; }
bool s5run_variable::as_bool() const { return m_b; }
i64 s5run_variable::as_int() const { return m_i; }
double s5run_variable::as_double() const { return m_d; }
const std::string &s5run_variable::as_string() const { return m_s; }
const std::vector<s5run_variable> &s5run_variable::as_array() const { return *m_a; }
const std::map<std::string, s5run_variable> &s5run_variable::as_object() const { return *m_o; }
int s5run_variable::ref_this_id() const { return (int)m_i; }
const std::string &s5run_variable::ref_type() const { return m_s; }
void s5run_variable::destroy() {
    delete m_a; m_a = 0;
    delete m_o; m_o = 0;
    m_kind = K_UNDEFINED;
}
void s5run_variable::copy_from(const s5run_variable &other) {
    m_kind = other.m_kind; m_b = other.m_b; m_i = other.m_i; m_d = other.m_d;
    m_s = other.m_s;
    m_a = other.m_a ? new std::vector<s5run_variable>(*other.m_a) : 0;
    m_o = other.m_o ? new std::map<std::string, s5run_variable>(*other.m_o) : 0;
}

std::string s5run_variable::to_string_s5run() const {
    std::string out;
    append_debug(out, 0);
    if (!out.empty() && out[out.size() - 1] == '\n')
        out.erase(out.size() - 1);
    return out;
}

std::string s5run_variable::to_string_json() const {
    std::string out;
    append_json(out);
    return out;
}

// Structured multi-line form: the first line is emitted without an
// indent prefix (the caller positions it), continuation lines carry
// the given indent.
void s5run_variable::append_debug(std::string &out, int indent) const {
    char buf[64];
    std::string pad(indent, ' ');
    std::string pad_in(indent + 2, ' ');
    switch (m_kind) {
    case K_UNDEFINED:
        out += "K_UNDEFINED\n";
        break;
    case K_NULL:
        out += "K_NULL\n";
        break;
    case K_BOOL:
        out += m_b ? "K_BOOL true\n" : "K_BOOL false\n";
        break;
    case K_INT:
        snprintf(buf, sizeof(buf), "K_INT %lld\n", (long long)m_i);
        out += buf;
        break;
    case K_DOUBLE:
        snprintf(buf, sizeof(buf), "K_DOUBLE %g\n", m_d);
        out += buf;
        break;
    case K_STRING:
        out += "K_STRING ";
        append_json_string(out, m_s);
        out += "\n";
        break;
    case K_ARRAY:
        out += "K_ARRAY\n";
        out += pad + "BEGIN\n";
        for (std::size_t i = 0; i < m_a->size(); ++i) {
            out += pad_in;
            (*m_a)[i].append_debug(out, indent + 2);
        }
        out += pad + "END\n";
        break;
    case K_OBJECT:
        // std::map iterates sorted by name
        out += "K_OBJECT\n";
        out += pad + "BEGIN\n";
        for (std::map<std::string, s5run_variable>::const_iterator it = m_o->begin();
             it != m_o->end(); ++it) {
            out += pad_in + it->first + " ";
            it->second.append_debug(out, indent + 2);
        }
        out += pad + "END\n";
        break;
    case K_REF:
        out += "K_REF " + num_to_str((long long)m_i) + " ";
        append_json_string(out, m_s);
        out += "\n";
        break;
    }
}

void s5run_variable::append_json_string(std::string &out, const std::string &s) {
    out += '"';
    for (std::size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if ((unsigned char)c < 0x20) {
                char b[8];
                snprintf(b, sizeof(b), "\\u%04x", (unsigned char)c);
                out += b;
            } else {
                out += c;
            }
        }
    }
    out += '"';
}

void s5run_variable::append_json(std::string &out) const {
    char buf[64];
    switch (m_kind) {
    case K_UNDEFINED:
    case K_NULL: out += "null"; break;
    case K_BOOL: out += m_b ? "true" : "false"; break;
    case K_INT:
        snprintf(buf, sizeof(buf), "%lld", (long long)m_i);
        out += buf;
        break;
    case K_DOUBLE:
        snprintf(buf, sizeof(buf), "%g", m_d);
        out += buf;
        break;
    case K_STRING:
        append_json_string(out, m_s);
        break;
    case K_ARRAY: {
        out += "[";
        for (std::size_t i = 0; i < m_a->size(); ++i) {
            if (i > 0)
                out += ", ";
            (*m_a)[i].append_json(out);
        }
        out += "]";
        break;
    }
    case K_OBJECT: {
        out += "{";
        bool first = true;
        for (std::map<std::string, s5run_variable>::const_iterator it = m_o->begin();
             it != m_o->end(); ++it) {
            if (!first)
                out += ", ";
            first = false;
            append_json_string(out, it->first);
            out += ": ";
            it->second.append_json(out);
        }
        out += "}";
        break;
    }
    case K_REF:
        out += "{\"__s5run_ref__\":{\"this_id\":" + num_to_str((long long)m_i) + ",\"type\":";
        append_json_string(out, m_s);
        out += "}}";
        break;
    }
}

run_result::run_result() : m_has_error(false), m_error_line(0), m_duration_ms(0) {}
bool run_result::has_error() const { return m_has_error; }
const std::string &run_result::error() const { return m_error; }
int run_result::error_line() const { return m_error_line; }
i64 run_result::duration() const { return m_duration_ms; }
const s5run_variable &run_result::get_value() const { return m_value; }

remote_server_info::remote_server_info() : m_port(0) {}
void remote_server_info::set_address(const std::string &address) {
    const std::string prefix = "s5run://";
    if (address.compare(0, prefix.size(), prefix) != 0)
        throw std::runtime_error("set_address: expected format s5run://host:port");
    std::string rest = address.substr(prefix.size());
    std::size_t colon = rest.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= rest.size())
        throw std::runtime_error("set_address: expected format s5run://host:port");
    m_address = address; m_host = rest.substr(0, colon);
    std::sscanf(rest.substr(colon + 1).c_str(), "%d", &m_port);
}
const std::string &remote_server_info::get_address() const { return m_address; }
void remote_server_info::set_api_key(const std::string &k) { m_api_key = k; }
const std::string &remote_server_info::get_api_key() const { return m_api_key; }
const std::string &remote_server_info::get_host() const { return m_host; }
int remote_server_info::get_port() const { return m_port; }

// ---------------------------------------------------------------------------
// context_impl
// ---------------------------------------------------------------------------

class context::context_impl {
public:
    context_impl() : m_sock(S5RUN_BAD_SOCKET), m_next_this_id(0) {
        s5run_sockets_init();
    }
    ~context_impl() { disconnect(); s5run_sockets_shutdown(); }

    void set_script(const std::string &script);
    void add_server(remote_server_info *server);
    void register_type(const std::string &name, const std::vector<std::string> &methods,
                         const std::vector<s5run_property_descriptor> &props);
    void add_object(s5run_object *obj);
    void add_function(s5run_function *fn);
    run_result run(const std::string &entry, const std::vector<s5run_variable> &args);
    // Provider: listen on host:port, serve one client until it
    // disconnects (metadata requests + API calls).
    bool serve(const std::string &host, int port);
    // registry fills (metadata comes from the generated base classes)
    void provider_note_class(s5run_object *sample);
    void provider_note_function(s5run_function *fn);
    void keep_sample(s5run_object *sample);

private:
    struct PendingType { std::string name; std::vector<std::string> methods; std::vector<s5run_property_descriptor> props; };
    struct PendingObject { std::string name; i32 this_id; std::vector<std::string> methods; std::vector<s5run_property_descriptor> props; };

    std::map<std::string, s5run_object *> m_class_samples;
    void cb_getQueue(i32 cb_id, Reader &r);
    void cbm_MassageCrew_Queue_publish(i32 cb_id, Reader &r);
    void cbm_MassageCrew_Queue_consume(i32 cb_id, Reader &r);
    void ctor_MassageCrew_Queue(i32 cb_id, Reader &r);
    i32 register_instance_MassageCrew_Queue(base_context_class_MassageCrew_Queue *obj);
    void store_instance(const std::string &cls, i32 id, s5run_object *obj);
    void dispatch_callback(const std::vector<u8> &body);
    void connect(); void disconnect(); void flush_pending();
    static void read_hello(sock_t s);
    static bool send_frame(sock_t s, u8 msg_type, const std::vector<u8> &body);
    static bool read_frame(sock_t s, u8 &msg_type, std::vector<u8> &body);
    void send_message(u8 msg_type, const std::vector<u8> &body);
    void expect_register_resp(const std::string &what);
    static i64 probe_server_load(remote_server_info *server);
    static int parse_error_line(const std::string &error);

    std::string m_script;
    std::vector<remote_server_info *> m_servers;
    std::vector<PendingType> m_pending_types;
    std::vector<PendingObject> m_pending_objects;
    std::vector<std::string> m_pending_functions;
    sock_t m_sock;
    i32 m_next_this_id;
    // provider metadata registry (filled by add_class/add_object/add_function)
    std::map<std::string, s5run_class_metadata> m_meta_classes;
    std::map<std::string, s5run_function_metadata> m_meta_functions;
    // global objects: name -> (class metadata, this_id)
    struct MetaObject { s5run_class_metadata meta; i32 this_id; };
    std::map<std::string, MetaObject> m_meta_objects;
    void handle_meta_req(const std::vector<u8> &body);
    void send_meta_class(Writer &w, u8 kind, const s5run_class_metadata &meta, i32 this_id);
    void send_meta_function(Writer &w, const s5run_function_metadata &meta);
    base_context_function_getQueue *m_fn_getQueue;
    std::map<i32, base_context_class_MassageCrew_Queue *> m_instances_MassageCrew_Queue;
};


void context::context_impl::set_script(const std::string &script) { m_script = script; }
void context::context_impl::add_server(remote_server_info *server) { m_servers.push_back(server); }

static void s5run__put_property_descriptors(Writer &w,
                                            const std::vector<s5run_property_descriptor> &props) {
    w.put_u32((u32)props.size());
    for (std::size_t i = 0; i < props.size(); ++i) {
        w.put_string(props[i].name);
        w.put_string(props[i].getter);
        w.put_string(props[i].setter);
    }
}

void context::context_impl::register_type(const std::string &name,
                                          const std::vector<std::string> &methods,
                                          const std::vector<s5run_property_descriptor> &props) {
    if (m_sock != S5RUN_BAD_SOCKET) {
        Writer w;
        w.put_string(name);
        w.put_u32((u32)methods.size());
        for (std::size_t i = 0; i < methods.size(); ++i) w.put_string(methods[i]);
        s5run__put_property_descriptors(w, props);
        send_message(M_REGISTER_TYPE, w.data());
        expect_register_resp("register_type(" + name + ")");
        return;
    }
    PendingType t; t.name = name; t.methods = methods; t.props = props;
    m_pending_types.push_back(t);
}

void context::context_impl::add_object(s5run_object *obj) {
    const std::string cls = obj->s5run_class_name();
    const std::vector<std::string> methods = obj->s5run_method_names();
    const std::vector<s5run_property_descriptor> props = obj->s5run_property_descriptors();
    i32 id = ++m_next_this_id;
    store_instance(cls, id, obj);
    if (obj->s5run_metadata() != 0) {
        MetaObject mo;
        mo.meta = *obj->s5run_metadata();
        mo.this_id = id;
        m_meta_objects[mo.meta.name] = mo;
    }

    if (m_sock != S5RUN_BAD_SOCKET) {
        Writer w;
        w.put_string(cls); w.put_i32(id); w.put_string(cls);
        w.put_u32((u32)methods.size());
        for (std::size_t i = 0; i < methods.size(); ++i) w.put_string(methods[i]);
        s5run__put_property_descriptors(w, props);
        send_message(M_REGISTER_OBJ, w.data());
        expect_register_resp("register_object(" + cls + ")");
        return;
    }
    PendingObject o; o.name = cls; o.this_id = id; o.methods = methods; o.props = props;
    m_pending_objects.push_back(o);
}

void context::context_impl::provider_note_class(s5run_object *sample) {
    const s5run_class_metadata *m = sample->s5run_metadata();
    if (m != 0)
        m_meta_classes[m->name] = *m;
}

void context::context_impl::provider_note_function(s5run_function *fn) {
    const s5run_function_metadata *m = fn->s5run_metadata();
    if (m != 0)
        m_meta_functions[m->name] = *m;
}


void context::context_impl::add_function(s5run_function *fn) {
    if (fn->s5run_function_name() == "getQueue") {
        m_fn_getQueue = (base_context_function_getQueue *)fn;
    }
    provider_note_function(fn);

    if (m_sock != S5RUN_BAD_SOCKET) {
        Writer w;
        w.put_string(fn->s5run_function_name());
        send_message(M_REGISTER_FUNC, w.data());
        expect_register_resp("register_function(" + fn->s5run_function_name() + ")");
        return;
    }
    m_pending_functions.push_back(fn->s5run_function_name());
}

void context::context_impl::store_instance(const std::string &cls, i32 id, s5run_object *obj) {
    if (cls == "MassageCrew_Queue") { m_instances_MassageCrew_Queue[id] = (base_context_class_MassageCrew_Queue *)obj; return; }
    throw std::runtime_error("add_object: unknown class: " + cls);
}

i32 context::context_impl::register_instance_MassageCrew_Queue(base_context_class_MassageCrew_Queue *obj) {
    i32 id = ++m_next_this_id;
    m_instances_MassageCrew_Queue[id] = obj;
    return id;
}

void context::context_impl::cb_getQueue(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("getQueue: wrong argument count");
    std::string a_queue_name = read_string(r);
    base_context_class_MassageCrew_Queue * res = m_fn_getQueue->execute(a_queue_name);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    i32 obj_id = register_instance_MassageCrew_Queue(res);
    w.put_u8(T_REF); w.put_i32(obj_id); w.put_string("MassageCrew_Queue");
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_MassageCrew_Queue_publish(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 3) throw std::runtime_error("MassageCrew_Queue.publish: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_MassageCrew_Queue *>::const_iterator it = m_instances_MassageCrew_Queue.find(this_id);
    if (it == m_instances_MassageCrew_Queue.end()) throw std::runtime_error("MassageCrew_Queue: unknown this_id");
    std::string a_msg_id = read_string(r);
    std::string a_msg_text = read_string(r);
    it->second->publish(a_msg_id, a_msg_text);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_MassageCrew_Queue_consume(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("MassageCrew_Queue.consume: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_MassageCrew_Queue *>::const_iterator it = m_instances_MassageCrew_Queue.find(this_id);
    if (it == m_instances_MassageCrew_Queue.end()) throw std::runtime_error("MassageCrew_Queue: unknown this_id");
    s5run_variable res = it->second->consume();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    encode_value(w, res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::keep_sample(s5run_object *sample) {
    m_class_samples[sample->s5run_class_name()] = sample;
}

void context::context_impl::ctor_MassageCrew_Queue(i32 cb_id, Reader &r) {
    (void)r; // constructor args are ignored (the factory is no-arg)
    std::map<std::string, s5run_object *>::const_iterator it = m_class_samples.find("MassageCrew_Queue");
    if (it == m_class_samples.end())
        throw std::runtime_error("MassageCrew_Queue: no class sample registered (add_class)");
    base_context_class_MassageCrew_Queue *cls = dynamic_cast<base_context_class_MassageCrew_Queue *>(it->second);
    if (cls == 0)
        throw std::runtime_error("MassageCrew_Queue: bad class sample");
    base_context_class_MassageCrew_Queue *inst = cls->s5run_construct();
    if (inst == 0)
        throw std::runtime_error("MassageCrew_Queue: not constructible (override s5run_construct)");
    i32 id = register_instance_MassageCrew_Queue(inst);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
    w.put_u8(T_REF);
    w.put_i32(id);
    w.put_string("MassageCrew_Queue");
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::dispatch_callback(const std::vector<u8> &body) {
    Reader r(body.empty() ? 0 : &body[0], body.size());
    i32 cb_id = r.get_i32();
    std::string method = r.get_string();
    try {
        if (method == "getQueue") { cb_getQueue(cb_id, r); return; }
        if (method == "publish") { cbm_MassageCrew_Queue_publish(cb_id, r); return; }
        if (method == "consume") { cbm_MassageCrew_Queue_consume(cb_id, r); return; }
        if (method == "MassageCrew_Queue") { ctor_MassageCrew_Queue(cb_id, r); return; }
    } catch (const std::exception &e) {
        Writer w; w.put_i32(cb_id); w.put_u8(0); w.put_string(e.what());
        send_message(M_CALLBACK_RESP, w.data());
        return;
    }
    Writer w; w.put_i32(cb_id); w.put_u8(0);
    w.put_string("unknown method: " + method);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::connect() {
    if (m_sock != S5RUN_BAD_SOCKET) return;
    if (m_servers.empty()) throw std::runtime_error("s5run: no server added (add_server)");
    std::size_t best = 0;
    if (m_servers.size() > 1) {
        i64 best_load = -1;
        for (std::size_t i = 0; i < m_servers.size(); ++i) {
            i64 load = probe_server_load(m_servers[i]);
            if (load < 0) continue;
            if (best_load < 0 || load < best_load) { best_load = load; best = i; }
        }
        if (best_load < 0) throw std::runtime_error("s5run: all servers are unreachable");
    }
    sock_t s = s5run_connect_socket(m_servers[best]->get_host(), m_servers[best]->get_port());
    if (s == S5RUN_BAD_SOCKET) throw std::runtime_error("s5run: connect failed");
    s5run_set_nodelay(s);
    s5run_set_recv_timeout(s, 10000);
    m_sock = s;
    read_hello(m_sock);
}

void context::context_impl::read_hello(sock_t s) {
    u8 msg_type = 0;
    std::vector<u8> body;
    if (!read_frame(s, msg_type, body) || msg_type != M_SERVER_HELLO)
        throw std::runtime_error("s5run: expected server hello");
    std::string text;
    if (!body.empty())
        text.assign((const char *)&body[0], body.size());
    if (text.compare(0, 6, "s5run ") != 0)
        throw std::runtime_error("s5run: bad server hello: " + text);
    std::size_t last_space = text.rfind(' ');
    if (last_space == std::string::npos)
        throw std::runtime_error("s5run: bad server hello: " + text);
    int protocol = 0; std::sscanf(text.substr(last_space + 1).c_str(), "%d", &protocol);
    if (protocol != S5RUN_PROTOCOL_VERSION)
        throw std::runtime_error("s5run: server protocol version " + num_to_str(protocol)
                                 + " is not supported (client supports " + num_to_str(S5RUN_PROTOCOL_VERSION) + ")");
}

void context::context_impl::disconnect() {
    if (m_sock != S5RUN_BAD_SOCKET) { s5run_close_socket(m_sock); m_sock = S5RUN_BAD_SOCKET; }
}

i64 context::context_impl::probe_server_load(remote_server_info *server) {
    sock_t s = s5run_connect_socket(server->get_host(), server->get_port());
    if (s == S5RUN_BAD_SOCKET) return -1;
    s5run_set_nodelay(s);
    s5run_set_recv_timeout(s, 2000);
    try {
        read_hello(s);
        std::vector<u8> empty_body;
        if (!send_frame(s, M_STATS_REQ, empty_body)) { s5run_close_socket(s); return -1; }
        u8 msg_type = 0;
        std::vector<u8> body;
        if (!read_frame(s, msg_type, body) || msg_type != M_STATS_RESP || body.size() < 16) {
            s5run_close_socket(s); return -1;
        }
        s5run_close_socket(s);
        Reader r(&body[0], body.size());
        r.get_u32(); // threads
        u32 busy = r.get_u32();
        u32 queued = r.get_u32();
        return (i64)busy + (i64)queued;
    } catch (...) { s5run_close_socket(s); return -1; }
}

void context::context_impl::flush_pending() {
    for (std::size_t i = 0; i < m_pending_types.size(); ++i)
        register_type(m_pending_types[i].name, m_pending_types[i].methods, m_pending_types[i].props);
    m_pending_types.clear();
    for (std::size_t i = 0; i < m_pending_objects.size(); ++i) {
        const PendingObject &o = m_pending_objects[i];
        Writer w;
        w.put_string(o.name); w.put_i32(o.this_id); w.put_string(o.name);
        w.put_u32((u32)o.methods.size());
        for (std::size_t j = 0; j < o.methods.size(); ++j) w.put_string(o.methods[j]);
        s5run__put_property_descriptors(w, o.props);
        send_message(M_REGISTER_OBJ, w.data());
        expect_register_resp("register_object(" + o.name + ")");
    }
    m_pending_objects.clear();
    for (std::size_t i = 0; i < m_pending_functions.size(); ++i) {
        Writer w;
        w.put_string(m_pending_functions[i]);
        send_message(M_REGISTER_FUNC, w.data());
        expect_register_resp("register_function(" + m_pending_functions[i] + ")");
    }
    m_pending_functions.clear();
}

bool context::context_impl::send_frame(sock_t s, u8 msg_type, const std::vector<u8> &body) {
    u32 len = (u32)(1 + body.size());
    char head[5];
    head[0] = (char)((len >> 24) & 0xFF); head[1] = (char)((len >> 16) & 0xFF);
    head[2] = (char)((len >> 8) & 0xFF); head[3] = (char)(len & 0xFF);
    head[4] = (char)msg_type;
    if (!s5run_send_all(s, head, 5)) return false;
    return body.empty() ? true : s5run_send_all(s, (const char *)&body[0], body.size());
}

void context::context_impl::send_message(u8 msg_type, const std::vector<u8> &body) {
    if (!send_frame(m_sock, msg_type, body))
        throw std::runtime_error("s5run: send failed");
}

bool context::context_impl::read_frame(sock_t s, u8 &msg_type, std::vector<u8> &body) {
    char head[5];
    if (!s5run_recv_exact(s, head, 5)) return false;
    u32 len = ((u32)(u8)head[0] << 24) | ((u32)(u8)head[1] << 16)
              | ((u32)(u8)head[2] << 8) | (u32)(u8)head[3];
    msg_type = (u8)head[4];
    if (len == 0 || len > 64 * 1024 * 1024) return false;
    body.resize(len - 1);
    if (len > 1 && !s5run_recv_exact(s, (char *)&body[0], body.size())) return false;
    return true;
}

void context::context_impl::expect_register_resp(const std::string &what) {
    u8 msg_type = 0;
    std::vector<u8> body;
    if (!read_frame(m_sock, msg_type, body) || msg_type != M_REGISTER_RESP)
        throw std::runtime_error("s5run: " + what + " failed");
    Reader r(body.empty() ? 0 : &body[0], body.size());
    if (r.get_u8() == 0)
        throw std::runtime_error("s5run: " + what + " error: " + r.get_string());
}

int context::context_impl::parse_error_line(const std::string &error) {
    for (std::size_t pos = 0; pos < error.size(); ++pos) {
        if (error[pos] != ':') continue;
        std::size_t p1 = pos + 1;
        if (p1 >= error.size() || error[p1] < '0' || error[p1] > '9') continue;
        std::size_t p2 = p1;
        while (p2 < error.size() && error[p2] >= '0' && error[p2] <= '9') ++p2;
        if (p2 + 1 < error.size() && error[p2] == ':' && error[p2 + 1] >= '0' && error[p2 + 1] <= '9') {
            int out = 0; std::sscanf(error.substr(p1, p2 - p1).c_str(), "%d", &out); return out;}
        pos = p2;
    }
    return 0;
}

run_result context::context_impl::run(const std::string &entry,
                                      const std::vector<s5run_variable> &args) {
    run_result out;
    std::clock_t started = std::clock();
    try {
        connect();
        flush_pending();
        Writer w;
        w.put_string(m_script);
        w.put_string(entry);
        w.put_u32((u32)args.size());
        for (std::size_t i = 0; i < args.size(); ++i) encode_value(w, args[i]);
        send_message(M_EXEC_REQ, w.data());
        while (true) {
            u8 msg_type = 0;
            std::vector<u8> body;
            if (!read_frame(m_sock, msg_type, body))
                throw std::runtime_error("s5run: connection closed");
            if (msg_type == M_CALLBACK_REQ) {
                try { dispatch_callback(body); }
                catch (const std::exception &e) { (void)e; }
                continue;
            }
            if (msg_type == M_EXEC_RESP) {
                Reader r(body.empty() ? 0 : &body[0], body.size());
                if (r.get_u8() == 0) throw std::runtime_error(r.get_string());
                out.m_value = decode_value(r);
                break;
            }
            throw std::runtime_error("s5run: unexpected message type");
        }
    } catch (const std::exception &e) {
        out.m_has_error = true;
        out.m_error = e.what();
        out.m_error_line = parse_error_line(out.m_error);
    }
    out.m_duration_ms = (i64)((double)(std::clock() - started) * 1000.0 / CLOCKS_PER_SEC);
    return out;
}

context::context() : m_impl(new context::context_impl()) {}
context::~context() { delete m_impl; }

void context::set_script(const std::string &s) { m_impl->set_script(s); }
void context::add_server(remote_server_info *s) { m_impl->add_server(s); }
void context::add_class(s5run_object *o) { m_impl->keep_sample(o); m_impl->register_type(o->s5run_class_name(), o->s5run_method_names(), o->s5run_property_descriptors()); m_impl->provider_note_class(o); }
void context::add_object(s5run_object *o) { m_impl->add_object(o); }
void context::add_function(s5run_function *f) { m_impl->add_function(f); }

context * make_context() { return new context(); }

run_result run_script(context *ctx, const std::string &entry, const std::vector<s5run_variable> &args) { return ctx->m_impl->run(entry, args); }

// ---- provider: metadata + serving ----

enum { M_PROVIDER_META_REQ = 0x10, M_PROVIDER_META_RESP = 0x11 };

void context::context_impl::send_meta_function(Writer &w, const s5run_function_metadata &meta) {
    w.put_u8(1); // function
    w.put_string(meta.name);
    w.put_string(meta.return_type);
    w.put_u32((u32)meta.args.size());
    for (std::size_t i = 0; i < meta.args.size(); ++i) {
        w.put_string(meta.args[i].name);
        w.put_string(meta.args[i].type);
    }
}

void context::context_impl::send_meta_class(Writer &w, u8 kind,
                                             const s5run_class_metadata &meta, i32 this_id) {
    w.put_u8(kind);
    w.put_string(meta.name);
    w.put_u32((u32)meta.methods.size());
    for (std::size_t i = 0; i < meta.methods.size(); ++i) {
        w.put_string(meta.methods[i].name);
        w.put_string(meta.methods[i].return_type);
        w.put_u32((u32)meta.methods[i].args.size());
        for (std::size_t j = 0; j < meta.methods[i].args.size(); ++j) {
            w.put_string(meta.methods[i].args[j].name);
            w.put_string(meta.methods[i].args[j].type);
        }
    }
    s5run__put_property_descriptors(w, meta.properties);
    if (kind == 3)
        w.put_i32(this_id);
}

void context::context_impl::handle_meta_req(const std::vector<u8> &body) {
    Reader r(body.empty() ? 0 : &body[0], body.size());
    std::string name = r.get_string();
    Writer w;
    if (name.empty()) {
        // list everything registered
        u8 count = (u8)(m_meta_functions.size() + m_meta_classes.size() + m_meta_objects.size());
        w.put_u8(count);
        for (std::map<std::string, s5run_function_metadata>::const_iterator it = m_meta_functions.begin();
             it != m_meta_functions.end(); ++it)
            send_meta_function(w, it->second);
        for (std::map<std::string, MetaObject>::const_iterator it = m_meta_objects.begin();
             it != m_meta_objects.end(); ++it)
            send_meta_class(w, 3, it->second.meta, it->second.this_id);
        for (std::map<std::string, s5run_class_metadata>::const_iterator it = m_meta_classes.begin();
             it != m_meta_classes.end(); ++it)
            send_meta_class(w, 2, it->second, 0);
    } else if (m_meta_functions.count(name) != 0) {
        w.put_u8(1);
        send_meta_function(w, m_meta_functions[name]);
    } else if (m_meta_objects.count(name) != 0) {
        w.put_u8(1);
        send_meta_class(w, 3, m_meta_objects[name].meta, m_meta_objects[name].this_id);
    } else if (m_meta_classes.count(name) != 0) {
        w.put_u8(1);
        send_meta_class(w, 2, m_meta_classes[name], 0);
    } else {
        w.put_u8(0); // not found
    }
    send_message(M_PROVIDER_META_RESP, w.data());
}

bool context::context_impl::serve(const std::string &host, int port) {
    disconnect();

    sock_t lfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == S5RUN_BAD_SOCKET)
        return false;
    int opt = 1;
    ::setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16)port);
    addr.sin_addr.s_addr = host.empty() ? htonl(INADDR_ANY) : inet_addr(host.c_str());
    if (::bind(lfd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        s5run_close_socket(lfd);
        return false;
    }
    if (::listen(lfd, 1) < 0) {
        s5run_close_socket(lfd);
        return false;
    }
    while (true) { // accept loop (empty connections like port probes are skipped)
        m_sock = ::accept(lfd, 0, 0);
        if (m_sock == S5RUN_BAD_SOCKET) {
            s5run_close_socket(lfd);
            return false;
        }
        s5run_set_nodelay(m_sock);

        // hello: "s5run <version> <protocol>"
        try {
            std::string hello = std::string("s5run ") + client_version() + " 2";
            std::vector<u8> hb(hello.begin(), hello.end());
            send_message(M_SERVER_HELLO, hb);
        } catch (const std::exception &) {
            // the client closed immediately (a port probe) - accept the next one
            disconnect();
            continue;
        }

        bool had_traffic = false;
        while (true) {
            u8 msg_type = 0;
            std::vector<u8> body;
            if (!read_frame(m_sock, msg_type, body))
                break; // client disconnected
            had_traffic = true;
            if (msg_type == M_PROVIDER_META_REQ) {
                handle_meta_req(body);
                continue;
            }
            if (msg_type == M_CALLBACK_REQ) {
                try { dispatch_callback(body); }
                catch (const std::exception &) {}
                continue;
            }
            break; // unexpected message
        }
        disconnect();
        if (had_traffic) {
            s5run_close_socket(lfd);
            return true; // a real client finished
        }
        // empty connection (e.g. a port probe) - accept the next one
    }
}

bool serve(context *ctx, const std::string &host, int port) { return ctx->m_impl->serve(host, port); }

} // namespace s5run_provider
