// AUTO-GENERATED. DO NOT EDIT.
#include "d1_s5run_cli_cpp98.h"

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

namespace s5run_cli_cpp98 {

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

static bool read_bool(Reader &r) {
    if (r.get_u8() != T_BOOL) throw std::runtime_error("wire: expected a bool");
    return r.get_u8() != 0;
}

static void write_bool(Writer &w, bool v) {
    w.put_u8(T_BOOL); w.put_u8(v ? 1 : 0);
}

static double read_double(Reader &r) {
    if (r.get_u8() != T_FLOAT64) throw std::runtime_error("wire: expected a double");
    return r.get_f64();
}

static void write_double(Writer &w, double v) {
    w.put_u8(T_FLOAT64); w.put_f64(v);
}

static std::string read_string(Reader &r) {
    if (r.get_u8() != T_STRING) throw std::runtime_error("wire: expected a string");
    return r.get_string();
}

static void write_string(Writer &w, const std::string &v) {
    w.put_u8(T_STRING); w.put_string(v);
}

static u8 read_byte(Reader &r) {
    if (r.get_u8() != T_UINT8) throw std::runtime_error("wire: expected a byte");
    return r.get_u8();
}

static void write_byte(Writer &w, u8 v) {
    w.put_u8(T_UINT8); w.put_u8(v);
}


static void encode_value(Writer &w, const value &v) {
    switch (v.get_kind()) {
    case value::K_NULL: w.put_u8(T_UNDEFINED); break;
    case value::K_BOOL: w.put_u8(T_BOOL); w.put_u8(v.as_bool() ? 1 : 0); break;
    case value::K_INT: write_int(w, v.as_int()); break;
    case value::K_DOUBLE: w.put_u8(T_FLOAT64); w.put_f64(v.as_double()); break;
    case value::K_STRING: w.put_u8(T_STRING); w.put_string(v.as_string()); break;
    case value::K_ARRAY: {
        w.put_u8(T_ARRAY);
        const std::vector<value> &a = v.as_array();
        w.put_u32((u32)a.size());
        for (std::size_t i = 0; i < a.size(); ++i) encode_value(w, a[i]);
        break;
    }
    case value::K_OBJECT: {
        w.put_u8(T_OBJECT);
        const std::map<std::string, value> &o = v.as_object();
        w.put_u32((u32)o.size());
        for (std::map<std::string, value>::const_iterator it = o.begin(); it != o.end(); ++it) {
            w.put_u8(T_STRING); w.put_string(it->first);
            encode_value(w, it->second);
        }
        break;
    }
    case value::K_REF: {
        w.put_u8(T_REF); w.put_i32(v.ref_this_id()); w.put_string(v.ref_type());
        break;
    }
    }
}

static value decode_value(Reader &r) {
    u8 tag = r.get_u8();
    switch (tag) {
    case T_UNDEFINED: return value();
    case T_BOOL: return value(r.get_u8() != 0);
    case T_INT8: return value((i64)r.get_i8());
    case T_INT16: return value((i64)r.get_i16());
    case T_INT32: return value((i64)r.get_i32());
    case T_INT64: return value(r.get_i64());
    case T_UINT8: return value((i64)r.get_u8());
    case T_UINT16: return value((i64)r.get_u16());
    case T_UINT32: return value((i64)r.get_u32());
    case T_UINT64: return value((i64)r.get_u64());
    case T_FLOAT64: return value(r.get_f64());
    case T_STRING: return value(r.get_string());
    case T_ARRAY: {
        u32 n = r.get_u32();
        std::vector<value> a; a.reserve(n);
        for (u32 i = 0; i < n; ++i) a.push_back(decode_value(r));
        return value(a);
    }
    case T_OBJECT: {
        u32 n = r.get_u32();
        std::map<std::string, value> o;
        for (u32 i = 0; i < n; ++i) {
            if (r.get_u8() != T_STRING) throw std::runtime_error("wire: expected a string key");
            std::string key = r.get_string();
            o[key] = decode_value(r);
        }
        return value(o);
    }
    case T_REF: {
        i32 this_id = r.get_i32();
        std::string type = r.get_string();
        return value::make_ref(this_id, type);
    }
    default: throw std::runtime_error("wire: unknown value tag");
    }
}

static std::string _value_to_string(const value &v) {
    switch (v.get_kind()) {
    case value::K_NULL: return "null";
    case value::K_BOOL: return v.as_bool() ? "true" : "false";
    case value::K_INT: return num_to_str(v.as_int());
    case value::K_DOUBLE: {
        char buf[64]; std::sprintf(buf, "%g", v.as_double());
        return std::string(buf);
    }
    case value::K_STRING: return v.as_string();
    default: throw std::runtime_error("wire: expected a scalar argument");
    }
}

value::value() : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0), m_a(0), m_o(0) {}
value::value(bool b) : m_kind(K_BOOL), m_b(b), m_i(0), m_d(0.0), m_a(0), m_o(0) {}
value::value(i64 i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0), m_a(0), m_o(0) {}
value::value(double d) : m_kind(K_DOUBLE), m_b(false), m_i(0), m_d(d), m_a(0), m_o(0) {}
value::value(const char *s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s), m_a(0), m_o(0) {}
value::value(const std::string &s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s), m_a(0), m_o(0) {}
value::value(const std::vector<value> &a) : m_kind(K_ARRAY), m_b(false), m_i(0), m_d(0.0), m_a(new std::vector<value>(a)), m_o(0) {}
value::value(const std::map<std::string, value> &o) : m_kind(K_OBJECT), m_b(false), m_i(0), m_d(0.0), m_a(0), m_o(new std::map<std::string, value>(o)) {}
value::value(const value &other) : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0), m_a(0), m_o(0) { copy_from(other); }
value &value::operator=(const value &other) {
    if (this != &other) { destroy(); copy_from(other); }
    return *this;
}
value::~value() { destroy(); }
value value::make_ref(int this_id, const std::string &type) {
    value v; v.m_kind = K_REF; v.m_i = this_id; v.m_s = type; return v;
}
value::kind value::get_kind() const { return m_kind; }
bool value::is_null() const { return m_kind == K_NULL; }
bool value::as_bool() const { return m_b; }
i64 value::as_int() const { return m_i; }
double value::as_double() const { return m_d; }
const std::string &value::as_string() const { return m_s; }
const std::vector<value> &value::as_array() const { return *m_a; }
const std::map<std::string, value> &value::as_object() const { return *m_o; }
int value::ref_this_id() const { return (int)m_i; }
const std::string &value::ref_type() const { return m_s; }
void value::destroy() {
    delete m_a; m_a = 0;
    delete m_o; m_o = 0;
    m_kind = K_NULL;
}
void value::copy_from(const value &other) {
    m_kind = other.m_kind; m_b = other.m_b; m_i = other.m_i; m_d = other.m_d;
    m_s = other.m_s;
    m_a = other.m_a ? new std::vector<value>(*other.m_a) : 0;
    m_o = other.m_o ? new std::map<std::string, value>(*other.m_o) : 0;
}

