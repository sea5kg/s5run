#
# MIT License
#
#           ░░░░░░░
#          ░██▓▓▓▓▒
# ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
# ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
#  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
# ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
#  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ▒▒▒▒ ▒▒░ ░▒▒   ▒▒░
#
# Copyright (c) 2026 Evgenii Sopov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Original repository: https://github.com/sea5kg/s5run
#

"""C++98 client generator for legacy toolchains (Windows/Linux portable)."""

from .s5run_typeref import S5RunCliTypeRef, s5run_parse_type

PRIMITIVES = {
    "bool":   "bool",
    "int":    "i64",
    "uint":   "i64",
    "float":  "double",
    "double": "double",
    "string": "std::string",
    "byte":   "u8",
}


def type_to_cpp(ref: S5RunCliTypeRef) -> str:
    if ref.is_prim():
        return PRIMITIVES[ref.name()]
    if ref.is_array():
        return f"std::vector<{type_to_cpp(s5run_parse_type(ref.inner()))}>"
    if ref.is_ref():
        return ref.name() + " *"
    return ref.name()


def _param_type(ref) -> str:
    t = type_to_cpp(ref)
    if t.startswith("std::"):
        return f"const {t} &"
    if t.endswith("*"):
        return t
    return t


def _is_void(fm) -> bool:
    return fm.return_type() == "void"


def _has_convert(fm) -> bool:
    args = fm.arguments()
    return len(args) == 1 and bool(args[0].convert_from())


class _Usage:
    def __init__(self):
        self.prim_reads = []
        self.prim_writes = []
        self.array_reads = []
        self.array_writes = []
        self.ref_reads = False

    def _mark(self, lst, item):
        if item not in lst:
            lst.append(item)

    def need_read(self, type_str, allow_ref=False):
        ref = s5run_parse_type(type_str)
        if ref.is_ref():
            if not allow_ref:
                raise ValueError(
                    "object references in arrays are not supported yet, got: " + type_str)
            self.ref_reads = True
            return
        if ref.is_prim():
            self._mark(self.prim_reads, ref.name())
        elif ref.is_array():
            inner = s5run_parse_type(ref.inner())
            if inner.is_ref():
                if not allow_ref:
                    raise ValueError(
                        "object references in nested arrays are not supported yet, got: " + type_str)
                self.ref_reads = True
                return
            self._mark(self.array_reads, type_str)
            self.need_read(ref.inner())

    def need_write(self, type_str, allow_ref=False):
        ref = s5run_parse_type(type_str)
        if ref.is_ref():
            return
        if ref.is_prim():
            self._mark(self.prim_writes, ref.name())
        elif ref.is_array():
            inner = s5run_parse_type(ref.inner())
            if inner.is_ref():
                return
            self._mark(self.array_writes, type_str)
            self.need_write(ref.inner())

    def add_all(self, fm):
        for a in fm.arguments():
            if not a.convert_from():
                self.need_read(a.type(), allow_ref=True)
        if not _is_void(fm):
            self.need_write(fm.return_type(), allow_ref=True)

    def suffix(self, type_str) -> str:
        ref = s5run_parse_type(type_str)
        if ref.is_prim():
            return ref.name()
        return "array__" + self.suffix(ref.inner())


