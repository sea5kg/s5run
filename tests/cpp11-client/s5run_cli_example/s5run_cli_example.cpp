// AUTO-GENERATED. DO NOT EDIT.
#include "s5run_cli_example.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace s5run_cli_example {

namespace {

// ---- binary wire codec (matches the s5run server protocol) ----

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
    M_STATS_REQ = 0x09, M_STATS_RESP = 0x0A
};

class Writer {
public:
    const std::vector<uint8_t> &data() const { return m_buf; }

    void put_u8(uint8_t v) { m_buf.push_back(v); }
    void put_u16(uint16_t v) {
        m_buf.push_back(v & 0xFF);
        m_buf.push_back((v >> 8) & 0xFF);
    }
    void put_u32(uint32_t v) {
        for (int i = 0; i < 4; ++i)
            m_buf.push_back((v >> (8 * i)) & 0xFF);
    }
    void put_u64(uint64_t v) {
        for (int i = 0; i < 8; ++i)
            m_buf.push_back((v >> (8 * i)) & 0xFF);
    }
    void put_i8(int8_t v) { put_u8((uint8_t)v); }
    void put_i16(int16_t v) { put_u16((uint16_t)v); }
    void put_i32(int32_t v) { put_u32((uint32_t)v); }
    void put_i64(int64_t v) { put_u64((uint64_t)v); }
    void put_f32(float v) {
        uint32_t u;
        std::memcpy(&u, &v, 4);
        put_u32(u);
    }
    void put_f64(double v) {
        uint64_t u;
        std::memcpy(&u, &v, 8);
        put_u64(u);
    }
    void put_string(const std::string &s) {
        size_t n = s.size();
        if (n <= 0xFF) {
            put_u8(S_LEN_1);
            put_u8((uint8_t)n);
        } else if (n <= 0xFFFF) {
            put_u8(S_LEN_2);
            put_u16((uint16_t)n);
        } else {
            put_u8(S_LEN_4);
            put_u32((uint32_t)n);
        }
        m_buf.insert(m_buf.end(), s.begin(), s.end());
    }

private:
    std::vector<uint8_t> m_buf;
};

class Reader {
public:
    Reader(const uint8_t *p, size_t n) : m_p(p), m_end(p + n) {}

    uint8_t get_u8() { check(1); return *m_p++; }
    uint16_t get_u16() {
        check(2);
        uint16_t v = m_p[0] | (m_p[1] << 8);
        m_p += 2;
        return v;
    }
    uint32_t get_u32() {
        check(4);
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i)
            v |= (uint32_t)m_p[i] << (8 * i);
        m_p += 4;
        return v;
    }
    uint64_t get_u64() {
        check(8);
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i)
            v |= (uint64_t)m_p[i] << (8 * i);
        m_p += 8;
        return v;
    }
    int8_t get_i8() { return (int8_t)get_u8(); }
    int16_t get_i16() { return (int16_t)get_u16(); }
    int32_t get_i32() { return (int32_t)get_u32(); }
    int64_t get_i64() { return (int64_t)get_u64(); }
    float get_f32() {
        uint32_t u = get_u32();
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    }
    double get_f64() {
        uint64_t u = get_u64();
        double d;
        std::memcpy(&d, &u, 8);
        return d;
    }
    std::string get_string() {
        uint8_t len_size = get_u8();
        size_t n = 0;
        if (len_size == S_LEN_1) {
            n = get_u8();
        } else if (len_size == S_LEN_2) {
            n = get_u16();
        } else if (len_size == S_LEN_4) {
            n = get_u32();
        } else {
            throw std::runtime_error("wire: bad string length size");
        }
        check(n);
        std::string s(reinterpret_cast<const char *>(m_p), n);
        m_p += n;
        return s;
    }

private:
    void check(size_t n) const {
        if (m_p + n > m_end)
            throw std::runtime_error("wire: unexpected end of buffer");
    }
    const uint8_t *m_p;
    const uint8_t *m_end;
};

int64_t read_int(Reader &r) {
    uint8_t tag = r.get_u8();
    switch (tag) {
    case T_INT8: return (int64_t)r.get_i8();
    case T_INT16: return (int64_t)r.get_i16();
    case T_INT32: return (int64_t)r.get_i32();
    case T_INT64: return r.get_i64();
    case T_UINT8: return (int64_t)r.get_u8();
    case T_UINT16: return (int64_t)r.get_u16();
    case T_UINT32: return (int64_t)r.get_u32();
    case T_UINT64: return (int64_t)r.get_u64();
    default:
        throw std::runtime_error("wire: expected an integer");
    }
}

void write_int(Writer &w, int64_t v) {
    if (v >= -128 && v <= 127) {
        w.put_u8(T_INT8);
        w.put_i8((int8_t)v);
    } else if (v >= -32768 && v <= 32767) {
        w.put_u8(T_INT16);
        w.put_i16((int16_t)v);
    } else if (v >= -2147483648LL && v <= 2147483647LL) {
        w.put_u8(T_INT32);
        w.put_i32((int32_t)v);
    } else {
        w.put_u8(T_INT64);
        w.put_i64(v);
    }
}



