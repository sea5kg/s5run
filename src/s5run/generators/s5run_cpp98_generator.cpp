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

#include "s5run_cpp98_generator.h"

#include "nlohmann/json.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sys/stat.h>

namespace s5run {
namespace generators {

// -----------------------------------------------------------------------
// Type helpers
// -----------------------------------------------------------------------

static std::string cpp_type(const std::string &t) {
  if (t == "bool") return "bool";
  if (t == "int" || t == "uint") return "i64";
  if (t == "float" || t == "double") return "double";
  if (t == "string") return "std::string";
  if (t == "byte") return "u8";
  if (t.substr(0, 6) == "array<")
    return "std::vector<" + cpp_type(t.substr(6, t.size() - 7)) + ">";
  return t + " *";
}

static std::string param_type(const std::string &t) {
  std::string ct = cpp_type(t);
  if (ct.substr(0, 5) == "std::") return "const " + ct + " &";
  return ct;
}

static bool is_void(const std::string &t) { return t == "void"; }
static bool has_convert(const std::vector<schema_argument> &args) {
  return args.size() == 1 && !args[0].convert_from.empty();
}
static bool is_ref(const std::string &t) {
  return t != "bool" && t != "int" && t != "uint" && t != "float" && t != "double" &&
         t != "string" && t != "byte" && t != "void" && t.substr(0, 6) != "array<";
}
static bool is_array_of_ref(const std::string &t) {
  if (t.substr(0, 6) != "array<") return false;
  return is_ref(t.substr(6, t.size() - 7));
}
static std::string array_inner(const std::string &t) {
  return t.substr(6, t.size() - 7);
}
static std::string array_suffix(const std::string &t) {
  if (t.substr(0, 6) == "array<") {
    std::string inner = array_inner(t);
    if (inner == "bool") return "array__bool";
    if (inner == "int" || inner == "uint") return "array__int";
    if (inner == "float" || inner == "double") return "array__double";
    if (inner == "string") return "array__string";
    if (inner == "byte") return "array__byte";
    return "array__" + array_suffix(inner);
  }
  return t;
}

// -----------------------------------------------------------------------
// Static template blocks (raw string literals)
// -----------------------------------------------------------------------

static const char *PLATFORM_LAYER = R"CODE(namespace {

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
)CODE";

static const char *CODEC_TEXT = R"CODE(namespace {

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
)CODE";

// (value codec, value impl, run_result impl, remote_server_info impl are emitted
//  in generate_impl since they reference types from the anonymous namespace)

// -----------------------------------------------------------------------
// Header generation
// -----------------------------------------------------------------------