run_result::run_result() : m_has_error(false), m_error_line(0), m_duration_ms(0) {}
bool run_result::has_error() const { return m_has_error; }
const std::string &run_result::error() const { return m_error; }
int run_result::error_line() const { return m_error_line; }
i64 run_result::duration() const { return m_duration_ms; }
const value &run_result::get_value() const { return m_value; }

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
    void register_type(const std::string &name, const std::vector<std::string> &methods);
    void add_object(s5run_object *obj);
    void add_function(s5run_function *fn);
    run_result run(const std::string &entry, const std::vector<value> &args);

private:
    struct PendingType { std::string name; std::vector<std::string> methods; };
    struct PendingObject { std::string name; i32 this_id; std::vector<std::string> methods; };

    void cb_GetFile(i32 cb_id, Reader &r);
    void cb_GetFileSize(i32 cb_id, Reader &r);
    void cb_GetTimeNow(i32 cb_id, Reader &r);
    void cb_HasFile(i32 cb_id, Reader &r);
    void cb_GetFileList(i32 cb_id, Reader &r);
    void cb_GetOpenedFiles(i32 cb_id, Reader &r);
    void cb_CloseFiles(i32 cb_id, Reader &r);
    void cb_GetPi(i32 cb_id, Reader &r);
    void cb_GetServerPort(i32 cb_id, Reader &r);
    void cbm_File_GetSize(i32 cb_id, Reader &r);
    void cbm_File_ReadBytes(i32 cb_id, Reader &r);
    void cbm_File_GetName(i32 cb_id, Reader &r);
    void cbm_File_GetBufferSize(i32 cb_id, Reader &r);
    void cbm_File_SetBufferSize(i32 cb_id, Reader &r);
    void cbm_FileSystem_GetList(i32 cb_id, Reader &r);
    void cbm_console_log(i32 cb_id, Reader &r);
    void cbm_console_warning(i32 cb_id, Reader &r);
    void cbm_console_error(i32 cb_id, Reader &r);
    void cbm_KeyValueStorage_SetValueByKey(i32 cb_id, Reader &r);
    void cbm_KeyValueStorage_GetValueByKey(i32 cb_id, Reader &r);
    i32 register_instance_File(base_context_class_File *obj);
    i32 register_instance_FileSystem(base_context_class_FileSystem *obj);
    i32 register_instance_console(base_context_class_console *obj);
    i32 register_instance_KeyValueStorage(base_context_class_KeyValueStorage *obj);
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
    base_context_function_GetFile *m_fn_GetFile;
    base_context_function_GetFileSize *m_fn_GetFileSize;
    base_context_function_GetTimeNow *m_fn_GetTimeNow;
    base_context_function_HasFile *m_fn_HasFile;
    base_context_function_GetFileList *m_fn_GetFileList;
    base_context_function_GetOpenedFiles *m_fn_GetOpenedFiles;
    base_context_function_CloseFiles *m_fn_CloseFiles;
    base_context_function_GetPi *m_fn_GetPi;
    base_context_function_GetServerPort *m_fn_GetServerPort;
    std::map<i32, base_context_class_File *> m_instances_File;
    std::map<i32, base_context_class_FileSystem *> m_instances_FileSystem;
    std::map<i32, base_context_class_console *> m_instances_console;
    std::map<i32, base_context_class_KeyValueStorage *> m_instances_KeyValueStorage;
};