int32_t read_ref(Reader &r) {
    if (r.get_u8() != T_REF)
        throw std::runtime_error("wire: expected an object reference");
    int32_t this_id = r.get_i32();
    r.get_string(); // skip the type name
    return this_id;
}



std::string read_string(Reader &r) {
    if (r.get_u8() != T_STRING)
        throw std::runtime_error("wire: expected a string");
    return r.get_string();
}



void write_byte(Writer &w, uint8_t v) {
    w.put_u8(T_UINT8);
    w.put_u8(v);
}


void write_string(Writer &w, const std::string &v) {
    w.put_u8(T_STRING);
    w.put_string(v);
}


void write_bool(Writer &w, bool v) {
    w.put_u8(T_BOOL);
    w.put_u8(v ? 1 : 0);
}


void write_double(Writer &w, double v) {
    w.put_u8(T_FLOAT64);
    w.put_f64(v);
}



inline void write_array__byte(Writer &w, const std::vector<uint8_t> &v) {
    w.put_u8(T_ARRAY);
    w.put_u32((uint32_t)v.size());
    for (size_t i = 0; i < v.size(); ++i)
        write_byte(w, v[i]);
}

inline void write_array__string(Writer &w, const std::vector<std::string> &v) {
    w.put_u8(T_ARRAY);
    w.put_u32((uint32_t)v.size());
    for (size_t i = 0; i < v.size(); ++i)
        write_string(w, v[i]);
}

void encode_value(Writer &w, const value &v) {
    switch (v.get_kind()) {
    case value::K_NULL: w.put_u8(T_UNDEFINED); break;
    case value::K_BOOL: w.put_u8(T_BOOL); w.put_u8(v.as_bool() ? 1 : 0); break;
    case value::K_INT: write_int(w, v.as_int()); break;
    case value::K_DOUBLE: w.put_u8(T_FLOAT64); w.put_f64(v.as_double()); break;
    case value::K_STRING: write_string(w, v.as_string()); break;
    case value::K_ARRAY: {
        w.put_u8(T_ARRAY);
        const std::vector<value> &a = v.as_array();
        w.put_u32((uint32_t)a.size());
        for (size_t i = 0; i < a.size(); ++i)
            encode_value(w, a[i]);
        break;
    }
    case value::K_OBJECT: {
        w.put_u8(T_OBJECT);
        const std::map<std::string, value> &o = v.as_object();
        w.put_u32((uint32_t)o.size());
        for (std::map<std::string, value>::const_iterator it = o.begin();
             it != o.end(); ++it) {
            write_string(w, it->first);
            encode_value(w, it->second);
        }
        break;
    }
    case value::K_REF: {
        w.put_u8(T_REF);
        w.put_i32(v.ref_this_id());
        w.put_string(v.ref_type());
        break;
    }
    }
}

value decode_value(Reader &r) {
    uint8_t tag = r.get_u8();
    switch (tag) {
    case T_UNDEFINED: return value();
    case T_BOOL: return value(r.get_u8() != 0);
    case T_INT8: return value((int64_t)r.get_i8());
    case T_INT16: return value((int64_t)r.get_i16());
    case T_INT32: return value((int64_t)r.get_i32());
    case T_INT64: return value(r.get_i64());
    case T_UINT8: return value((int64_t)r.get_u8());
    case T_UINT16: return value((int64_t)r.get_u16());
    case T_UINT32: return value((int64_t)r.get_u32());
    case T_UINT64: return value((int64_t)r.get_u64());
    case T_FLOAT32: return value((double)r.get_f32());
    case T_FLOAT64: return value(r.get_f64());
    case T_STRING: return value(r.get_string());
    case T_ARRAY: {
        uint32_t n = r.get_u32();
        std::vector<value> a;
        a.reserve(n);
        for (uint32_t i = 0; i < n; ++i)
            a.push_back(decode_value(r));
        return value(a);
    }
    case T_OBJECT: {
        uint32_t n = r.get_u32();
        std::map<std::string, value> o;
        for (uint32_t i = 0; i < n; ++i) {
            std::string key = read_string(r);
            o[key] = decode_value(r);
        }
        return value(o);
    }
    case T_REF: {
        int32_t this_id = r.get_i32();
        std::string type = r.get_string();
        return value::make_ref(this_id, type);
    }
    default:
        throw std::runtime_error("wire: unknown value tag");
    }
}

std::string _value_to_string(const value &v) {
    switch (v.get_kind()) {
    case value::K_NULL: return "null";
    case value::K_BOOL: return v.as_bool() ? "true" : "false";
    case value::K_INT: return std::to_string(v.as_int());
    case value::K_DOUBLE: return std::to_string(v.as_double());
    case value::K_STRING: return v.as_string();
    default:
        throw std::runtime_error("wire: expected a scalar argument");
    }
}

} // namespace

// ---------------------------------------------------------------------------
// context::impl
// ---------------------------------------------------------------------------

class context::impl {
public:
    typedef std::function<void(int32_t, Reader &)> handler_t;

    impl() : m_fd(-1), m_next_this_id(0) {
        _register_method_handlers();
    }