std::string cpp98_generator::generate_header(const schema &s) const {
  const std::string &ns = s.cpp98.namespace_;
  std::string out;

  out += "// AUTO-GENERATED. DO NOT EDIT.\n";
  out += "// C++98 legacy client, portable across Windows (WinSock2) and Linux.\n";
  out += "#pragma once\n\n#include <map>\n#include <string>\n#include <vector>\n\n";
  out += "namespace " + ns + " {\n\n";
  out += "typedef unsigned char u8;\ntypedef unsigned short u16;\ntypedef unsigned int u32;\n";
  out += "#ifdef _MSC_VER\ntypedef __int64 i64;\ntypedef unsigned __int64 u64;\n#else\n";
  out += "typedef long long i64;\ntypedef unsigned long long u64;\n#endif\n";
  out += "typedef signed char i8;\ntypedef short i16;\ntypedef int i32;\n\n";
  out += "class context;\nclass run_result;\ncontext * make_context();\n\n";
  out += "class value {\npublic:\n";
  out += "    enum kind { K_NULL, K_BOOL, K_INT, K_DOUBLE, K_STRING, K_ARRAY, K_OBJECT, K_REF };\n";
  out += "    value(); value(bool b); value(i64 i); value(double d); value(const char *s);\n";
  out += "    value(const std::string &s); value(const std::vector<value> &a);\n";
  out += "    value(const std::map<std::string, value> &o);\n";
  out += "    value(const value &other); value &operator=(const value &other); ~value();\n";
  out += "    static value make_ref(int this_id, const std::string &type);\n";
  out += "    kind get_kind() const; bool is_null() const; bool as_bool() const;\n";
  out += "    i64 as_int() const; double as_double() const;\n";
  out += "    const std::string &as_string() const;\n";
  out += "    const std::vector<value> &as_array() const;\n";
  out += "    const std::map<std::string, value> &as_object() const;\n";
  out += "    int ref_this_id() const; const std::string &ref_type() const;\n";
  out += "private:\n    void destroy(); void copy_from(const value &other);\n";
  out += "    kind m_kind; bool m_b; i64 m_i; double m_d;\n";
  out += "    std::string m_s; std::vector<value> m_a; std::map<std::string, value> m_o;\n";
  out += "};\n\n";

  out += "class remote_server_info {\npublic:\n    remote_server_info();\n";
  out += "    void set_address(const std::string &address);\n";
  out += "    const std::string &get_address() const;\n";
  out += "    void set_api_key(const std::string &api_key);\n";
  out += "    const std::string &get_api_key() const;\n";
  out += "    const std::string &get_host() const; int get_port() const;\n";
  out += "private:\n";
  out += "    std::string m_address; std::string m_api_key; std::string m_host; int m_port;\n";
  out += "};\n\n";

  out += "class s5run_object {\npublic:\n    virtual ~s5run_object() {}\n";
  out += "    virtual std::string s5run_class_name() const = 0;\n";
  out += "    virtual std::vector<std::string> s5run_method_names() const = 0;\n};\n\n";
  out += "class s5run_function {\npublic:\n    virtual ~s5run_function() {}\n";
  out += "    virtual std::string s5run_function_name() const = 0;\n};\n\n";

  for (size_t i = 0; i < s.classes.size(); ++i) {
    const schema_class &c = s.classes[i];
    out += "class " + c.name + " : public s5run_object {\npublic:\n";
    out += "    virtual ~" + c.name + "() {}\n";
    out += "    virtual std::string s5run_class_name() const { return \"" + c.name + "\"; }\n";
    out += "    virtual std::vector<std::string> s5run_method_names() const {\n";
    out += "        std::vector<std::string> out;\n";
    for (size_t j = 0; j < c.methods.size(); ++j)
      out += "        out.push_back(\"" + c.methods[j].name + "\");\n";
    out += "        return out;\n    }\n";
    for (size_t j = 0; j < c.methods.size(); ++j) {
      const schema_method &m = c.methods[j];
      std::string ret = is_void(m.return_type) ? "void" : cpp_type(m.return_type);
      std::string args;
      for (size_t k = 0; k < m.arguments.size(); ++k) {
        if (k > 0) args += ", ";
        args += param_type(m.arguments[k].type) + " " + m.arguments[k].name;
      }
      out += "    virtual " + ret + " " + m.name + "(" + args + ") = 0;\n";
    }
    out += "};\n\n";
  }

  for (size_t i = 0; i < s.functions.size(); ++i) {
    const schema_function &f = s.functions[i];
    std::string ret = is_void(f.return_type) ? "void" : cpp_type(f.return_type);
    std::string args;
    for (size_t k = 0; k < f.arguments.size(); ++k) {
      if (k > 0) args += ", ";
      args += param_type(f.arguments[k].type) + " " + f.arguments[k].name;
    }
    out += "class GlobalFunction_" + f.name + " : public s5run_function {\npublic:\n";
    out += "    virtual std::string s5run_function_name() const { return \"" + f.name + "\"; }\n";
    out += "    virtual ~GlobalFunction_" + f.name + "() {}\n";
    out += "    virtual " + ret + " execute(" + args + ") = 0;\n};\n\n";
  }

  out += "class context {\npublic:\n    context(); ~context();\n";
  out += "    static const char *client_version() { return \"" + s.client_version + "\"; }\n";
  out += "    void set_script(const std::string &script);\n";
  out += "    void add_server(remote_server_info *server);\n";
  out += "    void add_class(s5run_object *sample);\n";
  out += "    void add_object(s5run_object *obj);\n";
  out += "    void add_function(s5run_function *fn);\n";
  out += "    class context_impl;\nprivate:\n";
  out += "    friend run_result run_script(context *ctx, const std::string &entry,\n";
  out += "                                 const std::vector<value> &args);\n";
  out += "    context_impl *m_impl;\n};\n\n";

  out += "class run_result {\npublic:\n    run_result();\n";
  out += "    bool has_error() const; const std::string &error() const;\n";
  out += "    int error_line() const; i64 duration() const; const value &get_value() const;\n";
  out += "private:\n    friend class context::context_impl;\n";
  out += "    friend run_result run_script(context *ctx, const std::string &entry,\n";
  out += "                                 const std::vector<value> &args);\n";
  out += "    bool m_has_error; std::string m_error; int m_error_line; i64 m_duration_ms;\n";
  out += "    value m_value;\n};\n\n";

  out += "run_result run_script(context *ctx, const std::string &entry, ";
  out += "const std::vector<value> &args = std::vector<value>());\n\n";
  out += "} // namespace " + ns + "\n";
  return out;
}