class _Cpp99Generator:

    def __init__(self, schema):
        self.schema = schema
        self.ns = schema.get_cpp_options("cpp99").namespace()
        self.usage = _Usage()
        for c in schema.classes:
            for m in c.methods():
                self.usage.add_all(m)
        for f in schema.functions:
            self.usage.add_all(f)

    # ------------------------------------------------------------------

    def _read_expr(self, ref) -> str:
        if ref.is_prim():
            name = ref.name()
            if name in ("int", "uint"):
                return "read_int(r)"
            if name == "bool":
                return "read_bool(r)"
            if name in ("float", "double"):
                return "read_double(r)"
            if name == "string":
                return "read_string(r)"
            if name == "byte":
                return "read_byte(r)"
        return f"read_{self.usage.suffix(ref.name())}(r)"

    def _write_stmt(self, ref, expr) -> str:
        if ref.is_prim():
            name = ref.name()
            if name in ("int", "uint"):
                return f"write_int(w, {expr});"
            if name == "bool":
                return f"write_bool(w, {expr});"
            if name in ("float", "double"):
                return f"write_double(w, {expr});"
            if name == "string":
                return f"write_string(w, {expr});"
            if name == "byte":
                return f"write_byte(w, {expr});"
        return f"write_{self.usage.suffix(ref.name())}(w, {expr});"

    def _decode_args(self, fm, prefix="a_") -> list:
        L = []
        if _has_convert(fm):
            a = fm.arguments()[0]
            cpp_t = type_to_cpp(a.type_ref())
            L += [
                f"    {cpp_t} {prefix}{a.name()};",
                f"    {prefix}{a.name()}.reserve(n);",
                "    for (u32 i = 0; i < n; ++i)",
                f"        {prefix}{a.name()}.push_back(_value_to_string(decode_value(r)));",
            ]
            return L
        for a in fm.arguments():
            ref = a.type_ref()
            if ref.is_array() and s5run_parse_type(ref.inner()).is_ref():
                cls = s5run_parse_type(ref.inner()).name()
                err = self._unknown_ref_line(fm.name(), prefix, a.name())
                L += [
                    "    if (r.get_u8() != T_ARRAY)",
                    '        throw std::runtime_error("wire: expected array");',
                    "    u32 cnt_" + a.name() + " = r.get_u32();",
                    f"    std::vector<{cls} *> {prefix}{a.name()};",
                    f"    {prefix}{a.name()}.reserve(cnt_{a.name()});",
                    f"    for (u32 i = 0; i < cnt_{a.name()}; ++i) {{",
                    f"        i32 {prefix}ref_id_{a.name()} = read_ref(r);",
                    f"        std::map<i32, {cls} *>::const_iterator {prefix}it_{a.name()} =",
                    f"            m_instances_{cls}.find({prefix}ref_id_{a.name()});",
                    f"        if ({prefix}it_{a.name()} == m_instances_{cls}.end()) {{",
                    err,
                    "        }",
                    f"        {prefix}{a.name()}.push_back({prefix}it_{a.name()}->second);",
                    "    }",
                ]
            elif ref.is_ref():
                cls = ref.name()
                err = self._unknown_ref_line(fm.name(), prefix, a.name())
                L += [
                    f"    i32 {prefix}ref_id_{a.name()} = read_ref(r);",
                    f"    std::map<i32, {cls} *>::const_iterator {prefix}it_{a.name()} =",
                    f"        m_instances_{cls}.find({prefix}ref_id_{a.name()});",
                    f"    if ({prefix}it_{a.name()} == m_instances_{cls}.end()) {{",
                    err,
                    "    }",
                    f"    {cls} *{prefix}{a.name()} = {prefix}it_{a.name()}->second;",
                ]
            else:
                cpp_t = type_to_cpp(ref)
                L.append(f"    {cpp_t} {prefix}{a.name()} = {self._read_expr(ref)};")
        return L

    @staticmethod
    def _unknown_ref_line(fname, prefix, arg_name):
        return ("            throw std::runtime_error(\"" + fname
                + ": unknown object reference: \""
                + " + num_to_str(" + prefix + "ref_id_" + arg_name + "));")

    def _encode_result(self, fm, expr) -> list:
        if _is_void(fm):
            return ["    w.put_u8(T_UNDEFINED);"]
        ref = fm.return_ref()
        if ref.is_array() and s5run_parse_type(ref.inner()).is_ref():
            cls = s5run_parse_type(ref.inner()).name()
            return [
                "    w.put_u8(T_ARRAY);",
                f"    w.put_u32((u32){expr}.size());",
                f"    for (std::size_t i = 0; i < {expr}.size(); ++i) {{",
                f"        i32 obj_id = register_instance_{cls}({expr}[i]);",
                "        w.put_u8(T_REF);",
                "        w.put_i32(obj_id);",
                '        w.put_string("' + cls + '");',
                "    }",
            ]
        if ref.is_ref():
            cls = ref.name()
            return [
                f"    i32 obj_id = register_instance_{cls}({expr});",
                "    w.put_u8(T_REF);",
                "    w.put_i32(obj_id);",
                '        w.put_string("' + cls + '");',
            ]
        return ["    " + self._write_stmt(ref, expr)]

    # ------------------------------------------------------------------
    # header
    # ------------------------------------------------------------------

    def generate_header(self) -> str:
        s = self.schema
        L = ["// AUTO-GENERATED. DO NOT EDIT.",
             "// C++98 legacy client, portable across Windows (WinSock2) and Linux.",
             "#pragma once",
             "",
             "#include <map>",
             "#include <string>",
             "#include <vector>",
             "",
             f"namespace {self.ns} {{",
             "",
             "typedef unsigned char u8;",
             "typedef unsigned short u16;",
             "typedef unsigned int u32;",
             "#ifdef _MSC_VER",
             "typedef __int64 i64;",
             "typedef unsigned __int64 u64;",
             "#else",
             "typedef long long i64;",
             "typedef unsigned long long u64;",
             "#endif",
             "typedef signed char i8;",
             "typedef short i16;",
             "typedef int i32;",
             "",
             "class context;",
             "class run_result;",
             f"context * make_context();",
             "",
             self._value_class(),
             "",
             self._remote_server_info_class(),
             "",
             self._root_class(),
             ""]
        for c in s.classes:
            L.append(self._base_class(c))
            L.append("")
        for f in s.functions:
            L.append(self._global_function_class(f))
            L.append("")
        L.append(self._context_class())
        L.append("")
        L.append(self._run_result_class())
        L.append("")
        L.append(self._run_script_decl())
        L.append("")
        L.append(f"}} // namespace {self.ns}")
        return "\n".join(L) + "\n"

    def _value_class(self) -> str:
        return """class value {
public:
    enum kind {
        K_NULL, K_BOOL, K_INT, K_DOUBLE, K_STRING, K_ARRAY, K_OBJECT, K_REF
    };

    value();
    value(bool b);
    value(i64 i);
    value(double d);
    value(const char *s);
    value(const std::string &s);
    value(const std::vector<value> &a);
    value(const std::map<std::string, value> &o);
    value(const value &other);
    value &operator=(const value &other);
    ~value();

    static value make_ref(int this_id, const std::string &type);

    kind get_kind() const;
    bool is_null() const;
    bool as_bool() const;
    i64 as_int() const;
    double as_double() const;
    const std::string &as_string() const;
    const std::vector<value> &as_array() const;
    const std::map<std::string, value> &as_object() const;
    int ref_this_id() const;
    const std::string &ref_type() const;

private:
    void destroy();
    void copy_from(const value &other);

    kind m_kind;
    bool m_b;
    i64 m_i;
    double m_d;
    std::string m_s;
    std::vector<value> m_a;
    std::map<std::string, value> m_o;
};"""

    def _run_result_class(self) -> str:
        return """class run_result {
public:
    run_result();
    bool has_error() const;
    const std::string &error() const;
    int error_line() const;
    i64 duration() const; // milliseconds
    const value &get_value() const;

private:
    friend class context::context_impl;
    bool m_has_error;
    std::string m_error;
    int m_error_line;
    i64 m_duration_ms;
    value m_value;
};"""

    def _remote_server_info_class(self) -> str:
        return """// Settings of a remote s5run server (dedovskiy style setters).
class remote_server_info {
public:
    remote_server_info();
    // Address format: s5run://host:port
    void set_address(const std::string &address);
    const std::string &get_address() const;
    void set_api_key(const std::string &api_key);
    const std::string &get_api_key() const;
    // Valid after a successful set_address.
    const std::string &get_host() const;
    int get_port() const;

private:
    std::string m_address;
    std::string m_api_key;
    std::string m_host;
    int m_port;
};"""

    def _root_class(self) -> str:
        return """class s5run_object {
public:
    virtual ~s5run_object() {}
    virtual std::string s5run_class_name() const = 0;
    virtual std::vector<std::string> s5run_method_names() const = 0;
};

class s5run_function {
public:
    virtual ~s5run_function() {}
    virtual std::string s5run_function_name() const = 0;
};"""

    def _base_class(self, c) -> str:
        L = [f"class {c.name()} : public s5run_object {{",
             "public:",
             f"    virtual ~{c.name()}() {{}}",
             '    virtual std::string s5run_class_name() const {{ return "{0}"; }}'.format(c.name()),
             "    virtual std::vector<std::string> s5run_method_names() const {",
             "        std::vector<std::string> out;"]
        for m in c.methods():
            L.append('        out.push_back("{0}");'.format(m.name()))
        L.append("        return out;")
        L.append("    }")
        for m in c.methods():
            ret = "void" if _is_void(m) else type_to_cpp(m.return_ref())
            args = ", ".join(f"{_param_type(a.type_ref())} {a.name()}" for a in m.arguments())
            L.append(f"    virtual {ret} {m.name()}({args}) = 0;")
        L.append("};")
        return "\n".join(L)

    def _global_function_class(self, f) -> str:
        ret = "void" if _is_void(f) else type_to_cpp(f.return_ref())
        args = ", ".join(f"{_param_type(a.type_ref())} {a.name()}" for a in f.arguments())
        return "\n".join([
            f"class GlobalFunction_{f.name()} : public s5run_function {{",
            "public:",
            '    virtual std::string s5run_function_name() const { return "' + f.name() + '"; }',
            f"    virtual ~GlobalFunction_{f.name()}() {{}}",
            f"    virtual {ret} execute({args}) = 0;",
            "};",
        ])

    def _context_class(self) -> str:
        s = self.schema
        L = ["class context {",
             "public:",
             "    context();",
             "    ~context();",
             "",
             '    static const char *client_version() { return "' + self.schema.client_version + '"; }',
             "",
             "    void set_script(const std::string &script);",
             "    void add_server(remote_server_info *server);",
             "    void add_class(s5run_object *sample);",
             "    void add_object(s5run_object *obj);",
             "    void add_function(s5run_function *fn);",
             "",
             "    class context_impl;",
             "",
             "private:",
             "    context(const context &);",
             "    context &operator=(const context &);",
             "    context_impl *m_impl;",
             "    friend run_result run_script(context *ctx, const std::string &entry,",
             "                                 const std::vector<value> &args);",
             "};"]
        return "\n".join(L)

    def _run_script_decl(self) -> str:
        return ("run_result run_script(context *ctx, const std::string &entry, "
                "const std::vector<value> &args = std::vector<value>());")

    # ------------------------------------------------------------------
    # implementation
    # ------------------------------------------------------------------

    def generate_impl(self) -> str:
        L = ["// AUTO-GENERATED. DO NOT EDIT.",
             f'#include "{self.ns}.h"',
             "",
             "#ifdef _WIN32",
             "#  ifndef WIN32_LEAN_AND_MEAN",
             "#    define WIN32_LEAN_AND_MEAN",
             "#  endif",
             "#  include <winsock2.h>",
             "#  include <ws2tcpip.h>",
             "#  pragma comment(lib, \"ws2_32.lib\")",
             "#else",
             "#  include <arpa/inet.h>",
             "#  include <netinet/in.h>",
             "#  include <netinet/tcp.h>",
             "#  include <sys/socket.h>",
             "#  include <sys/time.h>",
             "#  include <unistd.h>",
             "#endif",
             "",
             "#include <cerrno>",
             "#include <cstdio>",
             "#include <cstring>",
             "#include <ctime>",
             "#include <stdexcept>",
             "",
             f"namespace {self.ns} {{",
             "",
             self._platform_layer(),
            ]
        L.append(self._codec_text())
        L.append("")
        for name in self.usage.prim_reads:
            L.append(self._prim_read(name))
        for name in self.usage.prim_writes:
            L.append(self._prim_write(name))
        for type_str in self.usage.array_reads:
            L.append(self._array_read(type_str))
        for type_str in self.usage.array_writes:
            L.append(self._array_write(type_str))
        if self.usage.ref_reads:
            L.append(self._read_ref_text())
        L.append(self._value_codec_text())
        if any(_has_convert(f) for f in self.schema.functions) or \
           any(_has_convert(m) for c in self.schema.classes for m in c.methods()):
            L.append(self._value_to_string_text())
        L.append(self._value_impl_text())
        L.append(self._run_result_impl_text())
        L.append(self._remote_server_info_impl_text())
        L.append(self._context_impl())
        L += [
            "context::context() : m_impl(new context::context_impl()) {",
            "}",
            "",
            "context::~context() {",
            "    delete m_impl;",
            "}",
            "",
            "void context::set_script(const std::string &script) {",
            "    m_impl->set_script(script);",
            "}",
            "",
            "void context::add_server(remote_server_info *server) {",
            "    m_impl->add_server(server);",
            "}",
            "",
            "void context::add_class(s5run_object *sample) {",
            "    m_impl->register_type(sample->s5run_class_name(), sample->s5run_method_names());",
            "}",
            "",
            "void context::add_object(s5run_object *obj) {",
            "    m_impl->add_object(obj);",
            "}",
            "",
            "void context::add_function(s5run_function *fn) {",
            "    m_impl->add_function(fn);",
            "}",
            "",
            "context * make_context() {",
            "    return new context();",
            "}",
            "",
            "run_result run_script(context *ctx, const std::string &entry, "
            "const std::vector<value> &args) {",
            "    return ctx->m_impl->run(entry, args);",
            "}",
            "",
            f"}} // namespace {self.ns}",
            "",
        ]
        return "\n".join(L)

    # ------------------------------------------------------------------

    def _platform_layer(self) -> str:
        return """namespace {

#ifdef _WIN32
typedef SOCKET sock_t;
#define S5RUN_BAD_SOCKET INVALID_SOCKET

static bool s5run_sockets_init() {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

static void s5run_sockets_shutdown() {
    WSACleanup();
}

static void s5run_close_socket(sock_t s) {
    closesocket(s);
}

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
        if (sent <= 0)
            return false;
        p += sent;
        n -= (std::size_t)sent;
    }
    return true;
}

static bool s5run_recv_exact(sock_t s, char *p, std::size_t n) {
    while (n > 0) {
        int got = recv(s, p, (int)n, 0);
        if (got == 0)
            return false;
        if (got < 0)
            return false;
        p += got;
        n -= (std::size_t)got;
    }
    return true;
}

static sock_t s5run_connect_socket(const std::string &host, int port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16)port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE)
        return S5RUN_BAD_SOCKET;
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == S5RUN_BAD_SOCKET)
        return S5RUN_BAD_SOCKET;
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

static void s5run_close_socket(sock_t s) {
    ::close(s);
}

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
            if (sent < 0 && errno == EINTR)
                continue;
            return false;
        }
        p += sent;
        n -= (std::size_t)sent;
    }
    return true;
}

static bool s5run_recv_exact(sock_t s, char *p, std::size_t n) {
    while (n > 0) {
        ssize_t got = ::recv(s, p, n, 0);
        if (got == 0)
            return false;
        if (got < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        p += got;
        n -= (std::size_t)got;
    }
    return true;
}

static sock_t s5run_connect_socket(const std::string &host, int port) {
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u16)port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
        return S5RUN_BAD_SOCKET;
    sock_t s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == S5RUN_BAD_SOCKET)
        return S5RUN_BAD_SOCKET;
    if (::connect(s, (sockaddr *)&addr, sizeof(addr)) != 0) {
        s5run_close_socket(s);
        return S5RUN_BAD_SOCKET;
    }
    return s;
}
#endif

static std::string num_to_str(long long v) {
    char buf[32];
    std::sprintf(buf, "%lld", v);
    return std::string(buf);
}

static std::string num_to_str(int v) {
    char buf[32];
    std::sprintf(buf, "%d", v);
    return std::string(buf);
}

static std::string num_to_str(u32 v) {
    char buf[32];
    std::sprintf(buf, "%u", v);
    return std::string(buf);
}

} // namespace"""

    def _codec_text(self) -> str:
        return """namespace {

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
        for (int i = 0; i < 4; ++i)
            m_buf.push_back((u8)((v >> (8 * i)) & 0xFF));
    }
    void put_u64(u64 v) {
        for (int i = 0; i < 8; ++i)
            m_buf.push_back((u8)((v >> (8 * i)) & 0xFF));
    }
    void put_i8(i8 v) { put_u8((u8)v); }
    void put_i16(i16 v) { put_u16((u16)v); }
    void put_i32(i32 v) { put_u32((u32)v); }
    void put_i64(i64 v) { put_u64((u64)v); }
    void put_f64(double v) {
        u64 u;
        std::memcpy(&u, &v, 8);
        put_u64(u);
    }
    void put_string(const std::string &s) {
        std::size_t n = s.size();
        if (n <= 0xFF) {
            put_u8(S_LEN_1);
            put_u8((u8)n);
        } else if (n <= 0xFFFF) {
            put_u8(S_LEN_2);
            put_u16((u16)n);
        } else {
            put_u8(S_LEN_4);
            put_u32((u32)n);
        }
        for (std::size_t i = 0; i < n; ++i)
            m_buf.push_back((u8)s[i]);
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
        u16 v = (u16)(m_p[0] | (m_p[1] << 8));
        m_p += 2;
        return v;
    }
    u32 get_u32() {
        check(4);
        u32 v = 0;
        for (int i = 0; i < 4; ++i)
            v |= (u32)m_p[i] << (8 * i);
        m_p += 4;
        return v;
    }
    u64 get_u64() {
        check(8);
        u64 v = 0;
        for (int i = 0; i < 8; ++i)
            v |= (u64)m_p[i] << (8 * i);
        m_p += 8;
        return v;
    }
    i8 get_i8() { return (i8)get_u8(); }
    i16 get_i16() { return (i16)get_u16(); }
    i32 get_i32() { return (i32)get_u32(); }
    i64 get_i64() { return (i64)get_u64(); }
    double get_f64() {
        u64 u = get_u64();
        double d;
        std::memcpy(&d, &u, 8);
        return d;
    }
    std::string get_string() {
        u8 len_size = get_u8();
        std::size_t n = 0;
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
    void check(std::size_t n) const {
        if (m_p + n > m_end)
            throw std::runtime_error("wire: unexpected end of buffer");
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
    default:
        throw std::runtime_error("wire: expected an integer");
    }
}

void write_int(Writer &w, i64 v) {
    if (v >= -128 && v <= 127) {
        w.put_u8(T_INT8);
        w.put_i8((i8)v);
    } else if (v >= -32768 && v <= 32767) {
        w.put_u8(T_INT16);
        w.put_i16((i16)v);
    } else if (v >= -2147483648LL && v <= 2147483647LL) {
        w.put_u8(T_INT32);
        w.put_i32((i32)v);
    } else {
        w.put_u8(T_INT64);
        w.put_i64(v);
    }
}

} // namespace"""

    def _prim_read(self, name) -> str:
        if name in ("int", "uint"):
            return ""
        if name == "bool":
            return ("bool read_bool(Reader &r) {\n"
                    "    if (r.get_u8() != T_BOOL)\n"
                    "        throw std::runtime_error(\"wire: expected a bool\");\n"
                    "    return r.get_u8() != 0;\n"
                    "}\n\n")
        if name in ("float", "double"):
            return ("double read_double(Reader &r) {\n"
                    "    u8 tag = r.get_u8();\n"
                    "    if (tag == T_FLOAT64)\n"
                    "        return r.get_f64();\n"
                    "    throw std::runtime_error(\"wire: expected a double\");\n"
                    "}\n\n")
        if name == "string":
            return ("std::string read_string(Reader &r) {\n"
                    "    if (r.get_u8() != T_STRING)\n"
                    "        throw std::runtime_error(\"wire: expected a string\");\n"
                    "    return r.get_string();\n"
                    "}\n\n")
        return ("u8 read_byte(Reader &r) {\n"
                "    if (r.get_u8() != T_UINT8)\n"
                "        throw std::runtime_error(\"wire: expected a byte\");\n"
                "    return r.get_u8();\n"
                "}\n\n")

    def _prim_write(self, name) -> str:
        if name in ("int", "uint"):
            return ""
        if name == "bool":
            return ("void write_bool(Writer &w, bool v) {\n"
                    "    w.put_u8(T_BOOL);\n"
                    "    w.put_u8(v ? 1 : 0);\n"
                    "}\n\n")
        if name in ("float", "double"):
            return ("void write_double(Writer &w, double v) {\n"
                    "    w.put_u8(T_FLOAT64);\n"
                    "    w.put_f64(v);\n"
                    "}\n\n")
        if name == "string":
            return ("void write_string(Writer &w, const std::string &v) {\n"
                    "    w.put_u8(T_STRING);\n"
                    "    w.put_string(v);\n"
                    "}\n\n")
        return ("void write_byte(Writer &w, u8 v) {\n"
                "    w.put_u8(T_UINT8);\n"
                "    w.put_u8(v);\n"
                "}\n\n")

    def _array_read(self, type_str) -> str:
        ref = s5run_parse_type(type_str)
        inner = s5run_parse_type(ref.inner())
        sfx = self.usage.suffix(type_str)
        inner_cpp = type_to_cpp(inner)
        read_inner = self._read_expr(inner)
        return ("\n".join([
            f"std::vector<{inner_cpp}> read_{sfx}(Reader &r) {{",
            "    if (r.get_u8() != T_ARRAY)",
            "        throw std::runtime_error(\"wire: expected array\");",
            "    u32 n = r.get_u32();",
            f"    std::vector<{inner_cpp}> out;",
            "    out.reserve(n);",
            "    for (u32 i = 0; i < n; ++i)",
            f"        out.push_back({read_inner});",
            "    return out;",
            "}",
            "",
        ]))

    def _array_write(self, type_str) -> str:
        ref = s5run_parse_type(type_str)
        inner = s5run_parse_type(ref.inner())
        sfx = self.usage.suffix(type_str)
        inner_cpp = type_to_cpp(inner)
        write_inner = self._write_stmt(inner, "v[i]").rstrip(";")
        return ("\n".join([
            f"void write_{sfx}(Writer &w, const std::vector<{inner_cpp}> &v) {{",
            "    w.put_u8(T_ARRAY);",
            "    w.put_u32((u32)v.size());",
            "    for (std::size_t i = 0; i < v.size(); ++i)",
            f"        {write_inner};",
            "}",
            "",
        ]))

    def _read_ref_text(self) -> str:
        return ("i32 read_ref(Reader &r) {\n"
                "    if (r.get_u8() != T_REF)\n"
                "        throw std::runtime_error(\"wire: expected an object reference\");\n"
                "    i32 this_id = r.get_i32();\n"
                "    r.get_string(); // skip the type name\n"
                "    return this_id;\n"
                "}\n\n")

    def _value_codec_text(self) -> str:
        return """void encode_value(Writer &w, const value &v) {
    switch (v.get_kind()) {
    case value::K_NULL: w.put_u8(T_UNDEFINED); break;
    case value::K_BOOL: w.put_u8(T_BOOL); w.put_u8(v.as_bool() ? 1 : 0); break;
    case value::K_INT: write_int(w, v.as_int()); break;
    case value::K_DOUBLE: w.put_u8(T_FLOAT64); w.put_f64(v.as_double()); break;
    case value::K_STRING: write_string(w, v.as_string()); break;
    case value::K_ARRAY: {
        w.put_u8(T_ARRAY);
        const std::vector<value> &a = v.as_array();
        w.put_u32((u32)a.size());
        for (std::size_t i = 0; i < a.size(); ++i)
            encode_value(w, a[i]);
        break;
    }
    case value::K_OBJECT: {
        w.put_u8(T_OBJECT);
        const std::map<std::string, value> &o = v.as_object();
        w.put_u32((u32)o.size());
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
        std::vector<value> a;
        a.reserve(n);
        for (u32 i = 0; i < n; ++i)
            a.push_back(decode_value(r));
        return value(a);
    }
    case T_OBJECT: {
        u32 n = r.get_u32();
        std::map<std::string, value> o;
        for (u32 i = 0; i < n; ++i) {
            std::string key = read_string(r);
            o[key] = decode_value(r);
        }
        return value(o);
    }
    case T_REF: {
        i32 this_id = r.get_i32();
        std::string type = r.get_string();
        return value::make_ref(this_id, type);
    }
    default:
        throw std::runtime_error("wire: unknown value tag");
    }
}"""

    def _value_to_string_text(self) -> str:
        return """std::string _value_to_string(const value &v) {
    switch (v.get_kind()) {
    case value::K_NULL: return "null";
    case value::K_BOOL: return v.as_bool() ? "true" : "false";
    case value::K_INT: return num_to_str(v.as_int());
    case value::K_DOUBLE: {
        char buf[64];
        std::sprintf(buf, "%g", v.as_double());
        return std::string(buf);
    }
    case value::K_STRING: return v.as_string();
    default:
        throw std::runtime_error("wire: expected a scalar argument");
    }
}"""

    def _value_impl_text(self) -> str:
        return """value::value() : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0) {}
value::value(bool b) : m_kind(K_BOOL), m_b(b), m_i(0), m_d(0.0) {}
value::value(i64 i) : m_kind(K_INT), m_b(false), m_i(i), m_d(0.0) {}
value::value(double d) : m_kind(K_DOUBLE), m_b(false), m_i(0), m_d(d) {}
value::value(const char *s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
value::value(const std::string &s) : m_kind(K_STRING), m_b(false), m_i(0), m_d(0.0), m_s(s) {}
value::value(const std::vector<value> &a) : m_kind(K_ARRAY), m_b(false), m_i(0), m_d(0.0), m_a(a) {}
value::value(const std::map<std::string, value> &o) : m_kind(K_OBJECT), m_b(false), m_i(0), m_d(0.0), m_o(o) {}

value::value(const value &other) : m_kind(K_NULL), m_b(false), m_i(0), m_d(0.0) {
    copy_from(other);
}

value &value::operator=(const value &other) {
    if (this != &other) {
        destroy();
        copy_from(other);
    }
    return *this;
}

value::~value() {
    destroy();
}

value value::make_ref(int this_id, const std::string &type) {
    value v;
    v.m_kind = K_REF;
    v.m_i = this_id;
    v.m_s = type;
    return v;
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

// members are plain (non-union) fields: the compiler destroys and
// copies them, so destroy/copy_from only touch the scalars.
void value::destroy() {
    m_kind = K_NULL;
}

void value::copy_from(const value &other) {
    m_kind = other.m_kind;
    m_b = other.m_b;
    m_i = other.m_i;
    m_d = other.m_d;
    m_s = other.m_s;
    m_a = other.m_a;
    m_o = other.m_o;
}"""

    def _run_result_impl_text(self) -> str:
        return """run_result::run_result()
    : m_has_error(false), m_error_line(0), m_duration_ms(0) {}

bool run_result::has_error() const { return m_has_error; }
const std::string &run_result::error() const { return m_error; }
int run_result::error_line() const { return m_error_line; }
i64 run_result::duration() const { return m_duration_ms; }
const value &run_result::get_value() const { return m_value; }"""

    def _remote_server_info_impl_text(self) -> str:
        return """remote_server_info::remote_server_info() : m_port(0) {}

void remote_server_info::set_address(const std::string &address) {
    const std::string prefix = "s5run://";
    if (address.compare(0, prefix.size(), prefix) != 0)
        throw std::runtime_error("set_address: expected format s5run://host:port");
    std::string rest = address.substr(prefix.size());
    std::size_t colon = rest.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 >= rest.size())
        throw std::runtime_error("set_address: expected format s5run://host:port");
    m_address = address;
    m_host = rest.substr(0, colon);
    std::sscanf(rest.substr(colon + 1).c_str(), "%d", &m_port);
}

const std::string &remote_server_info::get_address() const { return m_address; }
void remote_server_info::set_api_key(const std::string &api_key) { m_api_key = api_key; }
const std::string &remote_server_info::get_api_key() const { return m_api_key; }
const std::string &remote_server_info::get_host() const { return m_host; }
int remote_server_info::get_port() const { return m_port; }"""

    # ------------------------------------------------------------------
    # context::impl
    # ------------------------------------------------------------------

    def _context_impl(self) -> str:
        s = self.schema

        # declarations
        fn_cbs = []
        for f in s.functions:
            fn_cbs.append(f"    void cb_{f.name()}(i32 cb_id, Reader &r);")
        m_cbs = []
        for c in s.classes:
            for m in c.methods():
                m_cbs.append(f"    void cbm_{c.name()}_{m.name()}(i32 cb_id, Reader &r);")
        reg_inst = []
        for c in s.classes:
            reg_inst.append(
                f"    i32 register_instance_{c.name()}({c.name()} *obj);")

        fields = []
        for f in s.functions:
            fields.append(f"    GlobalFunction_{f.name()} *m_fn_{f.name()};")
        for c in s.classes:
            fields.append(f"    std::map<i32, {c.name()} *> m_instances_{c.name()};")

        # callback implementations
        fn_impls = []
        for f in s.functions:
            nargs = len(f.arguments())
            conv = _has_convert(f)
            head = ["    u32 n = r.get_u32();"]
            if not conv:
                head += [
                    "    if (n != %d) {" % nargs,
                    f"        throw std::runtime_error(\"{f.name()}: expected %d argument(s)\");" % nargs,
                    "    }",
                ]
            if conv:
                head.append("    u32 conv_n = n;")
            call_args = ", ".join(f"a_{a.name()}" for a in f.arguments())
            body = []
            if _is_void(f):
                body.append(f"    m_fn_{f.name()}->execute({call_args});")
            else:
                body.append(f"    {type_to_cpp(f.return_ref())} res = m_fn_{f.name()}->execute({call_args});")
            L = [f"void context::context_impl::cb_{f.name()}(i32 cb_id, Reader &r) {{"]
            L += head
            if conv:
                a0 = f.arguments()[0]
                L[-1] = "    u32 n = r.get_u32();"
                L.append(f"    std::vector<std::string> a_{a0.name()};")
                L.append(f"    a_{a0.name()}.reserve(n);")
                L.append("    for (u32 i = 0; i < n; ++i)")
                L.append("        a_" + a0.name() + ".push_back(_value_to_string(decode_value(r)));")
            else:
                L += self._decode_args(f)
            L += body
            L += ["    Writer w;",
                  "    w.put_i32(cb_id);",
                  "    w.put_u8(1);"]
            if not _is_void(f):
                L += self._encode_result(f, "res")
            else:
                L.append("    w.put_u8(T_UNDEFINED);")
            L += ["    send_message(M_CALLBACK_RESP, w.data());",
                  "}",
                  ""]
            fn_impls.append("\n".join(L))

        m_impls = []
        for c in s.classes:
            for m in c.methods():
                conv = _has_convert(m)
                if conv:
                    a0 = m.arguments()[0]
                    head = [
                        "    u32 n = r.get_u32();",
                        "    if (n < 1) {",
                        f"        throw std::runtime_error(\"{c.name()}.{m.name()}: expected this_id\");",
                        "    }",
                        "    n -= 1;",
                    ]
                    args_code = [
                        f"    std::vector<std::string> a_{a0.name()};",
                        f"    a_{a0.name()}.reserve(n);",
                        "    for (u32 i = 0; i < n; ++i)",
                        "        a_" + a0.name() + ".push_back(_value_to_string(decode_value(r)));",
                    ]
                    lookup = [
                        "    i32 this_id = (i32)read_int(r);",
                        f"    std::map<i32, {c.name()} *>::const_iterator it =",
                        f"        m_instances_{c.name()}.find(this_id);",
                        f"    if (it == m_instances_{c.name()}.end()) {{",
                        f"        throw std::runtime_error(\"{c.name()}: unknown this_id\");",
                        "    }",
                    ]
                else:
                    expected = 1 + len(m.arguments())
                    throw_line = (
                        "    if (n != " + str(expected) + ") { throw std::runtime_error(\""
                        + c.name() + "." + m.name()
                        + ": expected " + str(expected) + " argument(s)\"); }")
                    head = [
                        "    u32 n = r.get_u32();",
                        throw_line,
                    ]
                    args_code = []
                    lookup = [
                        "    i32 this_id = (i32)read_int(r);",
                        f"    std::map<i32, {c.name()} *>::const_iterator it =",
                        f"        m_instances_{c.name()}.find(this_id);",
                        f"    if (it == m_instances_{c.name()}.end()) {{",
                        f"        throw std::runtime_error(\"{c.name()}: unknown this_id\");",
                        "    }",
                    ]
                if not conv:
                    args_code = self._decode_args(m)
                call_args = ", ".join(f"a_{a.name()}" for a in m.arguments())
                body = []
                if _is_void(m):
                    body.append(f"    it->second->{m.name()}({call_args});")
                else:
                    body.append(f"    {type_to_cpp(m.return_ref())} res = it->second->{m.name()}({call_args});")
                L = [f"void context::context_impl::cbm_{c.name()}_{m.name()}(i32 cb_id, Reader &r) {{"]
                L += head
                L += lookup
                L += args_code
                L += body
                L += ["    Writer w;",
                      "    w.put_i32(cb_id);",
                      "    w.put_u8(1);"]
                if not _is_void(m):
                    L += self._encode_result(m, "res")
                else:
                    L.append("    w.put_u8(T_UNDEFINED);")
                L += ["    send_message(M_CALLBACK_RESP, w.data());",
                      "}",
                      ""]
                m_impls.append("\n".join(L))

        # register instance impls
        reg_impls = []
        for c in s.classes:
            reg_impls.append("\n".join([
                f"i32 context::context_impl::register_instance_{c.name()}({c.name()} *obj) {{",
                "    i32 id = ++m_next_this_id;",
                f"    m_instances_{c.name()}[id] = obj;",
                "    return id;",
                "}",
                "",
            ]))

        # dispatch: if/else chain by name
        dispatch = []
        for f in s.functions:
            dispatch.append(
                f'    if (method == "{f.name()}") {{ cb_{f.name()}(cb_id, r); return; }}')
        for c in s.classes:
            for m in c.methods():
                dispatch.append(
                    f'    if (method == "{m.name()}") {{ cbm_{c.name()}_{m.name()}(cb_id, r); return; }}')

        return "\n".join([
            "// ---------------------------------------------------------------------------",
            "// context::impl",
            "// ---------------------------------------------------------------------------",
            "",
            "class context::context_impl {",
            "public:",
            "    context_impl() : m_sock(S5RUN_BAD_SOCKET), m_next_this_id(0) {",
            "        s5run_sockets_init();",
            "    }",
            "",
            "    ~context_impl() {",
            "        disconnect();",
            "        s5run_sockets_shutdown();",
            "    }",
            "",
            "    void set_script(const std::string &script);",
            "    void add_server(remote_server_info *server);",
            "    void register_type(const std::string &name,",
            "                       const std::vector<std::string> &methods);",
            "    void add_object(s5run_object *obj);",
            "    void add_function(s5run_function *fn);",
            "    run_result run(const std::string &entry, const std::vector<value> &args);",
            "",
            "private:",
            "    struct PendingType {",
            "        std::string name;",
            "        std::vector<std::string> methods;",
            "    };",
            "    struct PendingObject {",
            "        std::string name;",
            "        i32 this_id;",
            "        std::vector<std::string> methods;",
            "    };",
            "",
            "\n".join(fn_cbs),
            "\n".join(m_cbs),
            "\n".join(reg_inst),
            "    void store_instance(const std::string &cls, i32 id, s5run_object *obj);",
            "    void dispatch_callback(const std::vector<u8> &body);",
            "    void connect();",
            "    void disconnect();",
            "    void flush_pending();",
            "    static void read_hello(sock_t s);",
            "    static bool send_frame(sock_t s, u8 msg_type, const std::vector<u8> &body);",
            "    static bool read_frame(sock_t s, u8 &msg_type, std::vector<u8> &body);",
            "    void send_message(u8 msg_type, const std::vector<u8> &body);",
            "    void expect_register_resp(const std::string &what);",
            "    static i64 probe_server_load(remote_server_info *server);",
            "    static int parse_error_line(const std::string &error);",
            "    static int parse_int_substr(const std::string &s, std::size_t p1, std::size_t p2);",
            "",
            "    std::string m_script;",
            "    std::vector<remote_server_info *> m_servers;",
            "    std::vector<PendingType> m_pending_types;",
            "    std::vector<PendingObject> m_pending_objects;",
            "    std::vector<std::string> m_pending_functions;",
            "    sock_t m_sock;",
            "    i32 m_next_this_id;",
            "\n".join(fields),
            "};",
            "",
            "void context::context_impl::set_script(const std::string &script) {",
            "    m_script = script;",
            "}",
            "",
            "void context::context_impl::add_server(remote_server_info *server) {",
            "    m_servers.push_back(server);",
            "}",
            "",
            "void context::context_impl::register_type(const std::string &name,",
            "                                          const std::vector<std::string> &methods) {",
            "    if (m_sock != S5RUN_BAD_SOCKET) {",
            "        Writer w;",
            "        w.put_string(name);",
            "        w.put_u32((u32)methods.size());",
            "        for (std::size_t i = 0; i < methods.size(); ++i)",
            "            w.put_string(methods[i]);",
            "        send_message(M_REGISTER_TYPE, w.data());",
            "        expect_register_resp(\"register_type(\" + name + \")\");",
            "        return;",
            "    }",
            "    PendingType t;",
            "    t.name = name;",
            "    t.methods = methods;",
            "    m_pending_types.push_back(t);",
            "}",
            "",
            "void context::context_impl::add_object(s5run_object *obj) {",
            "    const std::string cls = obj->s5run_class_name();",
            "    const std::vector<std::string> methods = obj->s5run_method_names();",
            "    i32 id = ++m_next_this_id;",
            "    // store into the typed registry via the name-keyed add below",
            "    store_instance(cls, id, obj);",
            "    if (m_sock != S5RUN_BAD_SOCKET) {",
            "        Writer w;",
            "        w.put_string(cls);",
            "        w.put_i32(id);",
            "        w.put_string(cls);",
            "        w.put_u32((u32)methods.size());",
            "        for (std::size_t i = 0; i < methods.size(); ++i)",
            "            w.put_string(methods[i]);",
            "        send_message(M_REGISTER_OBJ, w.data());",
            "        expect_register_resp(\"register_object(\" + cls + \")\");",
            "        return;",
            "    }",
            "    PendingObject o;",
            "    o.name = cls;",
            "    o.this_id = id;",
            "    o.methods = methods;",
            "    m_pending_objects.push_back(o);",
            "}",
            "",
            "\n".join(self._store_instance_impls()),
            "\n".join(self._add_function_impls()),
            "\n".join(fn_impls),
            "\n".join(m_impls),
            "\n".join(reg_impls),
            "\n".join(self._dispatch_and_io_impls(dispatch)),
        ])

    def _store_instance_impls(self) -> list:
        return ["void context::context_impl::store_instance(const std::string &cls, i32 id, s5run_object *obj) {"] + [
            f'    if (cls == "{c.name()}") {{ m_instances_{c.name()}[id] = ({c.name()} *)obj; return; }}'
            for c in self.schema.classes
        ] + [
            '    throw std::runtime_error("add_object: unknown class: " + cls);',
            "}",
            "",
        ]

    def _add_function_impls(self) -> list:
        L = ["void context::context_impl::add_function(s5run_function *fn) {"]
        for f in self.schema.functions:
            L.append(f'    if (fn->s5run_function_name() == "{f.name()}") {{')
            L.append(f"        m_fn_{f.name()} = (GlobalFunction_{f.name()} *)fn;")
            L.append('    }')
        L.append("    if (m_sock != S5RUN_BAD_SOCKET) {")
        L.append("        Writer w;")
        L.append("        w.put_string(fn->s5run_function_name());")
        L.append("        send_message(M_REGISTER_FUNC, w.data());")
        L.append('        expect_register_resp("register_function(" + fn->s5run_function_name() + ")");')
        L.append("        return;")
        L.append("    }")
        L.append("    m_pending_functions.push_back(fn->s5run_function_name());")
        L.append("}")
        L.append("")
        return L

    def _dispatch_and_io_impls(self, dispatch) -> list:
        return ["void context::context_impl::dispatch_callback(const std::vector<u8> &body) {",
                "    Reader r(body.empty() ? 0 : &body[0], body.size());",
                "    i32 cb_id = r.get_i32();",
                "    std::string method = r.get_string();",
                "    try {",
                "\n".join("        " + d.strip() for d in dispatch),
                "    } catch (const std::exception &e) {",
                "        Writer w;",
                "        w.put_i32(cb_id);",
                "        w.put_u8(0);",
                '        w.put_string(e.what());',
                "        send_message(M_CALLBACK_RESP, w.data());",
                "        return;",
                "    }",
                "    {",
                '    Writer w;',
                "    w.put_i32(cb_id);",
                "    w.put_u8(0);",
                '    w.put_string("unknown method: " + method);',
                "    send_message(M_CALLBACK_RESP, w.data());",
                "    }",
                "}",
                "",
                "void context::context_impl::connect() {",
                "    if (m_sock != S5RUN_BAD_SOCKET)",
                "        return;",
                "    if (m_servers.empty())",
                '        throw std::runtime_error("s5run: no server added (add_server)");',
                "    std::size_t best = 0;",
                "    if (m_servers.size() > 1) {",
                "        i64 best_load = -1;",
                "        for (std::size_t i = 0; i < m_servers.size(); ++i) {",
                "            i64 load = probe_server_load(m_servers[i]);",
                "            if (load < 0)",
                "                continue;",
                "            if (best_load < 0 || load < best_load) {",
                "                best_load = load;",
                "                best = i;",
                "            }",
                "        }",
                "        if (best_load < 0)",
                '            throw std::runtime_error("s5run: all servers are unreachable");',
                "    }",
                "    sock_t s = s5run_connect_socket(m_servers[best]->get_host(),",
                "                                    m_servers[best]->get_port());",
                "    if (s == S5RUN_BAD_SOCKET)",
                '        throw std::runtime_error("s5run: connect failed");',
                "    s5run_set_nodelay(s);",
                "    s5run_set_recv_timeout(s, 10000);",
                "    m_sock = s;",
                "    read_hello(m_sock);",
                "}",
                "",
                "void context::context_impl::read_hello(sock_t s) {",
                "    u8 msg_type = 0;",
                "    std::vector<u8> body;",
                "    if (!read_frame(s, msg_type, body) || msg_type != M_SERVER_HELLO)",
                '        throw std::runtime_error("s5run: expected server hello");',
                "    std::string text(body.empty() ? \"\" : std::string((const char *)&body[0], body.size()));",
                '    if (text.compare(0, 6, "s5run ") != 0)',
                '        throw std::runtime_error("s5run: bad server hello: " + text);',
                "    std::size_t last_space = text.rfind(' ');",
                "    if (last_space == std::string::npos)",
                '        throw std::runtime_error("s5run: bad server hello: " + text);',
                "    int protocol = 0; std::sscanf(text.substr(last_space + 1).c_str(), \"%d\", &protocol);",
                "    if (protocol != 2)",
                '        throw std::runtime_error("s5run: server protocol version "',
                "                              + num_to_str(protocol)",
                '                              + " is not supported (client supports 2)");',
                "}",
                "",
                "void context::context_impl::disconnect() {",
                "    if (m_sock != S5RUN_BAD_SOCKET) {",
                "        s5run_close_socket(m_sock);",
                "        m_sock = S5RUN_BAD_SOCKET;",
                "    }",
                "}",
                "",
                "i64 context::context_impl::probe_server_load(remote_server_info *server) {",
                "    sock_t s = s5run_connect_socket(server->get_host(), server->get_port());",
                "    if (s == S5RUN_BAD_SOCKET)",
                "        return -1;",
                "    s5run_set_nodelay(s);",
                "    s5run_set_recv_timeout(s, 2000);",
                "    try {",
                "        read_hello(s);",
                "        std::vector<u8> empty_body;",
                "        if (!send_frame(s, M_STATS_REQ, empty_body)) {",
                "            s5run_close_socket(s);",
                "            return -1;",
                "        }",
                "        u8 msg_type = 0;",
                "        std::vector<u8> body;",
                "        if (!read_frame(s, msg_type, body) || msg_type != M_STATS_RESP || body.size() < 16) {",
                "            s5run_close_socket(s);",
                "            return -1;",
                "        }",
                "        s5run_close_socket(s);",
                "        Reader r(&body[0], body.size());",
                "        r.get_u32(); // threads",
                "        u32 busy = r.get_u32();",
                "        u32 queued = r.get_u32();",
                "        return (i64)busy + (i64)queued;",
                "    } catch (...) {",
                "        s5run_close_socket(s);",
                "        return -1;",
                "    }",
                "}",
                "",
                "void context::context_impl::flush_pending() {",
                "    for (std::size_t i = 0; i < m_pending_types.size(); ++i) {",
                "        register_type(m_pending_types[i].name, m_pending_types[i].methods);",
                "    }",
                "    m_pending_types.clear();",
                "    for (std::size_t i = 0; i < m_pending_objects.size(); ++i) {",
                "        const PendingObject &o = m_pending_objects[i];",
                "        Writer w;",
                "        w.put_string(o.name);",
                "        w.put_i32(o.this_id);",
                "        w.put_string(o.name);",
                "        w.put_u32((u32)o.methods.size());",
                "        for (std::size_t j = 0; j < o.methods.size(); ++j)",
                "            w.put_string(o.methods[j]);",
                "        send_message(M_REGISTER_OBJ, w.data());",
                '        expect_register_resp("register_object(" + o.name + ")");',
                "    }",
                "    m_pending_objects.clear();",
                "    for (std::size_t i = 0; i < m_pending_functions.size(); ++i) {",
                "        Writer w;",
                "        w.put_string(m_pending_functions[i]);",
                "        send_message(M_REGISTER_FUNC, w.data());",
                '        expect_register_resp("register_function(" + m_pending_functions[i] + ")");',
                "    }",
                "    m_pending_functions.clear();",
                "}",
                "",
                "bool context::context_impl::send_frame(sock_t s, u8 msg_type, const std::vector<u8> &body) {",
                "    u32 len = (u32)(1 + body.size());",
                "    u32 net_len = htonl(len);",
                "    if (!s5run_send_all(s, (const char *)&net_len, sizeof(net_len)))",
                "        return false;",
                "    if (!s5run_send_all(s, (const char *)&msg_type, 1))",
                "        return false;",
                "    return body.empty() ? true : s5run_send_all(s, (const char *)&body[0], body.size());",
                "}",
                "",
                "void context::context_impl::send_message(u8 msg_type, const std::vector<u8> &body) {",
                "    if (!send_frame(m_sock, msg_type, body))",
                '        throw std::runtime_error("s5run: send failed");',
                "}",
                "",
                "bool context::context_impl::read_frame(sock_t s, u8 &msg_type, std::vector<u8> &body) {",
                "    char head[5];",
                "    if (!s5run_recv_exact(s, head, 5))",
                "        return false;",
                "    u32 len = ((u32)(u8)head[0] << 24) | ((u32)(u8)head[1] << 16)",
                "              | ((u32)(u8)head[2] << 8) | (u32)(u8)head[3];",
                "    msg_type = (u8)head[4];",
                "    if (len == 0 || len > 64 * 1024 * 1024)",
                "        return false;",
                "    body.resize(len - 1);",
                "    if (len > 1 && !s5run_recv_exact(s, (char *)&body[0], body.size()))",
                "        return false;",
                "    return true;",
                "}",
                "",
                "void context::context_impl::expect_register_resp(const std::string &what) {",
                "    u8 msg_type = 0;",
                "    std::vector<u8> body;",
                "    if (!read_frame(m_sock, msg_type, body) || msg_type != M_REGISTER_RESP)",
                '        throw std::runtime_error("s5run: " + what + " failed");',
                "    Reader r(body.empty() ? 0 : &body[0], body.size());",
                "    if (r.get_u8() == 0)",
                '        throw std::runtime_error("s5run: " + what + " error: " + r.get_string());',
                "}",
                "",
                "int context::context_impl::parse_int_substr(const std::string &s, std::size_t p1, std::size_t p2) {",
                "    int out = 0;",
                "    std::sscanf(s.substr(p1, p2 - p1).c_str(), \"%d\", &out);",
                "    return out;",
                "}",
                "",
                "int context::context_impl::parse_error_line(const std::string &error) {",
                "    for (std::size_t pos = 0; pos < error.size(); ++pos) {",
                "        if (error[pos] != ':')",
                "            continue;",
                "        std::size_t p1 = pos + 1;",
                "        if (p1 >= error.size() || error[p1] < '0' || error[p1] > '9')",
                "            continue;",
                "        std::size_t p2 = p1;",
                "        while (p2 < error.size() && error[p2] >= '0' && error[p2] <= '9')",
                "            ++p2;",
                "        if (p2 + 1 < error.size() && error[p2] == ':'"
                "            && error[p2 + 1] >= '0' && error[p2 + 1] <= '9')",
                "            return parse_int_substr(error, p1, p2);",
                "        pos = p2;",
                "    }",
                "    return 0;",
                "}",
                "",
                "run_result context::context_impl::run(const std::string &entry,",
                "                                      const std::vector<value> &args) {",
                "    run_result out;",
                "    std::clock_t started = std::clock();",
                "    try {",
                "        connect();",
                "        flush_pending();",
                "        Writer w;",
                "        w.put_string(m_script);",
                "        w.put_string(entry);",
                "        w.put_u32((u32)args.size());",
                "        for (std::size_t i = 0; i < args.size(); ++i)",
                "            encode_value(w, args[i]);",
                "        send_message(M_EXEC_REQ, w.data());",
                "        while (true) {",
                "            u8 msg_type = 0;",
                "            std::vector<u8> body;",
                "            if (!read_frame(m_sock, msg_type, body))",
                '                throw std::runtime_error("s5run: connection closed");',
                "            if (msg_type == M_CALLBACK_REQ) {",
                "                try {",
                "                    dispatch_callback(body);",
                "                } catch (const std::exception &e) {",
                "                    // error already reported back to the server",
                "                    (void)e;",
                "                }",
                "                continue;",
                "            }",
                "            if (msg_type == M_EXEC_RESP) {",
                "                Reader r(body.empty() ? 0 : &body[0], body.size());",
                "                if (r.get_u8() == 0)",
                "                    throw std::runtime_error(r.get_string());",
                "                out.m_value = decode_value(r);",
                "                break;",
                "            }",
                '            throw std::runtime_error("s5run: unexpected message type");',
                "        }",
                "    } catch (const std::exception &e) {",
                "        out.m_has_error = true;",
                "        out.m_error = e.what();",
                "        out.m_error_line = parse_error_line(out.m_error);",
                "    }",
                "    out.m_duration_ms = (i64)((double)(std::clock() - started) * 1000.0 / CLOCKS_PER_SEC);",
                "    return out;",
                "}"]


def generate_header(schema) -> str:
    return _Cpp99Generator(schema).generate_header()


def generate_impl(schema) -> str:
    return _Cpp99Generator(schema).generate_impl()