    ~impl() {
        _disconnect();
    }

    void add_server(const std::shared_ptr<remote_server_info> &server);
    void set_script(const std::string &script);
    void register_type(const std::string &name,
                       const std::vector<std::string> &methods);
    void add_object(const std::shared_ptr<s5run_object> &obj);
    void add_function(const std::shared_ptr<s5run_function> &fn);
    std::shared_ptr<run_result> run(const std::string &entry,
                                 const std::vector<value> &args);

private:
    struct PendingType {
        std::string name;
        std::vector<std::string> methods;
    };
    struct PendingObject {
        std::string name;
        int32_t this_id;
        std::string type;
        std::vector<std::string> methods;
    };
    struct PendingFunction {
        std::string name;
    };

    void _cb_GetFile(int32_t cb_id, Reader &r);
    void _cb_GetFileSize(int32_t cb_id, Reader &r);
    void _cb_GetTimeNow(int32_t cb_id, Reader &r);
    void _cb_HasFile(int32_t cb_id, Reader &r);
    void _cb_GetFileList(int32_t cb_id, Reader &r);
    void _cb_GetOpenedFiles(int32_t cb_id, Reader &r);
    void _cb_CloseFiles(int32_t cb_id, Reader &r);
    void _cb_GetPi(int32_t cb_id, Reader &r);
    void _cb_GetServerPort(int32_t cb_id, Reader &r);
    void _cbm_File_GetSize(int32_t cb_id, Reader &r);
    void _cbm_File_ReadBytes(int32_t cb_id, Reader &r);
    void _cbm_FileSystem_GetList(int32_t cb_id, Reader &r);
    void _cbm_console_log(int32_t cb_id, Reader &r);
    void _cbm_console_warning(int32_t cb_id, Reader &r);
    void _cbm_console_error(int32_t cb_id, Reader &r);
    int32_t _register_instance_File(const std::shared_ptr<File> &obj);
    int32_t _register_instance_FileSystem(const std::shared_ptr<FileSystem> &obj);
    int32_t _register_instance_console(const std::shared_ptr<console> &obj);
    void _register_method_handlers();
    void _connect();
    void _disconnect();
    void _flush_pending();
    static bool _send_message_fd(int fd, uint8_t msg_type, const std::vector<uint8_t> &body);
    static bool _read_message_fd(int fd, uint8_t &msg_type, std::vector<uint8_t> &body);
    static bool _read_exact_fd(int fd, void *buf, size_t n);
    static bool _write_all_fd(int fd, const void *buf, size_t n);
    bool _send_message(uint8_t msg_type, const std::vector<uint8_t> &body);
    bool _read_message(uint8_t &msg_type, std::vector<uint8_t> &body);
    void _expect_register_resp(const std::string &what);
    void _dispatch_callback(std::vector<uint8_t> &body);
    static long _probe_server_load(const std::shared_ptr<remote_server_info> &server);
    static int _parse_error_line(const std::string &error);

    std::string m_script;
    std::vector<std::shared_ptr<remote_server_info>> m_servers;
    int m_fd;
    int32_t m_next_this_id;
    std::vector<PendingType> m_pending_types;
    std::vector<PendingObject> m_pending_objects;
    std::vector<PendingFunction> m_pending_functions;
    std::map<std::string, handler_t> m_handlers;
    typedef std::function<void(int32_t, const std::shared_ptr<s5run_object> &)> registrar_t;
    std::map<std::string, registrar_t> m_instance_registrars;
    typedef std::function<void(const std::shared_ptr<s5run_function> &)> fn_registrar_t;
    std::map<std::string, fn_registrar_t> m_function_registrars;
    std::shared_ptr<GlobalFunction_GetFile> m_fn_GetFile;
    std::shared_ptr<GlobalFunction_GetFileSize> m_fn_GetFileSize;
    std::shared_ptr<GlobalFunction_GetTimeNow> m_fn_GetTimeNow;
    std::shared_ptr<GlobalFunction_HasFile> m_fn_HasFile;
    std::shared_ptr<GlobalFunction_GetFileList> m_fn_GetFileList;
    std::shared_ptr<GlobalFunction_GetOpenedFiles> m_fn_GetOpenedFiles;
    std::shared_ptr<GlobalFunction_CloseFiles> m_fn_CloseFiles;
    std::shared_ptr<GlobalFunction_GetPi> m_fn_GetPi;
    std::shared_ptr<GlobalFunction_GetServerPort> m_fn_GetServerPort;
    std::map<int32_t, std::shared_ptr<File>> m_instances_File;
    std::map<int32_t, std::shared_ptr<FileSystem>> m_instances_FileSystem;
    std::map<int32_t, std::shared_ptr<console>> m_instances_console;
};

// ---------------------------------------------------------------------------
// context::impl - address / connection
// ---------------------------------------------------------------------------

void context::impl::set_script(const std::string &script) {
    m_script = script;
}

void context::impl::add_server(const std::shared_ptr<remote_server_info> &server) {
    if (!server)
        throw std::runtime_error("add_server: server is null");
    m_servers.push_back(server);
}