// -----------------------------------------------------------------------
// Implementation generation
// -----------------------------------------------------------------------

// Helper: generate callback body for a function or method argument decode
static void emit_decode_args(std::string &out, const std::vector<schema_argument> &args,
                              const std::string &owner_name) {
  if (has_convert(args)) {
    out += "    std::vector<std::string> a_" + args[0].name + ";\n";
    out += "    a_" + args[0].name + ".reserve(n);\n";
    out += "    for (u32 i = 0; i < n; ++i)\n";
    out += "        a_" + args[0].name + ".push_back(_value_to_string(decode_value(r)));\n";
    return;
  }
  for (size_t k = 0; k < args.size(); ++k) {
    const schema_argument &a = args[k];
    if (is_array_of_ref(a.type)) {
      std::string cls = array_inner(a.type);
      out += "    if (r.get_u8() != T_ARRAY) throw std::runtime_error(\"wire: expected array\");\n";
      out += "    u32 cnt_" + a.name + " = r.get_u32();\n";
      out += "    std::vector<" + cls + " *> a_" + a.name + ";\n";
      out += "    a_" + a.name + ".reserve(cnt_" + a.name + ");\n";
      out += "    for (u32 i = 0; i < cnt_" + a.name + "; ++i) {\n";
      out += "        i32 a_ref_id = read_ref(r);\n";
      out += "        std::map<i32, " + cls + " *>::const_iterator a_it = m_instances_" + cls + ".find(a_ref_id);\n";
      out += "        if (a_it == m_instances_" + cls + ".end()) throw std::runtime_error(\"" +
                  owner_name + ": unknown object reference\");\n";
      out += "        a_" + a.name + ".push_back(a_it->second);\n";
      out += "    }\n";
    } else if (is_ref(a.type)) {
      out += "    i32 a_ref_id_" + a.name + " = read_ref(r);\n";
      out += "    std::map<i32, " + a.type + " *>::const_iterator a_it_" + a.name + " = m_instances_" + a.type + ".find(a_ref_id_" + a.name + ");\n";
      out += "    if (a_it_" + a.name + " == m_instances_" + a.type + ".end()) throw std::runtime_error(\"" +
                  owner_name + ": unknown object reference\");\n";
      out += "    " + a.type + " *a_" + a.name + " = a_it_" + a.name + "->second;\n";
    } else {
      std::string ct = cpp_type(a.type);
      if (a.type == "bool") {
        out += "    bool a_" + a.name + " = read_bool(r);\n";
      } else if (a.type == "int" || a.type == "uint") {
        out += "    i64 a_" + a.name + " = read_int(r);\n";
      } else if (a.type == "float" || a.type == "double") {
        out += "    double a_" + a.name + " = read_double(r);\n";
      } else if (a.type == "string") {
        out += "    std::string a_" + a.name + " = read_string(r);\n";
      } else if (a.type == "byte") {
        out += "    u8 a_" + a.name + " = read_byte(r);\n";
      } else if (a.type.substr(0, 6) == "array<") {
        std::string inner = array_inner(a.type);
        std::string reader_fn;
        if (inner == "string") reader_fn = "read_string(r)";
        else if (inner == "bool") reader_fn = "read_bool(r)";
        else if (inner == "byte") reader_fn = "read_byte(r)";
        else if (inner == "int" || inner == "uint") reader_fn = "read_int(r)";
        else if (inner == "float" || inner == "double") reader_fn = "read_double(r)";
        else reader_fn = "read_" + array_suffix(a.type) + "(r)";
        out += "    std::vector<" + cpp_type(inner) + "> a_" + a.name + " = read_" + array_suffix(a.type) + "(r);\n";
      }
    }
  }
}

