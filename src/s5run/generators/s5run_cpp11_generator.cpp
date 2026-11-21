/**********************************************************************************
 *
 *            ░░░░░░░
 *           ░██▓▓▓▓▒
 *  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 *  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 *  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ▒▒░
 *
 * MIT License
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

#include "s5run_generators.h"
#include "s5run_schema.h"

#include <cstdio>
#include <fstream>
#include <sys/stat.h>

namespace s5run {
namespace generators {

class cpp11_generator : public IGenerator {
public:
  std::vector<std::string> generate(const schema &s,
                                     const std::string &output_dir) override;

private:
  std::string generate_header(const schema &s) const;
  std::string generate_impl(const schema &s) const;
};

// -----------------------------------------------------------------------
// Type helpers
// -----------------------------------------------------------------------

static bool is_ref(const std::string &t) {
  return t != "bool" && t != "int" && t != "uint" && t != "float" && t != "double" &&
         t != "string" && t != "byte" && t != "void" && t.substr(0, 6) != "array<";
}
static std::string cpp11_type(const std::string &t) {
  if (t == "bool") return "bool";
  if (t == "int" || t == "uint") return "int64_t";
  if (t == "float" || t == "double") return "double";
  if (t == "string") return "std::string";
  if (t == "byte") return "uint8_t";
  if (t.substr(0, 6) == "array<") {
    std::string inner = t.substr(6, t.size() - 7);
    if (is_ref(inner))
      return "std::vector<std::shared_ptr<" + inner + ">>";
    return "std::vector<" + cpp11_type(inner) + ">";
  }
  return "std::shared_ptr<" + t + ">"; // class reference
}

static std::string cpp11_shared(const std::string &t) {
  if (t == "bool" || t == "int" || t == "uint" || t == "float" || t == "double" ||
      t == "string" || t == "byte" || t == "void")
    return cpp11_type(t);
  if (t.substr(0, 6) == "array<")
    return "std::vector<" + cpp11_shared(t.substr(6, t.size() - 7)) + ">";
  return "std::shared_ptr<" + t + ">";
}

static bool is_void(const std::string &t) { return t == "void"; }
static std::string cpp11_param(const std::string &t) {
  if (is_ref(t)) return "const std::shared_ptr<" + t + "> &";
  std::string ct = cpp11_type(t);
  if (ct.substr(0, 5) == "std::") return "const " + ct + " &";
  return ct;
}
static bool has_convert(const std::vector<schema_argument> &args) {
  return args.size() == 1 && !args[0].convert_from.empty();
}

// -----------------------------------------------------------------------
// Static template blocks
// -----------------------------------------------------------------------

static const char *VALUE_CLASS = R"GEN(class value {
public:
    enum kind {
        K_NULL, K_BOOL, K_INT, K_DOUBLE, K_STRING, K_ARRAY, K_OBJECT, K_REF
    };

    value() : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0) {}
    value(bool b) : m_kind(K_BOOL), m_b(b), m_i(0), m_d(0.0) {}
    value(int i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
    value(int64_t i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
    value(double d) : m_kind(K_DOUBLE), m_b(false), m_i(0), m_d(d) {}
    value(const char *s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
    value(const std::string &s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
    value(const std::vector<value> &a) : m_kind(K_ARRAY), m_b(false), m_i(0), m_d(0.0), m_a(a) {}
    value(const std::map<std::string, value> &o) : m_kind(K_OBJECT), m_b(false), m_i(0), m_d(0.0), m_o(o) {}

    static value make_ref(int this_id, const std::string &type) {
        value v;
        v.m_kind = K_REF;
        v.m_i = this_id;
        v.m_s = type;
        return v;
    }

    kind get_kind() const { return m_kind; }
    bool is_null() const { return m_kind == K_NULL; }

    bool as_bool() const { return m_b; }
    int64_t as_int() const { return m_i; }
    double as_double() const { return m_d; }
    const std::string &as_string() const { return m_s; }
    const std::vector<value> &as_array() const { return m_a; }
    const std::map<std::string, value> &as_object() const { return m_o; }
    int ref_this_id() const { return (int)m_i; }
    const std::string &ref_type() const { return m_s; }

private:
    kind m_kind;
    bool m_b;
    int64_t m_i;
    double m_d;
    std::string m_s;
    std::vector<value> m_a;
    std::map<std::string, value> m_o;
};)GEN";

static const char *REMOTE_SERVER = R"GEN(class remote_server_info {
public:
    remote_server_info() : m_port(0), m_have_address(false) {}

    // Address format: s5run://host:port
    void set_address(const std::string &address);
    const std::string &get_address() const { return m_address; }

    void set_api_key(const std::string &api_key) { m_api_key = api_key; }
    const std::string &get_api_key() const { return m_api_key; }

    // Valid after a successful set_address().
    const std::string &get_host() const { return m_host; }
    uint16_t get_port() const { return m_port; }

private:
    std::string m_address;
    std::string m_api_key;
    std::string m_host;
    uint16_t m_port;
    bool m_have_address;
};)GEN";

static const char *S5RUN_OBJECT = R"GEN(class s5run_object {
public:
    virtual ~s5run_object() {}
    virtual std::string s5run_class_name() const = 0;
    virtual std::vector<std::string> s5run_method_names() const = 0;
};)GEN";

static const char *S5RUN_FUNCTION = R"GEN(class s5run_function {
public:
    virtual ~s5run_function() {}
    virtual std::string s5run_function_name() const = 0;
};)GEN";

static const char *CONTEXT_CLASS = R"GEN(class context {
public:
    context();
    ~context();

    // Version of the generated client code.
    static const char *client_version() { return "1.0.0"; }

    // Address format: s5run://host:port
    // Adds a server the context talks to. The first added server
    // is used for script execution; the api_key is reserved for
    // future authentication.
    void add_server(const std::shared_ptr<remote_server_info> &server);

    // Registers a remote context provider: the listed context
    // names (global objects / classes) will be supplied by the
    // s5run server at the given address. Stub, not implemented yet.
    void add_context_provider(const std::string &address,
                              const std::string &api_key,
                              const std::vector<std::string> &contexts);

    // Registers the object type on the server (class name + methods).
    // Pass any instance of a generated base class; only its type
    // information is used, the instance itself is not stored.
    void add_class(const std::shared_ptr<s5run_object> &sample);

    // Registers a singleton object, visible in scripts as
    // `name.method(...)`. The raw pointer overload takes ownership.
    void add_object(s5run_object *obj);
    void add_object(const std::shared_ptr<s5run_object> &obj);

    // Registers a global function implementation.
    void add_function(const std::shared_ptr<s5run_function> &fn);

    // Sets the script executed by run_script().
    void set_script(const std::string &script);

    class impl; // public forward declaration: run_result befriends it

private:
    context(const context &);
    context &operator=(const context &);
    std::shared_ptr<run_result> _run(const std::string &entry, const std::vector<value> &args);
    friend std::shared_ptr<run_result> run_script(const std::shared_ptr<context> &,
                                                const std::string &, const std::vector<value> &);
    impl *m_impl;
};)GEN";

static const char *RUN_RESULT = R"GEN(class run_result {
public:
    bool has_error() const { return m_has_error; }
    const std::string &error() const { return m_error; }
    int error_line() const { return m_error_line; }
    int64_t duration() const { return m_duration_ms; } // milliseconds
    const value &get_value() const { return m_value; }

private:
    friend class context::impl;
    run_result() : m_has_error(false), m_error_line(0), m_duration_ms(0) {}

    bool m_has_error;
    std::string m_error;
    int m_error_line;
    int64_t m_duration_ms;
    value m_value;
};)GEN";

static const char *RUN_SCRIPT_DECL = R"GEN(// Runs the context script; serves callbacks while it executes.
// Never throws: all errors are reported via run_result.
inline std::shared_ptr<run_result> run_script(
    const std::shared_ptr<context> &ctx,
    const std::string &entry,
    const std::vector<value> &args = std::vector<value>()) {
    return ctx->_run(entry, args);
})GEN";

static const char *CODEC = R"GEN(namespace {

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
    M_STATS_REQ = 0x09, M_STATS_RESP = 0x0A, M_SERVER_HELLO = 0x0F
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

} // namespace)GEN";

static const char *CONNECT = R"GEN(void context::impl::_connect() {
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
    _read_hello(m_fd);
}

void context::impl::_read_hello(int fd) {
    uint8_t msg_type = 0;
    std::vector<uint8_t> body;
    if (!_read_message_fd(fd, msg_type, body) || msg_type != M_SERVER_HELLO)
        throw std::runtime_error("s5run: expected server hello");
    std::string text(body.begin(), body.end());
    if (text.compare(0, 6, "s5run ") != 0)
        throw std::runtime_error("s5run: bad server hello: " + text);
    size_t last_space = text.rfind(' ');
    if (last_space == std::string::npos)
        throw std::runtime_error("s5run: bad server hello: " + text);
    int protocol = atoi(text.substr(last_space + 1).c_str());
    if (protocol != 2)
        throw std::runtime_error("s5run: server protocol version " + std::to_string(protocol) + " is not supported (client supports 2)");
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

void context::impl::set_script(const std::string &script) {
    m_script = script;
}

void context::impl::add_server(const std::shared_ptr<remote_server_info> &server) {
    m_servers.push_back(server);
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

)GEN";

static const char *FLUSH = R"GEN(void context::impl::_flush_pending() {
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
    _read_hello(fd);
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

)GEN";

static const char *RUN_BLOCK = R"GEN(std::shared_ptr<run_result> context::impl::run(const std::string &entry,
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

)GEN";

static const char *CONTEXT_METHODS = R"GEN(context::context() : m_impl(new context::impl()) {
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
)GEN";

// -----------------------------------------------------------------------
// Dynamic code generation helpers
// -----------------------------------------------------------------------

static void emit_decode_args(std::string &out, const std::vector<schema_argument> &args,
                              const std::string &owner) {
  if (has_convert(args)) {
    out += "        " + cpp11_type(args[0].type) + " a_" + args[0].name + ";\n";
    out += "        a_" + args[0].name + ".reserve(n);\n";
    out += "        for (uint32_t i = 0; i < n; ++i)\n";
    out += "            a_" + args[0].name + ".push_back(_value_to_string(decode_value(r)));\n";
    return;
  }
  for (const auto &a : args) {
    if (a.type.substr(0, 6) == "array<") {
      std::string inner = a.type.substr(6, a.type.size() - 7);
      if (is_ref(inner)) {
        std::string cls = inner;
        out += "        if (r.get_u8() != T_ARRAY) throw std::runtime_error(\"wire: expected array\");\n";
        out += "        uint32_t cnt_" + a.name + " = r.get_u32();\n";
        out += "        std::vector<std::shared_ptr<" + cls + ">> a_" + a.name + ";\n";
        out += "        a_" + a.name + ".reserve(cnt_" + a.name + ");\n";
        out += "        for (uint32_t i = 0; i < cnt_" + a.name + "; ++i) {\n";
        out += "            int32_t a_ref_id = read_ref(r);\n";
        out += "            auto a_it = m_instances_" + cls + ".find(a_ref_id);\n";
        out += "            if (a_it == m_instances_" + cls + ".end())\n";
        out += "                throw std::runtime_error(\"" + owner + ": unknown object reference\");\n";
        out += "            a_" + a.name + ".push_back(a_it->second);\n";
        out += "        }\n";
      } else {
        out += "        std::vector<" + cpp11_type(inner) + "> a_" + a.name + " = read_" + inner + "(r);\n";
      }
    } else if (is_ref(a.type)) {
      out += "        int32_t a_ref_id_" + a.name + " = read_ref(r);\n";
      out += "        auto a_it_" + a.name + " = m_instances_" + a.type + ".find(a_ref_id_" + a.name + ");\n";
      out += "        if (a_it_" + a.name + " == m_instances_" + a.type + ".end())\n";
      out += "            throw std::runtime_error(\"" + owner + ": unknown object reference\");\n";
      out += "        std::shared_ptr<" + a.type + "> a_" + a.name + " = a_it_" + a.name + "->second;\n";
    } else {
      std::string fn;
      if (a.type == "bool") fn = "read_bool(r)";
      else if (a.type == "int" || a.type == "uint") fn = "read_int(r)";
      else if (a.type == "float" || a.type == "double") fn = "read_double(r)";
      else if (a.type == "string") fn = "read_string(r)";
      else if (a.type == "byte") fn = "read_byte(r)";
      else fn = "read_" + a.type + "(r)";
      out += "        " + cpp11_type(a.type) + " a_" + a.name + " = " + fn + ";\n";
    }
  }
}

static void emit_encode_result(std::string &out, const std::string &ret) {
  if (is_void(ret)) {
    out += "        w.put_u8(T_UNDEFINED);\n";
  } else if (is_ref(ret)) {
    out += "        int32_t obj_id = _register_instance_" + ret + "(res);\n";
    out += "        w.put_u8(T_REF); w.put_i32(obj_id); w.put_string(\"" + ret + "\");\n";
  } else if (ret == "bool") {
    out += "        write_bool(w, res);\n";
  } else if (ret == "int" || ret == "uint") {
    out += "        write_int(w, res);\n";
  } else if (ret == "float" || ret == "double") {
    out += "        write_double(w, res);\n";
  } else if (ret == "string") {
    out += "        write_string(w, res);\n";
  } else if (ret == "byte") {
    out += "        write_byte(w, res);\n";
  } else if (ret.substr(0, 6) == "array<") {
    std::string inner = ret.substr(6, ret.size() - 7);
    if (is_ref(inner)) {
      out += "        w.put_u8(T_ARRAY);\n        w.put_u32((uint32_t)res.size());\n";
      out += "        for (size_t i = 0; i < res.size(); ++i) {\n";
      out += "            int32_t obj_id = _register_instance_" + inner + "(res[i]);\n";
      out += "            w.put_u8(T_REF); w.put_i32(obj_id); w.put_string(\"" + inner + "\");\n";
      out += "        }\n";
    } else {
      std::string fn;
      if (inner == "bool") fn = "write_bool(w, res[i])";
      else if (inner == "int" || inner == "uint") fn = "write_int(w, res[i])";
      else if (inner == "float" || inner == "double") fn = "write_double(w, res[i])";
      else if (inner == "string") fn = "write_string(w, res[i])";
      else if (inner == "byte") fn = "write_byte(w, res[i])";
      out += "        w.put_u8(T_ARRAY); w.put_u32((uint32_t)res.size());\n";
      out += "        for (size_t i = 0; i < res.size(); ++i)\n";
      out += "            " + fn + ";\n";
    }
  }
}

// -----------------------------------------------------------------------
// Header generation
// -----------------------------------------------------------------------

std::string cpp11_generator::generate_header(const schema &s) const {
  const std::string &ns = s.cpp11.namespace_;
  std::string out;

  out += "// AUTO-GENERATED. DO NOT EDIT.\n#pragma once\n\n";
  out += "#include <cstdint>\n#include <map>\n#include <memory>\n#include <string>\n#include <vector>\n\n";
  out += "namespace " + ns + " {\n\n";
  out += "class context;\nclass remote_server_info;\nclass run_result;\n";
  out += "std::shared_ptr<context> make_context();\n\n";
  out += VALUE_CLASS;
  out += std::string("\n") + REMOTE_SERVER;
  out += std::string("\n") + S5RUN_OBJECT;
  out += "\n";

  // Schema base classes
  for (const auto &c : s.classes) {
    out += "\nclass " + c.name + " : public s5run_object {\npublic:\n";
    out += "    std::string s5run_class_name() const override { return \"" + c.name + "\"; }\n";
    out += "    std::vector<std::string> s5run_method_names() const override {\n";
    out += "        std::vector<std::string> out_;\n";
    for (const auto &m : c.methods)
      out += "        out_.push_back(\"" + m.name + "\");\n";
    out += "        return out_;\n    }\n";
    out += "    virtual ~" + c.name + "() {}\n";
    for (const auto &m : c.methods) {
      std::string ret = is_void(m.return_type) ? "void" : cpp11_shared(m.return_type);
      std::string args;
      for (size_t k = 0; k < m.arguments.size(); ++k) {
        if (k > 0) args += ", ";
        args += cpp11_param(m.arguments[k].type) + " " + m.arguments[k].name;
      }
      out += "    virtual " + ret + " " + m.name + "(" + args + ") = 0;\n";
    }
    out += "};\n";
  }

  out += std::string("\n") + S5RUN_FUNCTION;
  out += "\n";

  // GlobalFunction_* interfaces
  for (const auto &f : s.functions) {
    std::string ret = is_void(f.return_type) ? "void" : cpp11_shared(f.return_type);
    std::string args;
    for (size_t k = 0; k < f.arguments.size(); ++k) {
      if (k > 0) args += ", ";
      args += cpp11_param(f.arguments[k].type) + " " + f.arguments[k].name;
    }
    out += "\nclass GlobalFunction_" + f.name + " : public s5run_function {\npublic:\n";
    out += "    std::string s5run_function_name() const override { return \"" + f.name + "\"; }\n";
    out += "    virtual ~GlobalFunction_" + f.name + "() {}\n";
    out += "    virtual " + ret + " execute(" + args + ") = 0;\n};\n";
  }

  out += std::string("\n") + CONTEXT_CLASS;
  out += std::string("\n\n") + RUN_RESULT;
  out += std::string("\n\n") + RUN_SCRIPT_DECL;
  out += "\n\n} // namespace " + ns + "\n";
  return out;
}

// -----------------------------------------------------------------------
// Implementation generation (full port from Python)
// -----------------------------------------------------------------------

std::string cpp11_generator::generate_impl(const schema &s) const {
  const std::string &ns = s.cpp11.namespace_;
  std::string out;

  out += "// AUTO-GENERATED. DO NOT EDIT.\n";
  out += "#include \"" + ns + ".h\"\n\n";
  out += "#include <arpa/inet.h>\n#include <netinet/in.h>\n#include <netinet/tcp.h>\n";
  out += "#include <sys/socket.h>\n#include <sys/time.h>\n#include <unistd.h>\n\n";
  out += "#include <cerrno>\n#include <cstdio>\n#include <cstdlib>\n#include <cstring>\n";
  out += "#include <ctime>\n#include <functional>\n#include <stdexcept>\n\n";
  out += "namespace " + ns + " {\n\n";

  out += CODEC;
  out += "\n";

  out += "\n// ---------------------------------------------------------------------------\n";
  out += "// context::impl\n";
  out += "// ---------------------------------------------------------------------------\n\n";

  // context::impl class declaration (schema-dependent)
  out += "class context::impl {\npublic:\n";
  out += "    typedef std::function<void(int32_t, Reader &)> handler_t;\n\n";
  out += "    impl() : m_fd(-1), m_next_this_id(0) {\n";
  out += "        _register_method_handlers();\n    }\n\n";
  out += "    ~impl() { _disconnect(); }\n\n";
  out += "    void set_script(const std::string &script);\n";
  out += "    void add_server(const std::shared_ptr<remote_server_info> &server);\n";
  out += "    void register_type(const std::string &name,\n";
  out += "                       const std::vector<std::string> &methods);\n";
  out += "    void add_object(const std::shared_ptr<s5run_object> &obj);\n";
  out += "    void add_function(const std::shared_ptr<s5run_function> &fn);\n";
  out += "    std::shared_ptr<run_result> run(const std::string &entry,\n";
  out += "                                   const std::vector<value> &args);\n\n";
  out += "private:\n";
  out += "    struct PendingType { std::string name; std::vector<std::string> methods; };\n";
  out += "    struct PendingObject { std::string name; int32_t this_id; std::string type;\n";
  out += "                          std::vector<std::string> methods; };\n";
  out += "    struct PendingFunction { std::string name; };\n\n";
  for (const auto &f : s.functions)
    out += "    void _cb_" + f.name + "(int32_t cb_id, Reader &r);\n";
  for (const auto &c : s.classes)
    for (const auto &m : c.methods)
      out += "    void _cbm_" + c.name + "_" + m.name + "(int32_t cb_id, Reader &r);\n";
  for (const auto &c : s.classes)
    out += "    int32_t _register_instance_" + c.name + "(const std::shared_ptr<" + c.name + "> &obj);\n";
  out += "    void _register_method_handlers();\n";
  out += "    void _connect(); void _disconnect(); void _flush_pending();\n";
  out += "    static void _read_hello(int fd);\n";
  out += "    static bool _send_message_fd(int fd, uint8_t msg_type, const std::vector<uint8_t> &body);\n";
  out += "    static bool _read_message_fd(int fd, uint8_t &msg_type, std::vector<uint8_t> &body);\n";
  out += "    static bool _read_exact_fd(int fd, void *buf, size_t n);\n";
  out += "    static bool _write_all_fd(int fd, const void *buf, size_t n);\n";
  out += "    bool _send_message(uint8_t msg_type, const std::vector<uint8_t> &body);\n";
  out += "    bool _read_message(uint8_t &msg_type, std::vector<uint8_t> &body);\n";
  out += "    bool _read_exact(void *buf, size_t n);\n";
  out += "    bool _write_all(const void *buf, size_t n);\n";
  out += "    void _expect_register_resp(const std::string &what);\n";
  out += "    void _dispatch_callback(std::vector<uint8_t> &body);\n";
  out += "    static int _parse_error_line(const std::string &error);\n";
  out += "    static long _probe_server_load(const std::shared_ptr<remote_server_info> &server);\n\n";
  out += "    std::string m_script;\n";
  out += "    std::vector<std::shared_ptr<remote_server_info>> m_servers;\n";
  out += "    int m_fd;\n    int32_t m_next_this_id;\n";
  out += "    std::vector<PendingType> m_pending_types;\n";
  out += "    std::vector<PendingObject> m_pending_objects;\n";
  out += "    std::vector<PendingFunction> m_pending_functions;\n";
  out += "    std::map<std::string, handler_t> m_handlers;\n";
  out += "    typedef std::function<void(int32_t, const std::shared_ptr<s5run_object> &)> registrar_t;\n";
  out += "    std::map<std::string, registrar_t> m_instance_registrars;\n";
  out += "    typedef std::function<void(const std::shared_ptr<s5run_function> &)> fn_registrar_t;\n";
  out += "    std::map<std::string, fn_registrar_t> m_function_registrars;\n";
  for (const auto &f : s.functions)
    out += "    std::shared_ptr<GlobalFunction_" + f.name + "> m_fn_" + f.name + ";\n";
  for (const auto &c : s.classes)
    out += "    std::map<int32_t, std::shared_ptr<" + c.name + ">> m_instances_" + c.name + ";\n";
  out += "};\n\n";

  // Static impl methods
  out += CONNECT;
  out += "\n";

  // add_object
  out += R"GEN(
void context::impl::add_object(const std::shared_ptr<s5run_object> &obj) {
    const std::string class_name = obj->s5run_class_name();
    auto rit = m_instance_registrars.find(class_name);
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
)GEN";

  // add_function
  out += "\nvoid context::impl::add_function(const std::shared_ptr<s5run_function> &fn) {\n";
  out += "    const std::string name = fn->s5run_function_name();\n";
  out += "    auto rit = m_function_registrars.find(name);\n";
  out += "    if (rit == m_function_registrars.end())\n";
  out += "        throw std::runtime_error(\"add_function: unknown function: \" + name);\n";
  out += "    rit->second(fn);\n";
  out += "    if (m_fd >= 0) {\n        Writer w;\n        w.put_string(name);\n";
  out += "        _send_message(M_REGISTER_FUNC, w.data());\n";
  out += "        _expect_register_resp(\"register_function(\" + name + \")\");\n";
  out += "        return;\n    }\n";
  out += "    PendingFunction pf;\n    pf.name = name;\n";
  out += "    m_pending_functions.push_back(pf);\n}\n\n";

  // Function callbacks (schema-dependent)
  for (const auto &f : s.functions) {
    bool conv = has_convert(f.arguments);
    out += "\nvoid context::impl::_cb_" + f.name + "(int32_t cb_id, Reader &r) {\n";
    out += "    uint32_t n = r.get_u32();\n";
    if (!conv)
      out += "    if (n != " + std::to_string(f.arguments.size()) + ") {\n"
             "        throw std::runtime_error(\"" + f.name + ": wrong argument count\");\n    }\n";
    emit_decode_args(out, f.arguments, f.name);
    std::string call_args;
    for (size_t k = 0; k < f.arguments.size(); ++k) {
      if (k > 0) call_args += ", ";
      call_args += "a_" + f.arguments[k].name;
    }
    if (is_void(f.return_type)) {
      out += "    m_fn_" + f.name + "->execute(" + call_args + ");\n";
    } else {
      out += "    " + cpp11_shared(f.return_type) + " res = m_fn_" + f.name + "->execute(" + call_args + ");\n";
    }
    out += "    Writer w; w.put_i32(cb_id); w.put_u8(1);\n";
    emit_encode_result(out, f.return_type);
    out += "    _send_message(M_CALLBACK_RESP, w.data());\n}\n";
  }

  // Method callbacks (schema-dependent)
  for (const auto &c : s.classes) {
    for (const auto &m : c.methods) {
      bool conv = has_convert(m.arguments);
      out += "\nvoid context::impl::_cbm_" + c.name + "_" + m.name + "(int32_t cb_id, Reader &r) {\n";
      if (conv) {
        out += "    uint32_t n = r.get_u32();\n";
        out += "    if (n < 1) { throw std::runtime_error(\"" + c.name + "." + m.name +
               ": expected this_id\"); }\n";
        out += "    n -= 1;\n";
      } else {
        out += "    uint32_t n = r.get_u32();\n";
        out += "    if (n != " + std::to_string(1 + m.arguments.size()) + ") { throw std::runtime_error(\"" +
               c.name + "." + m.name + ": wrong argument count\"); }\n";
      }
      out += "    int32_t this_id = (int32_t)read_int(r);\n";
      out += "    auto it = m_instances_" + c.name + ".find(this_id);\n";
      out += "    if (it == m_instances_" + c.name + ".end()) { throw std::runtime_error(\"" +
             c.name + ": unknown this_id\"); }\n";
      emit_decode_args(out, m.arguments, c.name + "." + m.name);
      std::string call_args;
      for (size_t k = 0; k < m.arguments.size(); ++k) {
        if (k > 0) call_args += ", ";
        call_args += "a_" + m.arguments[k].name;
      }
      if (is_void(m.return_type)) {
        out += "    it->second->" + m.name + "(" + call_args + ");\n";
      } else {
        out += "    " + cpp11_shared(m.return_type) + " res = it->second->" + m.name + "(" + call_args + ");\n";
      }
      out += "    Writer w; w.put_i32(cb_id); w.put_u8(1);\n";
      emit_encode_result(out, m.return_type);
      out += "    _send_message(M_CALLBACK_RESP, w.data());\n}\n";
    }
  }

  // Register instance methods
  for (const auto &c : s.classes) {
    out += "\nint32_t context::impl::_register_instance_" + c.name +
           "(const std::shared_ptr<" + c.name + "> &obj) {\n";
    out += "    int32_t id = ++m_next_this_id;\n";
    out += "    m_instances_" + c.name + "[id] = obj;\n    return id;\n}\n";
  }

  // _register_method_handlers
  out += "\nvoid context::impl::_register_method_handlers() {\n";
  for (const auto &c : s.classes) {
    out += "    m_instance_registrars[\"" + c.name + "\"] = [this](int32_t id, const std::shared_ptr<s5run_object> &obj) {\n";
    out += "        m_instances_" + c.name + "[id] = std::static_pointer_cast<" + c.name + ">(obj);\n";
    out += "    };\n";
    for (const auto &m : c.methods) {
      out += "    m_handlers[\"" + m.name + "\"] = [this](int32_t cb_id, Reader &r) {\n";
      out += "        _cbm_" + c.name + "_" + m.name + "(cb_id, r);\n";
      out += "    };\n";
    }
  }
  for (const auto &f : s.functions) {
    out += "    m_function_registrars[\"" + f.name + "\"] = [this](const std::shared_ptr<s5run_function> &fn) {\n";
    out += "        m_fn_" + f.name + " = std::static_pointer_cast<GlobalFunction_" + f.name + ">(fn);\n";
    out += "        m_handlers[\"" + f.name + "\"] = [this](int32_t cb_id, Reader &r) {\n";
    out += "            _cb_" + f.name + "(cb_id, r);\n";
    out += "        };\n";
    out += "    };\n";
  }
  out += "}\n";

  // Flush + run + context methods
  out += std::string("\n") + FLUSH;
  out += std::string("\n") + RUN_BLOCK;
  out += std::string("\n") + CONTEXT_METHODS;

  return out;
}

std::vector<std::string> cpp11_generator::generate(const schema &s,
                                                    const std::string &output_dir) {
  const std::string &ns = s.cpp11.namespace_;
  std::vector<std::string> written;
  ::mkdir(output_dir.c_str(), 0755);

  std::string header_path = output_dir + "/" + ns + ".h";
  { std::ofstream f(header_path.c_str()); f << generate_header(s); }
  written.push_back(header_path);

  std::string impl_path = output_dir + "/" + ns + ".cpp";
  { std::ofstream f(impl_path.c_str()); f << generate_impl(s); }
  written.push_back(impl_path);

  return written;
}

std::shared_ptr<IGenerator> make_cpp11_generator() {
  return std::make_shared<cpp11_generator>();
}

} // namespace generators
} // namespace s5run