void context::impl::_connect() {
    if (m_fd >= 0)
        return;
    if (m_servers.empty())
        throw std::runtime_error("s5run: no server added (add_server)");
    size_t best = 0;
    if (m_servers.size() > 1) {
        // Ask every server for its load and pick the least loaded one.
        long best_load = -1;
        for (size_t i = 0; i < m_servers.size(); ++i) {
            long load = _probe_server_load(m_servers[i]);
            if (load < 0)
                continue; // unreachable
            if (best_load < 0 || load < best_load) {
                best_load = load;
                best = i;
            }
        }
        if (best_load < 0)
            throw std::runtime_error("s5run: all servers are unreachable");
    }
    const std::string m_host = m_servers[best]->get_host();
    const uint16_t m_port = m_servers[best]->get_port();
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("s5run: socket failed");
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    if (::inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr) != 1) {
        ::close(fd);
        throw std::runtime_error("s5run: invalid host (only IPv4 supported): " + m_host);
    }
    if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        ::close(fd);
        throw std::runtime_error("s5run: connect failed");
    }
    timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    m_fd = fd;
}

void context::impl::_disconnect() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool context::impl::_read_exact_fd(int fd, void *buf, size_t n) {
    char *p = (char *)buf;
    while (n > 0) {
        ssize_t got = ::recv(fd, p, n, 0);
        if (got == 0)
            return false;
        if (got < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        p += got;
        n -= (size_t)got;
    }
    return true;
}

bool context::impl::_write_all_fd(int fd, const void *buf, size_t n) {
    const char *p = (const char *)buf;
    while (n > 0) {
        ssize_t sent = ::send(fd, p, n, MSG_NOSIGNAL);
        if (sent <= 0) {
            if (sent < 0 && errno == EINTR)
                continue;
            return false;
        }
        p += sent;
        n -= (size_t)sent;
    }
    return true;
}

bool context::impl::_send_message_fd(int fd, uint8_t msg_type, const std::vector<uint8_t> &body) {
    uint32_t len = (uint32_t)(1 + body.size());
    uint32_t net_len = htonl(len);
    if (!_write_all_fd(fd, &net_len, sizeof(net_len)))
        return false;
    if (!_write_all_fd(fd, &msg_type, 1))
        return false;
    return body.empty() ? true : _write_all_fd(fd, &body[0], body.size());
}

bool context::impl::_send_message(uint8_t msg_type, const std::vector<uint8_t> &body) {
    return _send_message_fd(m_fd, msg_type, body);
}

bool context::impl::_read_message_fd(int fd, uint8_t &msg_type, std::vector<uint8_t> &body) {
    uint32_t net_len = 0;
    if (!_read_exact_fd(fd, &net_len, sizeof(net_len)))
        return false;
    uint32_t len = ntohl(net_len);
    if (len == 0 || len > 64 * 1024 * 1024)
        return false;
    std::vector<uint8_t> frame(len);
    if (!_read_exact_fd(fd, &frame[0], len))
        return false;
    msg_type = frame[0];
    body.assign(frame.begin() + 1, frame.end());
    return true;
}

bool context::impl::_read_message(uint8_t &msg_type, std::vector<uint8_t> &body) {
    return _read_message_fd(m_fd, msg_type, body);
}

void context::impl::_expect_register_resp(const std::string &what) {
    uint8_t msg_type = 0;
    std::vector<uint8_t> body;
    if (!_read_message(msg_type, body) || msg_type != M_REGISTER_RESP)
        throw std::runtime_error("s5run: " + what + " failed");
    Reader r(&body[0], body.size());
    if (r.get_u8() == 0)
        throw std::runtime_error("s5run: " + what + " error: " + r.get_string());
}

void context::impl::register_type(const std::string &name,
                                 const std::vector<std::string> &methods) {
    if (m_fd >= 0) {
        Writer w;
        w.put_string(name);
        w.put_u32((uint32_t)methods.size());
        for (size_t i = 0; i < methods.size(); ++i)
            w.put_string(methods[i]);
        _send_message(M_REGISTER_TYPE, w.data());
        _expect_register_resp("register_type(" + name + ")");
        return;
    }
    PendingType t;
    t.name = name;
    t.methods = methods;
    m_pending_types.push_back(t);
}

void context::impl::add_object(const std::shared_ptr<s5run_object> &obj) {
    const std::string class_name = obj->s5run_class_name();
    std::map<std::string, registrar_t>::const_iterator rit =
        m_instance_registrars.find(class_name);
    if (rit == m_instance_registrars.end())
        throw std::runtime_error("add_object: unknown class: " + class_name);
    int32_t id = ++m_next_this_id;
    rit->second(id, obj);
    const std::vector<std::string> methods = obj->s5run_method_names();
    if (m_fd >= 0) {
        Writer w;
        w.put_string(class_name);
        w.put_i32(id);
        w.put_string(class_name);
        w.put_u32((uint32_t)methods.size());
        for (size_t i = 0; i < methods.size(); ++i)
            w.put_string(methods[i]);
        _send_message(M_REGISTER_OBJ, w.data());
        _expect_register_resp("register_object(" + class_name + ")");
        return;
    }
    PendingObject o;
    o.name = class_name;
    o.this_id = id;
    o.type = class_name;
    o.methods = methods;
    m_pending_objects.push_back(o);
}

void context::impl::add_function(const std::shared_ptr<s5run_function> &fn) {
    const std::string name = fn->s5run_function_name();
    std::map<std::string, fn_registrar_t>::const_iterator rit =
        m_function_registrars.find(name);
    if (rit == m_function_registrars.end())
        throw std::runtime_error("add_function: unknown function: " + name);
    rit->second(fn);
    if (m_fd >= 0) {
        Writer w;
        w.put_string(name);
        _send_message(M_REGISTER_FUNC, w.data());
        _expect_register_resp("register_function(" + name + ")");
        return;
    }
    PendingFunction pf;
    pf.name = name;
    m_pending_functions.push_back(pf);
}

void context::impl::_cb_GetFile(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 1) {
        throw std::runtime_error("GetFile: expected 1 argument(s)");
    }
    std::string a_filepath = read_string(r);
    std::shared_ptr<File> res = m_fn_GetFile->execute(a_filepath);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
    int32_t obj_id = _register_instance_File(res);
    w.put_u8(T_REF);
    w.put_i32(obj_id);
    w.put_string("File");
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_GetFileSize(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 1) {
        throw std::runtime_error("GetFileSize: expected 1 argument(s)");
    }
    int32_t a_ref_id_file = read_ref(r);
    std::map<int32_t, std::shared_ptr<File>>::const_iterator a_it_file =
        m_instances_File.find(a_ref_id_file);
    if (a_it_file == m_instances_File.end()) {
        throw std::runtime_error("GetFileSize: unknown object reference: " + std::to_string(a_ref_id_file));
    }
    std::shared_ptr<File> a_file = a_it_file->second;
    int64_t res = m_fn_GetFileSize->execute(a_file);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_int(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_GetTimeNow(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 0) {
        throw std::runtime_error("GetTimeNow: expected 0 argument(s)");
    }
    int64_t res = m_fn_GetTimeNow->execute();
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_int(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_HasFile(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 1) {
        throw std::runtime_error("HasFile: expected 1 argument(s)");
    }
    std::string a_filepath = read_string(r);
    bool res = m_fn_HasFile->execute(a_filepath);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_bool(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_GetFileList(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 1) {
        throw std::runtime_error("GetFileList: expected 1 argument(s)");
    }
    std::string a_filepath = read_string(r);
    std::vector<std::string> res = m_fn_GetFileList->execute(a_filepath);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_array__string(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_GetOpenedFiles(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 0) {
        throw std::runtime_error("GetOpenedFiles: expected 0 argument(s)");
    }
    std::vector<std::shared_ptr<File>> res = m_fn_GetOpenedFiles->execute();
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
    w.put_u8(T_ARRAY);
    w.put_u32((uint32_t)res.size());
    for (size_t i = 0; i < res.size(); ++i) {
        int32_t obj_id = _register_instance_File(res[i]);
        w.put_u8(T_REF);
        w.put_i32(obj_id);
        w.put_string("File");
    }
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_CloseFiles(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 1) {
        throw std::runtime_error("CloseFiles: expected 1 argument(s)");
    }
    if (r.get_u8() != T_ARRAY)
        throw std::runtime_error("wire: expected array");
    uint32_t cnt_files = r.get_u32();
    std::vector<std::shared_ptr<File>> a_files;
    a_files.reserve(cnt_files);
    for (uint32_t i = 0; i < cnt_files; ++i) {
        int32_t a_ref_id_files = read_ref(r);
        std::map<int32_t, std::shared_ptr<File>>::const_iterator a_it_files =
            m_instances_File.find(a_ref_id_files);
        if (a_it_files == m_instances_File.end()) {
            throw std::runtime_error("CloseFiles: unknown object reference: " + std::to_string(a_ref_id_files));
        }
        a_files.push_back(a_it_files->second);
    }
    m_fn_CloseFiles->execute(a_files);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_GetPi(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 0) {
        throw std::runtime_error("GetPi: expected 0 argument(s)");
    }
    double res = m_fn_GetPi->execute();
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_double(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cb_GetServerPort(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 0) {
        throw std::runtime_error("GetServerPort: expected 0 argument(s)");
    }
    uint64_t res = m_fn_GetServerPort->execute();
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_int(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cbm_File_GetSize(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 1) {
        throw std::runtime_error("File.GetSize: expected 1 argument(s)");
    }
    int32_t this_id = (int32_t)read_int(r);
    std::map<int32_t, std::shared_ptr<File>>::const_iterator it =
        m_instances_File.find(this_id);
    if (it == m_instances_File.end()) {
        throw std::runtime_error("File: unknown this_id");
    }
    int64_t res = it->second->GetSize();
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_int(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cbm_File_ReadBytes(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 2) {
        throw std::runtime_error("File.ReadBytes: expected 2 argument(s)");
    }
    int32_t this_id = (int32_t)read_int(r);
    std::map<int32_t, std::shared_ptr<File>>::const_iterator it =
        m_instances_File.find(this_id);
    if (it == m_instances_File.end()) {
        throw std::runtime_error("File: unknown this_id");
    }
    int64_t a_readBytes = read_int(r);
    std::vector<uint8_t> res = it->second->ReadBytes(a_readBytes);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_array__byte(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cbm_FileSystem_GetList(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n != 2) {
        throw std::runtime_error("FileSystem.GetList: expected 2 argument(s)");
    }
    int32_t this_id = (int32_t)read_int(r);
    std::map<int32_t, std::shared_ptr<FileSystem>>::const_iterator it =
        m_instances_FileSystem.find(this_id);
    if (it == m_instances_FileSystem.end()) {
        throw std::runtime_error("FileSystem: unknown this_id");
    }
    std::string a_filepath = read_string(r);
    std::vector<std::string> res = it->second->GetList(a_filepath);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
        write_array__string(w, res);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cbm_console_log(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n < 1) {
        throw std::runtime_error("console.log: expected this_id");
    }
    int32_t this_id = (int32_t)read_int(r);
    std::map<int32_t, std::shared_ptr<console>>::const_iterator it =
        m_instances_console.find(this_id);
    if (it == m_instances_console.end()) {
        throw std::runtime_error("console: unknown this_id");
    }
    n -= 1;
    std::vector<std::string> a_args;
    a_args.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
        a_args.push_back(_value_to_string(decode_value(r)));
    it->second->log(a_args);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cbm_console_warning(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n < 1) {
        throw std::runtime_error("console.warning: expected this_id");
    }
    int32_t this_id = (int32_t)read_int(r);
    std::map<int32_t, std::shared_ptr<console>>::const_iterator it =
        m_instances_console.find(this_id);
    if (it == m_instances_console.end()) {
        throw std::runtime_error("console: unknown this_id");
    }
    n -= 1;
    std::vector<std::string> a_args;
    a_args.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
        a_args.push_back(_value_to_string(decode_value(r)));
    it->second->warning(a_args);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    _send_message(M_CALLBACK_RESP, w.data());
}

void context::impl::_cbm_console_error(int32_t cb_id, Reader &r) {
    uint32_t n = r.get_u32();
    if (n < 1) {
        throw std::runtime_error("console.error: expected this_id");
    }
    int32_t this_id = (int32_t)read_int(r);
    std::map<int32_t, std::shared_ptr<console>>::const_iterator it =
        m_instances_console.find(this_id);
    if (it == m_instances_console.end()) {
        throw std::runtime_error("console: unknown this_id");
    }
    n -= 1;
    std::vector<std::string> a_args;
    a_args.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
        a_args.push_back(_value_to_string(decode_value(r)));
    it->second->error(a_args);
    Writer w;
    w.put_i32(cb_id);
    w.put_u8(1);
    w.put_u8(T_UNDEFINED);
    _send_message(M_CALLBACK_RESP, w.data());
}

int32_t context::impl::_register_instance_File(const std::shared_ptr<File> &obj) {
    int32_t id = ++m_next_this_id;
    m_instances_File[id] = obj;
    return id;
}

int32_t context::impl::_register_instance_FileSystem(const std::shared_ptr<FileSystem> &obj) {
    int32_t id = ++m_next_this_id;
    m_instances_FileSystem[id] = obj;
    return id;
}

int32_t context::impl::_register_instance_console(const std::shared_ptr<console> &obj) {
    int32_t id = ++m_next_this_id;
    m_instances_console[id] = obj;
    return id;
}

void context::impl::_register_method_handlers() {
    m_instance_registrars["File"] = [this](int32_t id, const std::shared_ptr<s5run_object> &obj) {
        m_instances_File[id] = std::static_pointer_cast<File>(obj);
    };
    m_instance_registrars["FileSystem"] = [this](int32_t id, const std::shared_ptr<s5run_object> &obj) {
        m_instances_FileSystem[id] = std::static_pointer_cast<FileSystem>(obj);
    };
    m_instance_registrars["console"] = [this](int32_t id, const std::shared_ptr<s5run_object> &obj) {
        m_instances_console[id] = std::static_pointer_cast<console>(obj);
    };
    m_function_registrars["GetFile"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_GetFile = std::static_pointer_cast<GlobalFunction_GetFile>(fn);
        m_handlers["GetFile"] = [this](int32_t cb_id, Reader &r) {
            _cb_GetFile(cb_id, r);
        };
    };
    m_function_registrars["GetFileSize"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_GetFileSize = std::static_pointer_cast<GlobalFunction_GetFileSize>(fn);
        m_handlers["GetFileSize"] = [this](int32_t cb_id, Reader &r) {
            _cb_GetFileSize(cb_id, r);
        };
    };
    m_function_registrars["GetTimeNow"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_GetTimeNow = std::static_pointer_cast<GlobalFunction_GetTimeNow>(fn);
        m_handlers["GetTimeNow"] = [this](int32_t cb_id, Reader &r) {
            _cb_GetTimeNow(cb_id, r);
        };
    };
    m_function_registrars["HasFile"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_HasFile = std::static_pointer_cast<GlobalFunction_HasFile>(fn);
        m_handlers["HasFile"] = [this](int32_t cb_id, Reader &r) {
            _cb_HasFile(cb_id, r);
        };
    };
    m_function_registrars["GetFileList"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_GetFileList = std::static_pointer_cast<GlobalFunction_GetFileList>(fn);
        m_handlers["GetFileList"] = [this](int32_t cb_id, Reader &r) {
            _cb_GetFileList(cb_id, r);
        };
    };
    m_function_registrars["GetOpenedFiles"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_GetOpenedFiles = std::static_pointer_cast<GlobalFunction_GetOpenedFiles>(fn);
        m_handlers["GetOpenedFiles"] = [this](int32_t cb_id, Reader &r) {
            _cb_GetOpenedFiles(cb_id, r);
        };
    };
    m_function_registrars["CloseFiles"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_CloseFiles = std::static_pointer_cast<GlobalFunction_CloseFiles>(fn);
        m_handlers["CloseFiles"] = [this](int32_t cb_id, Reader &r) {
            _cb_CloseFiles(cb_id, r);
        };
    };
    m_function_registrars["GetPi"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_GetPi = std::static_pointer_cast<GlobalFunction_GetPi>(fn);
        m_handlers["GetPi"] = [this](int32_t cb_id, Reader &r) {
            _cb_GetPi(cb_id, r);
        };
    };
    m_function_registrars["GetServerPort"] = [this](const std::shared_ptr<s5run_function> &fn) {
        m_fn_GetServerPort = std::static_pointer_cast<GlobalFunction_GetServerPort>(fn);
        m_handlers["GetServerPort"] = [this](int32_t cb_id, Reader &r) {
            _cb_GetServerPort(cb_id, r);
        };
    };
    m_handlers["GetSize"] = [this](int32_t cb_id, Reader &r) {
        _cbm_File_GetSize(cb_id, r);
    };
    m_handlers["ReadBytes"] = [this](int32_t cb_id, Reader &r) {
        _cbm_File_ReadBytes(cb_id, r);
    };
    m_handlers["GetList"] = [this](int32_t cb_id, Reader &r) {
        _cbm_FileSystem_GetList(cb_id, r);
    };
    m_handlers["log"] = [this](int32_t cb_id, Reader &r) {
        _cbm_console_log(cb_id, r);
    };
    m_handlers["warning"] = [this](int32_t cb_id, Reader &r) {
        _cbm_console_warning(cb_id, r);
    };
    m_handlers["error"] = [this](int32_t cb_id, Reader &r) {
        _cbm_console_error(cb_id, r);
    };
}

void context::impl::_flush_pending() {
    for (size_t i = 0; i < m_pending_types.size(); ++i) {
        const PendingType &t = m_pending_types[i];
        Writer w;
        w.put_string(t.name);
        w.put_u32((uint32_t)t.methods.size());
        for (size_t j = 0; j < t.methods.size(); ++j)
            w.put_string(t.methods[j]);
        _send_message(M_REGISTER_TYPE, w.data());
        _expect_register_resp("register_type(" + t.name + ")");
    }
    m_pending_types.clear();
    for (size_t i = 0; i < m_pending_objects.size(); ++i) {
        const PendingObject &o = m_pending_objects[i];
        Writer w;
        w.put_string(o.name);
        w.put_i32(o.this_id);
        w.put_string(o.type);
        w.put_u32((uint32_t)o.methods.size());
        for (size_t j = 0; j < o.methods.size(); ++j)
            w.put_string(o.methods[j]);
        _send_message(M_REGISTER_OBJ, w.data());
        _expect_register_resp("register_object(" + o.name + ")");
    }
    m_pending_objects.clear();
    for (size_t i = 0; i < m_pending_functions.size(); ++i) {
        const PendingFunction &pf = m_pending_functions[i];
        Writer w;
        w.put_string(pf.name);
        _send_message(M_REGISTER_FUNC, w.data());
        _expect_register_resp("register_function(" + pf.name + ")");
    }
    m_pending_functions.clear();
}

void context::impl::_dispatch_callback(std::vector<uint8_t> &body) {
    Reader r(&body[0], body.size());
    int32_t cb_id = r.get_i32();
    std::string method = r.get_string();
    std::map<std::string, handler_t>::const_iterator it = m_handlers.find(method);
    if (it == m_handlers.end()) {
        Writer w;
        w.put_i32(cb_id);
        w.put_u8(0);
        w.put_string("unknown method: " + method);
        _send_message(M_CALLBACK_RESP, w.data());
        return;
    }
    try {
        it->second(cb_id, r);
    } catch (const std::exception &e) {
        Writer w;
        w.put_i32(cb_id);
        w.put_u8(0);
        w.put_string(e.what());
        _send_message(M_CALLBACK_RESP, w.data());
    }
}

long context::impl::_probe_server_load(const std::shared_ptr<remote_server_info> &server) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->get_port());
    if (::inet_pton(AF_INET, server->get_host().c_str(), &addr.sin_addr) != 1) {
        ::close(fd);
        return -1;
    }
    if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }
    timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    std::vector<uint8_t> empty_body;
    if (!_send_message_fd(fd, M_STATS_REQ, empty_body)) {
        ::close(fd);
        return -1;
    }
    uint8_t msg_type = 0;
    std::vector<uint8_t> body;
    if (!_read_message_fd(fd, msg_type, body) || msg_type != M_STATS_RESP || body.size() < 16) {
        ::close(fd);
        return -1;
    }
    ::close(fd);
    Reader r(&body[0], body.size());
    r.get_u32(); // threads
    uint32_t busy = r.get_u32();
    uint32_t queued = r.get_u32();
    r.get_u32(); // clients
    return (long)busy + (long)queued;
}

int context::impl::_parse_error_line(const std::string &error) {
    // Engine errors carry positions as ":LINE:COLUMN" (e.g. "at main (<script>:5:15)").
    for (size_t pos = 0; pos < error.size(); ++pos) {
        if (error[pos] != ':')
            continue;
        size_t p1 = pos + 1;
        if (p1 >= error.size() || error[p1] < '0' || error[p1] > '9')
            continue;
        size_t p2 = p1;
        while (p2 < error.size() && error[p2] >= '0' && error[p2] <= '9')
            ++p2;
        if (p2 + 1 < error.size() && error[p2] == ':' && error[p2 + 1] >= '0' && error[p2 + 1] <= '9')
            return atoi(error.substr(p1, p2 - p1).c_str());
        pos = p2;
    }
    return 0;
}

std::shared_ptr<run_result> context::impl::run(const std::string &entry,
                                 const std::vector<value> &args) {
    std::shared_ptr<run_result> out(new run_result());
    clock_t started = clock();
    try {
        _connect();
        _flush_pending();
        Writer w;
        w.put_string(m_script);
        w.put_string(entry);
        w.put_u32((uint32_t)args.size());
        for (size_t i = 0; i < args.size(); ++i)
            encode_value(w, args[i]);
        if (!_send_message(M_EXEC_REQ, w.data()))
            throw std::runtime_error("s5run: send failed");
        while (true) {
            uint8_t msg_type = 0;
            std::vector<uint8_t> body;
            if (!_read_message(msg_type, body))
                throw std::runtime_error("s5run: connection closed");
            if (msg_type == M_CALLBACK_REQ) {
                _dispatch_callback(body);
                continue;
            }
            if (msg_type == M_EXEC_RESP) {
                Reader r(&body[0], body.size());
                if (r.get_u8() == 0)
                    throw std::runtime_error(r.get_string());
                out->m_value = decode_value(r);
                break;
            }
            throw std::runtime_error("s5run: unexpected message type");
        }
    } catch (const std::exception &e) {
        out->m_has_error = true;
        out->m_error = e.what();
        out->m_error_line = _parse_error_line(out->m_error);
    }
    out->m_duration_ms = (int64_t)((double)(clock() - started) * 1000.0 / CLOCKS_PER_SEC);
    return out;
}

// ---------------------------------------------------------------------------
// context
// ---------------------------------------------------------------------------

context::context() : m_impl(new context::impl()) {
}

context::~context() {
    delete m_impl;
}

void remote_server_info::set_address(const std::string &address) {
    const std::string prefix = "s5run://";
    if (address.compare(0, prefix.size(), prefix) != 0)
        throw std::runtime_error("set_address: expected format s5run://host:port");
    std::string rest = address.substr(prefix.size());
    size_t colon = rest.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= rest.size())
        throw std::runtime_error("set_address: expected format s5run://host:port");
    m_address = address;
    m_host = rest.substr(0, colon);
    m_port = (uint16_t)atoi(rest.substr(colon + 1).c_str());
    m_have_address = true;
}

void context::add_server(const std::shared_ptr<remote_server_info> &server) {
    m_impl->add_server(server);
}

void context::add_context_provider(const std::string &address,
                                  const std::string &api_key,
                                  const std::vector<std::string> &contexts) {
    (void)api_key;
    const std::string prefix = "s5run://";
    if (address.compare(0, prefix.size(), prefix) != 0)
        throw std::runtime_error("add_context_provider: expected format s5run://host:port");
    if (contexts.empty())
        throw std::runtime_error("add_context_provider: contexts list is empty");
    throw std::runtime_error("add_context_provider: not implemented yet");
}

void context::add_object(s5run_object *obj) {
    m_impl->add_object(std::shared_ptr<s5run_object>(obj));
}

void context::add_object(const std::shared_ptr<s5run_object> &obj) {
    m_impl->add_object(obj);
}

void context::add_class(const std::shared_ptr<s5run_object> &sample) {
    m_impl->register_type(sample->s5run_class_name(),
                          sample->s5run_method_names());
}

void context::add_function(const std::shared_ptr<s5run_function> &fn) {
    m_impl->add_function(fn);
}

void context::set_script(const std::string &script) {
    m_impl->set_script(script);
}

std::shared_ptr<run_result> context::_run(const std::string &entry,
                                      const std::vector<value> &args) {
    return m_impl->run(entry, args);
}

std::shared_ptr<context> make_context() {
    return std::shared_ptr<context>(new context());
}

} // namespace s5run_cli_example