// Helper: generate result encoding
static void emit_encode_result(std::string &out, const std::string &return_type,
                                 const std::string &owner_name) {
  if (is_void(return_type)) {
    out += "    w.put_u8(T_UNDEFINED);\n";
    return;
  }
  if (is_array_of_ref(return_type)) {
    std::string cls = array_inner(return_type);
    out += "    w.put_u8(T_ARRAY);\n";
    out += "    w.put_u32((u32)res.size());\n";
    out += "    for (std::size_t i = 0; i < res.size(); ++i) {\n";
    out += "        i32 obj_id = register_instance_" + cls + "(res[i]);\n";
    out += "        w.put_u8(T_REF); w.put_i32(obj_id); w.put_string(\"" + cls + "\");\n";
    out += "    }\n";
  } else if (is_ref(return_type)) {
    out += "    i32 obj_id = register_instance_" + return_type + "(res);\n";
    out += "    w.put_u8(T_REF); w.put_i32(obj_id); w.put_string(\"" + return_type + "\");\n";
  } else if (return_type == "bool") {
    out += "    w.put_u8(T_BOOL); w.put_u8(res ? 1 : 0);\n";
  } else if (return_type == "int" || return_type == "uint") {
    out += "    write_int(w, res);\n";
  } else if (return_type == "float" || return_type == "double") {
    out += "    w.put_u8(T_FLOAT64); w.put_f64(res);\n";
  } else if (return_type == "string") {
    out += "    w.put_u8(T_STRING); w.put_string(res);\n";
  } else if (return_type == "byte") {
    out += "    w.put_u8(T_UINT8); w.put_u8(res);\n";
  } else if (return_type.substr(0, 6) == "array<") {
    std::string inner = array_inner(return_type);
    out += "    w.put_u8(T_ARRAY); w.put_u32((u32)res.size());\n";
    out += "    for (std::size_t i = 0; i < res.size(); ++i) {\n";
    if (inner == "string") { out += "        w.put_u8(T_STRING); w.put_string(res[i]);\n"; }
    else if (inner == "bool") { out += "        w.put_u8(T_BOOL); w.put_u8(res[i] ? 1 : 0);\n"; }
    else if (inner == "int" || inner == "uint") { out += "        write_int(w, res[i]);\n"; }
    else if (inner == "float" || inner == "double") { out += "        w.put_u8(T_FLOAT64); w.put_f64(res[i]);\n"; }
    else if (inner == "byte") { out += "        w.put_u8(T_UINT8); w.put_u8(res[i]);\n"; }
    out += "    }\n";
  }
}