void context::context_impl::set_script(const std::string &script) { m_script = script; }
void context::context_impl::add_server(remote_server_info *server) { m_servers.push_back(server); }

void context::context_impl::register_type(const std::string &name,
                                          const std::vector<std::string> &methods) {
    if (m_sock != S5RUN_BAD_SOCKET) {
        Writer w;
        w.put_string(name);
        w.put_u32((u32)methods.size());
        for (std::size_t i = 0; i < methods.size(); ++i) w.put_string(methods[i]);
        send_message(M_REGISTER_TYPE, w.data());
        expect_register_resp("register_type(" + name + ")");
        return;
    }
    PendingType t; t.name = name; t.methods = methods;
    m_pending_types.push_back(t);
}

void context::context_impl::add_object(s5run_object *obj) {
    const std::string cls = obj->s5run_class_name();
    const std::vector<std::string> methods = obj->s5run_method_names();
    i32 id = ++m_next_this_id;
    store_instance(cls, id, obj);
    if (m_sock != S5RUN_BAD_SOCKET) {
        Writer w;
        w.put_string(cls); w.put_i32(id); w.put_string(cls);
        w.put_u32((u32)methods.size());
        for (std::size_t i = 0; i < methods.size(); ++i) w.put_string(methods[i]);
        send_message(M_REGISTER_OBJ, w.data());
        expect_register_resp("register_object(" + cls + ")");
        return;
    }
    PendingObject o; o.name = cls; o.this_id = id; o.methods = methods;
    m_pending_objects.push_back(o);
}

void context::context_impl::add_function(s5run_function *fn) {
    if (fn->s5run_function_name() == "GetFile") {
        m_fn_GetFile = (base_context_function_GetFile *)fn;
    }
    if (fn->s5run_function_name() == "GetFileSize") {
        m_fn_GetFileSize = (base_context_function_GetFileSize *)fn;
    }
    if (fn->s5run_function_name() == "GetTimeNow") {
        m_fn_GetTimeNow = (base_context_function_GetTimeNow *)fn;
    }
    if (fn->s5run_function_name() == "HasFile") {
        m_fn_HasFile = (base_context_function_HasFile *)fn;
    }
    if (fn->s5run_function_name() == "GetFileList") {
        m_fn_GetFileList = (base_context_function_GetFileList *)fn;
    }
    if (fn->s5run_function_name() == "GetOpenedFiles") {
        m_fn_GetOpenedFiles = (base_context_function_GetOpenedFiles *)fn;
    }
    if (fn->s5run_function_name() == "CloseFiles") {
        m_fn_CloseFiles = (base_context_function_CloseFiles *)fn;
    }
    if (fn->s5run_function_name() == "GetPi") {
        m_fn_GetPi = (base_context_function_GetPi *)fn;
    }
    if (fn->s5run_function_name() == "GetServerPort") {
        m_fn_GetServerPort = (base_context_function_GetServerPort *)fn;
    }
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
    if (cls == "File") { m_instances_File[id] = (base_context_class_File *)obj; return; }
    if (cls == "FileSystem") { m_instances_FileSystem[id] = (base_context_class_FileSystem *)obj; return; }
    if (cls == "console") { m_instances_console[id] = (base_context_class_console *)obj; return; }
    if (cls == "KeyValueStorage") { m_instances_KeyValueStorage[id] = (base_context_class_KeyValueStorage *)obj; return; }
    throw std::runtime_error("add_object: unknown class: " + cls);
}