// Helper: generate a function callback
static std::string gen_fn_callback(const schema_function &f) {
  std::string out;
  bool conv = has_convert(f.arguments);
  out += "void context::context_impl::cb_" + f.name + "(i32 cb_id, Reader &r) {\n";
  if (conv) {
    out += "    u32 n = r.get_u32();\n";
  } else {
    out += "    u32 n = r.get_u32();\n";
    out += "    if (n != " + std::to_string(f.arguments.size()) + ") throw std::runtime_error(\"" +
                f.name + ": wrong argument count\");\n";
  }
  emit_decode_args(out, f.arguments, f.name);
  std::string call_args;
  for (size_t k = 0; k < f.arguments.size(); ++k) {
    if (k > 0) call_args += ", ";
    call_args += "a_" + f.arguments[k].name;
  }
  if (is_void(f.return_type)) {
    out += "    m_fn_" + f.name + "->execute(" + call_args + ");\n";
  } else {
    out += "    " + cpp_type(f.return_type) + " res = m_fn_" + f.name + "->execute(" + call_args + ");\n";
  }
  out += "    Writer w; w.put_i32(cb_id); w.put_u8(1);\n";
  emit_encode_result(out, f.return_type, f.name);
  out += "    send_message(M_CALLBACK_RESP, w.data());\n}\n\n";
  return out;
}

// Helper: generate a class method callback
static std::string gen_m_callback(const schema_class &c, const schema_method &m) {
  std::string out;
  bool conv = has_convert(m.arguments);
  out += "void context::context_impl::cbm_" + c.name + "_" + m.name + "(i32 cb_id, Reader &r) {\n";
  if (conv) {
    out += "    u32 n = r.get_u32();\n";
    out += "    if (n < 1) throw std::runtime_error(\"" + c.name + "." + m.name + ": expected this_id\");\n";
    out += "    n -= 1;\n";
  } else {
    out += "    u32 n = r.get_u32();\n";
    out += "    if (n != " + std::to_string(1 + m.arguments.size()) + ") throw std::runtime_error(\"" +
                c.name + "." + m.name + ": wrong argument count\");\n";
  }
  out += "    i32 this_id = (i32)read_int(r);\n";
  out += "    std::map<i32, " + c.name + " *>::const_iterator it = m_instances_" + c.name + ".find(this_id);\n";
  out += "    if (it == m_instances_" + c.name + ".end()) throw std::runtime_error(\"" + c.name + ": unknown this_id\");\n";
  emit_decode_args(out, m.arguments, c.name + "." + m.name);
  std::string call_args;
  for (size_t k = 0; k < m.arguments.size(); ++k) {
    if (k > 0) call_args += ", ";
    call_args += "a_" + m.arguments[k].name;
  }
  if (is_void(m.return_type)) {
    out += "    it->second->" + m.name + "(" + call_args + ");\n";
  } else {
    out += "    " + cpp_type(m.return_type) + " res = it->second->" + m.name + "(" + call_args + ");\n";
  }
  out += "    Writer w; w.put_i32(cb_id); w.put_u8(1);\n";
  emit_encode_result(out, m.return_type, c.name + "." + m.name);
  out += "    send_message(M_CALLBACK_RESP, w.data());\n}\n\n";
  return out;
}

std::string cpp98_generator::generate_impl(const schema &s) const {
  const std::string &ns = s.cpp98.namespace_;
  std::string out;

  // File header + includes + namespace
  out += "// AUTO-GENERATED. DO NOT EDIT.\n";
  out += "#include \"" + ns + ".h\"\n\n";
  out += "#ifdef _WIN32\n#ifndef WIN32_LEAN_AND_MEAN\n#define WIN32_LEAN_AND_MEAN\n#endif\n";
  out += "#include <winsock2.h>\n#include <ws2tcpip.h>\n#pragma comment(lib, \"ws2_32.lib\")\n";
  out += "#else\n#include <arpa/inet.h>\n#include <netinet/in.h>\n#include <netinet/tcp.h>\n";
  out += "#include <sys/socket.h>\n#include <sys/time.h>\n#include <unistd.h>\n#endif\n\n";
  out += "#include <cerrno>\n#include <cstdio>\n#include <cstring>\n#include <ctime>\n#include <stdexcept>\n\n";
  out += "namespace " + ns + " {\n\n";

  // Platform layer
  out += PLATFORM_LAYER;
  out += "\n";

  // Codec
  out += CODEC_TEXT;
  out += "\n";

  // Primitive read/write helpers (based on usage)
  {
    std::string prims_used;
    for (auto &f : s.functions) {
      for (auto &a : f.arguments) prims_used += a.type + " ";
      if (!is_void(f.return_type)) prims_used += f.return_type + " ";
    }
    for (auto &c : s.classes)
      for (auto &m : c.methods) {
        for (auto &a : m.arguments) prims_used += a.type + " ";
        if (!is_void(m.return_type)) prims_used += m.return_type + " ";
      }

    if (prims_used.find("bool") != std::string::npos) {
      out += "static bool read_bool(Reader &r) {\n";
      out += "    if (r.get_u8() != T_BOOL) throw std::runtime_error(\"wire: expected a bool\");\n";
      out += "    return r.get_u8() != 0;\n}\n\n";
      out += "static void write_bool(Writer &w, bool v) {\n";
      out += "    w.put_u8(T_BOOL); w.put_u8(v ? 1 : 0);\n}\n\n";
    }
    if (prims_used.find("double") != std::string::npos || prims_used.find("float") != std::string::npos) {
      out += "static double read_double(Reader &r) {\n";
      out += "    if (r.get_u8() != T_FLOAT64) throw std::runtime_error(\"wire: expected a double\");\n";
      out += "    return r.get_f64();\n}\n\n";
      out += "static void write_double(Writer &w, double v) {\n";
      out += "    w.put_u8(T_FLOAT64); w.put_f64(v);\n}\n\n";
    }
    if (prims_used.find("string") != std::string::npos) {
      out += "static std::string read_string(Reader &r) {\n";
      out += "    if (r.get_u8() != T_STRING) throw std::runtime_error(\"wire: expected a string\");\n";
      out += "    return r.get_string();\n}\n\n";
      out += "static void write_string(Writer &w, const std::string &v) {\n";
      out += "    w.put_u8(T_STRING); w.put_string(v);\n}\n\n";
    }
    if (prims_used.find("byte") != std::string::npos) {
      out += "static u8 read_byte(Reader &r) {\n";
      out += "    if (r.get_u8() != T_UINT8) throw std::runtime_error(\"wire: expected a byte\");\n";
      out += "    return r.get_u8();\n}\n\n";
      out += "static void write_byte(Writer &w, u8 v) {\n";
      out += "    w.put_u8(T_UINT8); w.put_u8(v);\n}\n\n";
    }
  }

  // Value codec (encode_value, decode_value, _value_to_string)
  out += R"CODE(
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
)CODE";

  // Value implementation
  out += R"CODE(