i32 context::context_impl::register_instance_File(base_context_class_File *obj) {
    i32 id = ++m_next_this_id;
    m_instances_File[id] = obj;
    return id;
}

i32 context::context_impl::register_instance_FileSystem(base_context_class_FileSystem *obj) {
    i32 id = ++m_next_this_id;
    m_instances_FileSystem[id] = obj;
    return id;
}

i32 context::context_impl::register_instance_console(base_context_class_console *obj) {
    i32 id = ++m_next_this_id;
    m_instances_console[id] = obj;
    return id;
}

i32 context::context_impl::register_instance_KeyValueStorage(base_context_class_KeyValueStorage *obj) {
    i32 id = ++m_next_this_id;
    m_instances_KeyValueStorage[id] = obj;
    return id;
}

void context::context_impl::cb_GetFile(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("GetFile: wrong argument count");
    std::string a_filepath = read_string(r);
    base_context_class_File * res = m_fn_GetFile->execute(a_filepath);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    i32 obj_id = register_instance_File(res);
    w.put_u8(T_REF); w.put_i32(obj_id); w.put_string("File");
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_GetFileSize(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("GetFileSize: wrong argument count");
    i32 a_ref_id_file = read_ref(r);
    std::map<i32, base_context_class_File *>::const_iterator a_it_file = m_instances_File.find(a_ref_id_file);
    if (a_it_file == m_instances_File.end()) throw std::runtime_error("GetFileSize: unknown object reference");
    base_context_class_File *a_file = a_it_file->second;
    i64 res = m_fn_GetFileSize->execute(a_file);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    write_int(w, res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_GetTimeNow(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 0) throw std::runtime_error("GetTimeNow: wrong argument count");
    i64 res = m_fn_GetTimeNow->execute();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    write_int(w, res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_HasFile(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("HasFile: wrong argument count");
    std::string a_filepath = read_string(r);
    bool res = m_fn_HasFile->execute(a_filepath);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_BOOL); w.put_u8(res ? 1 : 0);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_GetFileList(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("GetFileList: wrong argument count");
    std::string a_filepath = read_string(r);
    std::vector<std::string> res = m_fn_GetFileList->execute(a_filepath);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_ARRAY); w.put_u32((u32)res.size());
    for (std::size_t i = 0; i < res.size(); ++i) {
        w.put_u8(T_STRING); w.put_string(res[i]);
    }
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_GetOpenedFiles(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 0) throw std::runtime_error("GetOpenedFiles: wrong argument count");
    std::vector<base_context_class_File *> res = m_fn_GetOpenedFiles->execute();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_ARRAY);
    w.put_u32((u32)res.size());
    for (std::size_t i = 0; i < res.size(); ++i) {
        i32 obj_id = register_instance_File(res[i]);
        w.put_u8(T_REF); w.put_i32(obj_id); w.put_string("File");
    }
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_CloseFiles(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("CloseFiles: wrong argument count");
    if (r.get_u8() != T_ARRAY) throw std::runtime_error("wire: expected array");
    u32 cnt_files = r.get_u32();
    std::vector<base_context_class_File *> a_files;
    a_files.reserve(cnt_files);
    for (u32 i = 0; i < cnt_files; ++i) {
        i32 a_ref_id = read_ref(r);
        std::map<i32, base_context_class_File *>::const_iterator a_it = m_instances_File.find(a_ref_id);
        if (a_it == m_instances_File.end()) throw std::runtime_error("CloseFiles: unknown object reference");
        a_files.push_back(a_it->second);
    }
    m_fn_CloseFiles->execute(a_files);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_GetPi(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 0) throw std::runtime_error("GetPi: wrong argument count");
    double res = m_fn_GetPi->execute();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_FLOAT64); w.put_f64(res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cb_GetServerPort(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 0) throw std::runtime_error("GetServerPort: wrong argument count");
    i64 res = m_fn_GetServerPort->execute();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    write_int(w, res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_File_GetSize(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("File.GetSize: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_File *>::const_iterator it = m_instances_File.find(this_id);
    if (it == m_instances_File.end()) throw std::runtime_error("File: unknown this_id");
    i64 res = it->second->GetSize();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    write_int(w, res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_File_ReadBytes(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 2) throw std::runtime_error("File.ReadBytes: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_File *>::const_iterator it = m_instances_File.find(this_id);
    if (it == m_instances_File.end()) throw std::runtime_error("File: unknown this_id");
    i64 a_readBytes = read_int(r);
    std::vector<u8> res = it->second->ReadBytes(a_readBytes);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_ARRAY); w.put_u32((u32)res.size());
    for (std::size_t i = 0; i < res.size(); ++i) {
        w.put_u8(T_UINT8); w.put_u8(res[i]);
    }
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_File_GetName(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("File.GetName: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_File *>::const_iterator it = m_instances_File.find(this_id);
    if (it == m_instances_File.end()) throw std::runtime_error("File: unknown this_id");
    std::string res = it->second->GetName();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_STRING); w.put_string(res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_File_GetBufferSize(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 1) throw std::runtime_error("File.GetBufferSize: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_File *>::const_iterator it = m_instances_File.find(this_id);
    if (it == m_instances_File.end()) throw std::runtime_error("File: unknown this_id");
    i64 res = it->second->GetBufferSize();
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    write_int(w, res);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_File_SetBufferSize(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 2) throw std::runtime_error("File.SetBufferSize: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_File *>::const_iterator it = m_instances_File.find(this_id);
    if (it == m_instances_File.end()) throw std::runtime_error("File: unknown this_id");
    i64 a_bufferSize = read_int(r);
    it->second->SetBufferSize(a_bufferSize);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_FileSystem_GetList(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 2) throw std::runtime_error("FileSystem.GetList: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_FileSystem *>::const_iterator it = m_instances_FileSystem.find(this_id);
    if (it == m_instances_FileSystem.end()) throw std::runtime_error("FileSystem: unknown this_id");
    std::string a_filepath = read_string(r);
    std::vector<std::string> res = it->second->GetList(a_filepath);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_ARRAY); w.put_u32((u32)res.size());
    for (std::size_t i = 0; i < res.size(); ++i) {
        w.put_u8(T_STRING); w.put_string(res[i]);
    }
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_console_log(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n < 1) throw std::runtime_error("console.log: expected this_id");
    n -= 1;
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_console *>::const_iterator it = m_instances_console.find(this_id);
    if (it == m_instances_console.end()) throw std::runtime_error("console: unknown this_id");
    std::vector<std::string> a_args;
    a_args.reserve(n);
    for (u32 i = 0; i < n; ++i)
        a_args.push_back(_value_to_string(decode_value(r)));
    it->second->log(a_args);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_console_warning(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n < 1) throw std::runtime_error("console.warning: expected this_id");
    n -= 1;
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_console *>::const_iterator it = m_instances_console.find(this_id);
    if (it == m_instances_console.end()) throw std::runtime_error("console: unknown this_id");
    std::vector<std::string> a_args;
    a_args.reserve(n);
    for (u32 i = 0; i < n; ++i)
        a_args.push_back(_value_to_string(decode_value(r)));
    it->second->warning(a_args);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_console_error(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n < 1) throw std::runtime_error("console.error: expected this_id");
    n -= 1;
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_console *>::const_iterator it = m_instances_console.find(this_id);
    if (it == m_instances_console.end()) throw std::runtime_error("console: unknown this_id");
    std::vector<std::string> a_args;
    a_args.reserve(n);
    for (u32 i = 0; i < n; ++i)
        a_args.push_back(_value_to_string(decode_value(r)));
    it->second->error(a_args);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_KeyValueStorage_SetValueByKey(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 3) throw std::runtime_error("KeyValueStorage.SetValueByKey: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_KeyValueStorage *>::const_iterator it = m_instances_KeyValueStorage.find(this_id);
    if (it == m_instances_KeyValueStorage.end()) throw std::runtime_error("KeyValueStorage: unknown this_id");
    std::string a_name = read_string(r);
    value a_value = decode_value(r);
    it->second->SetValueByKey(a_name, a_value);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::cbm_KeyValueStorage_GetValueByKey(i32 cb_id, Reader &r) {
    u32 n = r.get_u32();
    if (n != 2) throw std::runtime_error("KeyValueStorage.GetValueByKey: wrong argument count");
    i32 this_id = (i32)read_int(r);
    std::map<i32, base_context_class_KeyValueStorage *>::const_iterator it = m_instances_KeyValueStorage.find(this_id);
    if (it == m_instances_KeyValueStorage.end()) throw std::runtime_error("KeyValueStorage: unknown this_id");
    std::string a_name = read_string(r);
    it->second->GetValueByKey(a_name);
    Writer w; w.put_i32(cb_id); w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    send_message(M_CALLBACK_RESP, w.data());
}


void context::context_impl::dispatch_callback(const std::vector<u8> &body) {
    Reader r(body.empty() ? 0 : &body[0], body.size());
    i32 cb_id = r.get_i32();
    std::string method = r.get_string();
    try {
        if (method == "GetFile") { cb_GetFile(cb_id, r); return; }
        if (method == "GetFileSize") { cb_GetFileSize(cb_id, r); return; }
        if (method == "GetTimeNow") { cb_GetTimeNow(cb_id, r); return; }
        if (method == "HasFile") { cb_HasFile(cb_id, r); return; }
        if (method == "GetFileList") { cb_GetFileList(cb_id, r); return; }
        if (method == "GetOpenedFiles") { cb_GetOpenedFiles(cb_id, r); return; }
        if (method == "CloseFiles") { cb_CloseFiles(cb_id, r); return; }
        if (method == "GetPi") { cb_GetPi(cb_id, r); return; }
        if (method == "GetServerPort") { cb_GetServerPort(cb_id, r); return; }
        if (method == "GetSize") { cbm_File_GetSize(cb_id, r); return; }
        if (method == "ReadBytes") { cbm_File_ReadBytes(cb_id, r); return; }
        if (method == "GetName") { cbm_File_GetName(cb_id, r); return; }
        if (method == "GetBufferSize") { cbm_File_GetBufferSize(cb_id, r); return; }
        if (method == "SetBufferSize") { cbm_File_SetBufferSize(cb_id, r); return; }
        if (method == "GetList") { cbm_FileSystem_GetList(cb_id, r); return; }
        if (method == "log") { cbm_console_log(cb_id, r); return; }
        if (method == "warning") { cbm_console_warning(cb_id, r); return; }
        if (method == "error") { cbm_console_error(cb_id, r); return; }
        if (method == "SetValueByKey") { cbm_KeyValueStorage_SetValueByKey(cb_id, r); return; }
        if (method == "GetValueByKey") { cbm_KeyValueStorage_GetValueByKey(cb_id, r); return; }
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
    if (protocol != 2)
        throw std::runtime_error("s5run: server protocol version " + num_to_str(protocol)
                                 + " is not supported (client supports 2)");
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
        register_type(m_pending_types[i].name, m_pending_types[i].methods);
    m_pending_types.clear();
    for (std::size_t i = 0; i < m_pending_objects.size(); ++i) {
        const PendingObject &o = m_pending_objects[i];
        Writer w;
        w.put_string(o.name); w.put_i32(o.this_id); w.put_string(o.name);
        w.put_u32((u32)o.methods.size());
        for (std::size_t j = 0; j < o.methods.size(); ++j) w.put_string(o.methods[j]);
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
                                      const std::vector<value> &args) {
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
void context::add_class(s5run_object *o) { m_impl->register_type(o->s5run_class_name(), o->s5run_method_names()); }
void context::add_object(s5run_object *o) { m_impl->add_object(o); }
void context::add_function(s5run_function *f) { m_impl->add_function(f); }

context * make_context() { return new context(); }

run_result run_script(context *ctx, const std::string &entry, const std::vector<value> &args) { return ctx->m_impl->run(entry, args); }

} // namespace s5run_cli_cpp98