value::value() : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0) {}
value::value(bool b) : m_kind(K_BOOL), m_b(b), m_i(0), m_d(0.0) {}
value::value(i64 i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
value::value(double d) : m_kind(K_DOUBLE), m_b(false), m_i(0), m_d(d) {}
value::value(const char *s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
value::value(const std::string &s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
value::value(const std::vector<value> &a) : m_kind(K_ARRAY), m_b(false), m_i(0), m_d(0.0), m_a(a) {}
value::value(const std::map<std::string, value> &o) : m_kind(K_OBJECT), m_b(false), m_i(0), m_d(0.0), m_o(o) {}
value::value(const value &other) : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0) { copy_from(other); }
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
const std::vector<value> &value::as_array() const { return m_a; }
const std::map<std::string, value> &value::as_object() const { return m_o; }
int value::ref_this_id() const { return (int)m_i; }
const std::string &value::ref_type() const { return m_s; }
void value::destroy() { m_kind = K_NULL; }
void value::copy_from(const value &other) {
    m_kind = other.m_kind; m_b = other.m_b; m_i = other.m_i; m_d = other.m_d;
    m_s = other.m_s; m_a = other.m_a; m_o = other.m_o;
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
)CODE";

  // ---------------------------------------------------------------------
  // context_impl (schema-dependent)
  // ---------------------------------------------------------------------
  out += "\n// ---------------------------------------------------------------------------\n";
  out += "// context_impl\n";
  out += "// ---------------------------------------------------------------------------\n\n";
  out += "class context::context_impl {\npublic:\n";
  out += "    context_impl() : m_sock(S5RUN_BAD_SOCKET), m_next_this_id(0) {\n";
  out += "        s5run_sockets_init();\n    }\n";
  out += "    ~context_impl() { disconnect(); s5run_sockets_shutdown(); }\n\n";
  out += "    void set_script(const std::string &script);\n";
  out += "    void add_server(remote_server_info *server);\n";
  out += "    void register_type(const std::string &name, const std::vector<std::string> &methods);\n";
  out += "    void add_object(s5run_object *obj);\n";
  out += "    void add_function(s5run_function *fn);\n";
  out += "    run_result run(const std::string &entry, const std::vector<value> &args);\n\n";
  out += "private:\n";
  out += "    struct PendingType { std::string name; std::vector<std::string> methods; };\n";
  out += "    struct PendingObject { std::string name; i32 this_id; std::vector<std::string> methods; };\n\n";

  // Callback declarations
  for (auto &f : s.functions)
    out += "    void cb_" + f.name + "(i32 cb_id, Reader &r);\n";
  for (auto &c : s.classes)
    for (auto &m : c.methods)
      out += "    void cbm_" + c.name + "_" + m.name + "(i32 cb_id, Reader &r);\n";

  // Instance registration declarations
  for (auto &c : s.classes)
    out += "    i32 register_instance_" + c.name + "(" + c.name + " *obj);\n";

  out += "    void store_instance(const std::string &cls, i32 id, s5run_object *obj);\n";
  out += "    void dispatch_callback(const std::vector<u8> &body);\n";
  out += "    void connect(); void disconnect(); void flush_pending();\n";
  out += "    static void read_hello(sock_t s);\n";
  out += "    static bool send_frame(sock_t s, u8 msg_type, const std::vector<u8> &body);\n";
  out += "    static bool read_frame(sock_t s, u8 &msg_type, std::vector<u8> &body);\n";
  out += "    void send_message(u8 msg_type, const std::vector<u8> &body);\n";
  out += "    void expect_register_resp(const std::string &what);\n";
  out += "    static i64 probe_server_load(remote_server_info *server);\n";
  out += "    static int parse_error_line(const std::string &error);\n\n";

  // Member fields
  out += "    std::string m_script;\n";
  out += "    std::vector<remote_server_info *> m_servers;\n";
  out += "    std::vector<PendingType> m_pending_types;\n";
  out += "    std::vector<PendingObject> m_pending_objects;\n";
  out += "    std::vector<std::string> m_pending_functions;\n";
  out += "    sock_t m_sock;\n    i32 m_next_this_id;\n";
  for (auto &f : s.functions)
    out += "    GlobalFunction_" + f.name + " *m_fn_" + f.name + ";\n";
  for (auto &c : s.classes)
    out += "    std::map<i32, " + c.name + " *> m_instances_" + c.name + ";\n";
  out += "};\n\n";

  // Static methods of context_impl
  out += R"CODE(
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
)CODE";

  // add_function: if/else chain by name
  for (auto &f : s.functions) {
    out += "    if (fn->s5run_function_name() == \"" + f.name + "\") {\n";
    out += "        m_fn_" + f.name + " = (GlobalFunction_" + f.name + " *)fn;\n    }\n";
  }
  out += R"CODE(    if (m_sock != S5RUN_BAD_SOCKET) {
        Writer w;
        w.put_string(fn->s5run_function_name());
        send_message(M_REGISTER_FUNC, w.data());
        expect_register_resp("register_function(" + fn->s5run_function_name() + ")");
        return;
    }
    m_pending_functions.push_back(fn->s5run_function_name());
}

void context::context_impl::store_instance(const std::string &cls, i32 id, s5run_object *obj) {
)CODE";
  for (auto &c : s.classes) {
    out += "    if (cls == \"" + c.name + "\") { m_instances_" + c.name + "[id] = (" + c.name + " *)obj; return; }\n";
  }
  out += R"CODE(    throw std::runtime_error("add_object: unknown class: " + cls);
}
)CODE";

  // Register instance methods
  for (auto &c : s.classes) {
    out += "\ni32 context::context_impl::register_instance_" + c.name + "(" + c.name + " *obj) {\n";
    out += "    i32 id = ++m_next_this_id;\n";
    out += "    m_instances_" + c.name + "[id] = obj;\n    return id;\n}\n";
  }

  // Function callbacks
  for (auto &f : s.functions)
    out += "\n" + gen_fn_callback(f);

  // Class method callbacks
  for (auto &c : s.classes)
    for (auto &m : c.methods)
      out += "\n" + gen_m_callback(c, m);

  // Dispatch
  out += "\nvoid context::context_impl::dispatch_callback(const std::vector<u8> &body) {\n";
  out += "    Reader r(body.empty() ? 0 : &body[0], body.size());\n";
  out += "    i32 cb_id = r.get_i32();\n";
  out += "    std::string method = r.get_string();\n";
  out += "    try {\n";
  for (auto &f : s.functions)
    out += "        if (method == \"" + f.name + "\") { cb_" + f.name + "(cb_id, r); return; }\n";
  for (auto &c : s.classes)
    for (auto &m : c.methods)
      out += "        if (method == \"" + m.name + "\") { cbm_" + c.name + "_" + m.name + "(cb_id, r); return; }\n";
  out += "    } catch (const std::exception &e) {\n";
  out += "        Writer w; w.put_i32(cb_id); w.put_u8(0); w.put_string(e.what());\n";
  out += "        send_message(M_CALLBACK_RESP, w.data());\n        return;\n    }\n";
  out += "    Writer w; w.put_i32(cb_id); w.put_u8(0);\n";
  out += "    w.put_string(\"unknown method: \" + method);\n";
  out += "    send_message(M_CALLBACK_RESP, w.data());\n}\n\n";

  // Connect / disconnect / hello / probe / framing / flush / run
  out += R"CODE(
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
    std::string text(body.empty() ? "" : std::string((const char *)&body[0], body.size()));
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
)CODE";

  // context public methods
  out += "\ncontext::context() : m_impl(new context::context_impl()) {}\n";
  out += "context::~context() { delete m_impl; }\n\n";
  out += "void context::set_script(const std::string &s) { m_impl->set_script(s); }\n";
  out += "void context::add_server(remote_server_info *s) { m_impl->add_server(s); }\n";
  out += "void context::add_class(s5run_object *o) { m_impl->register_type(o->s5run_class_name(), o->s5run_method_names()); }\n";
  out += "void context::add_object(s5run_object *o) { m_impl->add_object(o); }\n";
  out += "void context::add_function(s5run_function *f) { m_impl->add_function(f); }\n\n";
  out += "context * make_context() { return new context(); }\n\n";
  out += "run_result run_script(context *ctx, const std::string &entry, ";
  out += "const std::vector<value> &args) { return ctx->m_impl->run(entry, args); }\n\n";
  out += "} // namespace " + ns + "\n";

  return out;
}

std::vector<std::string> cpp98_generator::generate(const schema &s,
                                                   const std::string &output_dir) {
  const std::string &ns = s.cpp98.namespace_;
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

} // namespace generators
} // namespace s5run
