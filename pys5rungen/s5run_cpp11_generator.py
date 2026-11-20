#!/usr/bin/env python3
##################################################################################
#
#            ░░░░░░░
#           ░██▓▓▓▓▒
#  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
#  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
#   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
#  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
#   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
#
# MIT License
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
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
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
##################################################################################

"""C++11 client generator.

Produces a standalone {ns}.h / {ns}.cpp pair: typed base classes from the
schema (a developer inherits from them and registers implementations on a
client), the binary wire codec matching the s5run server protocol, and the
transport client with typed callback dispatch.
"""

from .s5run_typeref import s5run_parse_type

# schema primitive name -> C++ type
PRIMITIVES = {
    "bool":   "bool",
    "int":    "int64_t",
    "uint":   "uint64_t",
    "float":  "double",
    "double": "double",
    "string": "std::string",
    "byte":   "uint8_t",
}


def type_to_cpp(ref) -> str:
    if ref.is_prim():
        return PRIMITIVES[ref.name()]
    if ref.is_array():
        return f"std::vector<{type_to_cpp(s5run_parse_type(ref.inner()))}>"
    if ref.is_ref():
        return f"std::shared_ptr<{ref.name()}>"
    return ref.name()


def _param_type(ref) -> str:
    t = type_to_cpp(ref)
    if t.startswith("std::"):
        return f"const {t} &"
    return t


def _is_void(fm) -> bool:
    return fm.return_type() == "void"


def _has_convert(fm) -> bool:
    args = fm.arguments()
    return len(args) == 1 and bool(args[0].convert_from())


class _TypeUsage:
    """Tracks which codec helpers are used by signatures."""

    def __init__(self):
        self.prim_reads = []    # primitive names needing a read_X
        self.prim_writes = []   # primitive names needing a write_X
        self.array_reads = []   # array type strings needing a read_X
        self.array_writes = []  # array type strings needing a write_X
        self.ref_reads = False  # object references used as arguments

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
            if not allow_ref:
                raise ValueError(
                    "object references are only supported as return values, got: " + type_str)
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


class _Cpp11Generator:

    def __init__(self, schema):
        self.schema = schema
        self.ns = schema.get_cpp_options("cpp11").namespace()
        self.usage = _TypeUsage()
        for c in schema.classes:
            for m in c.methods():
                self.usage.add_all(m)
        for f in schema.functions:
            self.usage.add_all(f)
        if schema.classes:
            self.usage.need_read("int")        # this_id in method callbacks
        self.usage.need_write("int")           # encode_value (K_INT)
        self.usage.need_write("string")        # encode_value (K_STRING)

    # ------------------------------------------------------------------
    # typed read/write call expressions
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

    def _encode_result(self, fm, expr) -> list:
        """Statements writing the result of a call into Writer w."""
        if _is_void(fm):
            return ["    w.put_u8(T_UNDEFINED);"]
        ref = fm.return_ref()
        if ref.is_array() and s5run_parse_type(ref.inner()).is_ref():
            cls = s5run_parse_type(ref.inner()).name()
            return [
                "    w.put_u8(T_ARRAY);",
                f"    w.put_u32((uint32_t){expr}.size());",
                f"    for (size_t i = 0; i < {expr}.size(); ++i) {{",
                f"        int32_t obj_id = _register_instance_{cls}({expr}[i]);",
                "        w.put_u8(T_REF);",
                "        w.put_i32(obj_id);",
                f'        w.put_string("{cls}");',
                "    }",
            ]
        if ref.is_ref():
            cls = ref.name()
            return [
                f"    int32_t obj_id = _register_instance_{cls}({expr});",
                "    w.put_u8(T_REF);",
                "    w.put_i32(obj_id);",
                f'    w.put_string("{cls}");',
            ]
        return ["        " + self._write_stmt(ref, expr)]

    def _decode_args(self, fm, prefix="a_") -> list:
        L = []
        if _has_convert(fm):
            a = fm.arguments()[0]
            cpp_t = type_to_cpp(a.type_ref())
            L += [
                f"    {cpp_t} {prefix}{a.name()};",
                f"    {prefix}{a.name()}.reserve(n);",
                "    for (uint32_t i = 0; i < n; ++i)",
                f"        {prefix}{a.name()}.push_back(_value_to_string(decode_value(r)));",
            ]
            return L
        for a in fm.arguments():
            ref = a.type_ref()
            if ref.is_array() and s5run_parse_type(ref.inner()).is_ref():
                cls = s5run_parse_type(ref.inner()).name()
                L += [
                    "    if (r.get_u8() != T_ARRAY)",
                    '        throw std::runtime_error("wire: expected array");',
                    "    uint32_t cnt_" + a.name() + " = r.get_u32();",
                    f"    std::vector<std::shared_ptr<{cls}>> {prefix}{a.name()};",
                    f"    {prefix}{a.name()}.reserve(cnt_{a.name()});",
                    f"    for (uint32_t i = 0; i < cnt_{a.name()}; ++i) {{",
                    f"        int32_t {prefix}ref_id_{a.name()} = read_ref(r);",
                    f"        std::map<int32_t, std::shared_ptr<{cls}>>::const_iterator "
                    f"{prefix}it_{a.name()} =",
                    f"            m_instances_{cls}.find({prefix}ref_id_{a.name()});",
                    f"        if ({prefix}it_{a.name()} == m_instances_{cls}.end()) {{",
                    f"            throw std::runtime_error(\"{fm.name()}: unknown object reference: \""
                    f" + std::to_string({prefix}ref_id_{a.name()}));",
                    "        }",
                    f"        {prefix}{a.name()}.push_back({prefix}it_{a.name()}->second);",
                    "    }",
                ]
                continue
            if ref.is_ref():
                cls = ref.name()
                L += [
                    f"    int32_t {prefix}ref_id_{a.name()} = read_ref(r);",
                    f"    std::map<int32_t, std::shared_ptr<{cls}>>::const_iterator "
                    f"{prefix}it_{a.name()} =",
                    f"        m_instances_{cls}.find({prefix}ref_id_{a.name()});",
                    f"    if ({prefix}it_{a.name()} == m_instances_{cls}.end()) {{",
                    f"        throw std::runtime_error(\"{fm.name()}: unknown object reference: \""
                    f" + std::to_string({prefix}ref_id_{a.name()}));",
                    "    }",
                    f"    std::shared_ptr<{cls}> {prefix}{a.name()} = {prefix}it_{a.name()}->second;",
                ]
            else:
                cpp_t = type_to_cpp(ref)
                L.append(f"    {cpp_t} {prefix}{a.name()} = {self._read_expr(ref)};")
        return L

    # ------------------------------------------------------------------
    # header
    # ------------------------------------------------------------------

    def generate_header(self) -> str:
        s = self.schema
        L = ["// AUTO-GENERATED. DO NOT EDIT.",
             "#pragma once",
             "",
             "#include <cstdint>",
             "#include <map>",
             "#include <memory>",
             "#include <string>",
             "#include <vector>",
             "",
             f"namespace {self.ns} {{",
             "",
             "class context;",
             "class remote_server_info;",
             "class run_result;",
             "std::shared_ptr<context> make_context();",
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
        L.append(self._function_root_class())
        L.append("")
        for f in s.functions:
            L.append(self._global_function_class(f))
            L.append("")
        L.append(self._client_class())
        L.append("")
        L.append(self._run_result_class())
        L.append("")
        L.append(
            "// Runs the context script; serves callbacks while it executes.\n"
            "// Never throws: all errors are reported via run_result.\n"
            "inline std::shared_ptr<run_result> run_script(\n"
            "    const std::shared_ptr<context> &ctx,\n"
            "    const std::string &entry,\n"
            "    const std::vector<value> &args = std::vector<value>()) {\n"
            "    return ctx->_run(entry, args);\n"
            "}")
        L.append("")
        L.append(f"}} // namespace {self.ns}")
        return "\n".join(L) + "\n"

    def _value_class(self) -> str:
        return """// Generic value: used for run() arguments and results.
class value {
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
};"""

    def _function_root_class(self) -> str:
        return """// Common root of all generated global functions: lets the client
// register implementations without knowing their concrete types.
class s5run_function {
public:
    virtual ~s5run_function() {}
    virtual std::string s5run_function_name() const = 0;
};"""

    def _remote_server_info_class(self) -> str:
        return """// Settings of a remote s5run server.
class remote_server_info {
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
};"""

    def _root_class(self) -> str:
        return """// Common root of all generated classes: lets the client work with
// objects without knowing their concrete types.
class s5run_object {
public:
    virtual ~s5run_object() {}
    virtual std::string s5run_class_name() const = 0;
    virtual std::vector<std::string> s5run_method_names() const = 0;
};"""

    def _base_class(self, c) -> str:
        L = [f"class {c.name()} : public s5run_object {{",
             "public:",
             '    std::string s5run_class_name() const override { return "' + c.name() + '"; }',
             "    std::vector<std::string> s5run_method_names() const override {",
             "        std::vector<std::string> out;"]
        for m in c.methods():
            L.append('        out.push_back("' + m.name() + '");')
        L += [
             "        return out;",
             "    }",
             f"    virtual ~{c.name()}() {{}}"]
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
            '    std::string s5run_function_name() const override { return "' + f.name() + '"; }',
            f"    virtual ~GlobalFunction_{f.name()}() {{}}",
            f"    virtual {ret} execute({args}) = 0;",
            "};",
        ])

    def _run_result_class(self) -> str:
        return """// Result of a script run: either a value or an error with details.
class run_result {
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
};"""

    def _client_class(self) -> str:
        s = self.schema
        L = ["class context {",
             "public:",
             "    context();",
             "    ~context();",
             "",
             "    // Version of the generated client code.",
             "    static const char *client_version() { return \"" + self.schema.client_version + "\"; }",
             "",
             "    // Address format: s5run://host:port",
             "    // Adds a server the context talks to. The first added server",
             "    // is used for script execution; the api_key is reserved for",
             "    // future authentication.",
             "    void add_server(const std::shared_ptr<remote_server_info> &server);",
             "",
             "    // Registers a remote context provider: the listed context",
             "    // names (global objects / classes) will be supplied by the",
             "    // s5run server at the given address. Stub, not implemented yet.",
             "    void add_context_provider(const std::string &address,",
             "                              const std::string &api_key,",
             "                              const std::vector<std::string> &contexts);",
             "",
             "    // Registers the object type on the server (class name + methods).",
             "    // Pass any instance of a generated base class; only its type",
             "    // information is used, the instance itself is not stored.",
             "    void add_class(const std::shared_ptr<s5run_object> &sample);",
             "",
             "    // Registers a singleton object, visible in scripts as",
             "    // `name.method(...)`. The raw pointer overload takes ownership.",
             "    void add_object(s5run_object *obj);",
             "    void add_object(const std::shared_ptr<s5run_object> &obj);",
             "",
             "    // Registers a global function implementation.",
             "    void add_function(const std::shared_ptr<s5run_function> &fn);",
             "",
             "    // Sets the script executed by run_script().",
             "    void set_script(const std::string &script);",
             "",
             "    class impl; // public forward declaration: run_result befriends it",
             "",
             "private:",
             "    context(const context &);",
             "    context &operator=(const context &);",
             "    std::shared_ptr<run_result> _run(const std::string &entry, const std::vector<value> &args);",
             "    friend std::shared_ptr<run_result> run_script(const std::shared_ptr<context> &,",
             "                                                const std::string &, const std::vector<value> &);",
             "    impl *m_impl;",
             "};"]
        return "\n".join(L)

    # ------------------------------------------------------------------
    # implementation
    # ------------------------------------------------------------------

    def generate_impl(self) -> str:
        L = ["// AUTO-GENERATED. DO NOT EDIT.",
             f'#include "{self.ns}.h"',
             "",
             "#include <arpa/inet.h>",
             "#include <netinet/in.h>",
             "#include <sys/socket.h>",
             "#include <sys/time.h>",
             "#include <unistd.h>",
             "",
             "#include <cstdio>",
             "#include <cerrno>",
             "#include <cstdlib>",
             "#include <ctime>",
             "#include <cstring>",
             "#include <functional>",
             "#include <stdexcept>",
             "",
             f"namespace {self.ns} {{",
             "",
             "namespace {",
             "",
             self._codec_text(),
             ""]
        if self.usage.ref_reads:
            L.append(
                "int32_t read_ref(Reader &r) {\n"
                "    if (r.get_u8() != T_REF)\n"
                "        throw std::runtime_error(\"wire: expected an object reference\");\n"
                "    int32_t this_id = r.get_i32();\n"
                "    r.get_string(); // skip the type name\n"
                "    return this_id;\n"
                "}\n\n")
        for name in self.usage.prim_reads:
            L.append(self._prim_read(name))
        for name in self.usage.prim_writes:
            L.append(self._prim_write(name))
        for type_str in self.usage.array_reads:
            L.append(self._array_read(type_str))
        for type_str in self.usage.array_writes:
            L.append(self._array_write(type_str))
        L += [self._value_codec_text(),
              ""]
        if any(_has_convert(f) for f in self.schema.functions) or \
           any(_has_convert(m) for c in self.schema.classes for m in c.methods()):
            L += [
                "std::string _value_to_string(const value &v) {",
                "    switch (v.get_kind()) {",
                "    case value::K_NULL: return \"null\";",
                "    case value::K_BOOL: return v.as_bool() ? \"true\" : \"false\";",
                "    case value::K_INT: return std::to_string(v.as_int());",
                "    case value::K_DOUBLE: return std::to_string(v.as_double());",
                "    case value::K_STRING: return v.as_string();",
                "    default:",
                "        throw std::runtime_error(\"wire: expected a scalar argument\");",
                "    }",
                "}",
                "",
            ]
        L += ["} // namespace",
              "",
              self._impl_class(),
              "",
              "// ---------------------------------------------------------------------------",
              "// context",
              "// ---------------------------------------------------------------------------",
              "",
              "context::context() : m_impl(new context::impl()) {",
              "}",
              "",
              "context::~context() {",
              "    delete m_impl;",
              "}",
              "",
              "void remote_server_info::set_address(const std::string &address) {",
              '    const std::string prefix = "s5run://";',
              "    if (address.compare(0, prefix.size(), prefix) != 0)",
              '        throw std::runtime_error("set_address: expected format s5run://host:port");',
              "    std::string rest = address.substr(prefix.size());",
              "    size_t colon = rest.rfind(':');",
              "    if (colon == std::string::npos || colon == 0 || colon + 1 >= rest.size())",
              '        throw std::runtime_error("set_address: expected format s5run://host:port");',
              "    m_address = address;",
              "    m_host = rest.substr(0, colon);",
              "    m_port = (uint16_t)atoi(rest.substr(colon + 1).c_str());",
              "    m_have_address = true;",
              "}",
              "",
              "void context::add_server(const std::shared_ptr<remote_server_info> &server) {",
              "    m_impl->add_server(server);",
              "}",
              "",
              "void context::add_context_provider(const std::string &address,",
              "                                  const std::string &api_key,",
              "                                  const std::vector<std::string> &contexts) {",
              "    (void)api_key;",
              '    const std::string prefix = "s5run://";',
              "    if (address.compare(0, prefix.size(), prefix) != 0)",
              '        throw std::runtime_error("add_context_provider: expected format s5run://host:port");',
              "    if (contexts.empty())",
              '        throw std::runtime_error("add_context_provider: contexts list is empty");',
              '    throw std::runtime_error("add_context_provider: not implemented yet");',
              "}",
              ""]
        L += ["void context::add_object(s5run_object *obj) {",
              "    m_impl->add_object(std::shared_ptr<s5run_object>(obj));",
              "}",
              "",
              "void context::add_object(const std::shared_ptr<s5run_object> &obj) {",
              "    m_impl->add_object(obj);",
              "}",
              "",
              "void context::add_class(const std::shared_ptr<s5run_object> &sample) {",
              "    m_impl->register_type(sample->s5run_class_name(),",
              "                          sample->s5run_method_names());",
              "}",
              ""]
        L += ["void context::add_function(const std::shared_ptr<s5run_function> &fn) {",
              "    m_impl->add_function(fn);",
              "}",
              ""]
        L += ["void context::set_script(const std::string &script) {",
              "    m_impl->set_script(script);",
              "}",
              "",
              "std::shared_ptr<run_result> context::_run(const std::string &entry,",
              "                                      const std::vector<value> &args) {",
              "    return m_impl->run(entry, args);",
              "}",
              "",
              "std::shared_ptr<context> make_context() {",
              "    return std::shared_ptr<context>(new context());",
              "}",
              "",
              f"}} // namespace {self.ns}",
              ""]
        return "\n".join(L)

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
                    "    uint8_t tag = r.get_u8();\n"
                    "    if (tag == T_FLOAT32)\n"
                    "        return (double)r.get_f32();\n"
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
        return ("uint8_t read_byte(Reader &r) {\n"
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
        return ("void write_byte(Writer &w, uint8_t v) {\n"
                "    w.put_u8(T_UINT8);\n"
                "    w.put_u8(v);\n"
                "}\n\n")

    def _array_read(self, type_str) -> str:
        ref = s5run_parse_type(type_str)
        inner = s5run_parse_type(ref.inner())
        sfx = self.usage.suffix(type_str)
        inner_cpp = type_to_cpp(inner)
        read_inner = self._read_expr(inner)
        return "\n".join([
            f"inline std::vector<{inner_cpp}> read_{sfx}(Reader &r) {{",
            "    if (r.get_u8() != T_ARRAY)",
            "        throw std::runtime_error(\"wire: expected array\");",
            "    uint32_t n = r.get_u32();",
            f"    std::vector<{inner_cpp}> out;",
            "    out.reserve(n);",
            "    for (uint32_t i = 0; i < n; ++i)",
            f"        out.push_back({read_inner});",
            "    return out;",
            "}",
            "",
        ])

    def _array_write(self, type_str) -> str:
        ref = s5run_parse_type(type_str)
        inner = s5run_parse_type(ref.inner())
        sfx = self.usage.suffix(type_str)
        inner_cpp = type_to_cpp(inner)
        write_inner = self._write_stmt(inner, "v[i]").rstrip(";")
        return "\n".join([
            f"inline void write_{sfx}(Writer &w, const std::vector<{inner_cpp}> &v) {{",
            "    w.put_u8(T_ARRAY);",
            "    w.put_u32((uint32_t)v.size());",
            "    for (size_t i = 0; i < v.size(); ++i)",
            f"        {write_inner};",
            "}",
            "",
        ])

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
}"""

    def _codec_text(self) -> str:
        return """// ---- binary wire codec (matches the s5run server protocol) ----

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

"""

    # ------------------------------------------------------------------
    # context::impl
    # ------------------------------------------------------------------

    def _impl_class(self) -> str:
        s = self.schema
        L = ["// ---------------------------------------------------------------------------",
             "// context::impl",
             "// ---------------------------------------------------------------------------",
             "",
             "class context::impl {",
             "public:",
             "    typedef std::function<void(int32_t, Reader &)> handler_t;",
             "",
             "    impl() : m_fd(-1), m_next_this_id(0) {",
             "        _register_method_handlers();",
             "    }",
             "",
             "    ~impl() {",
             "        _disconnect();",
             "    }",
             "",
             "    void add_server(const std::shared_ptr<remote_server_info> &server);",
             "    void set_script(const std::string &script);",
             "    void register_type(const std::string &name,",
             "                       const std::vector<std::string> &methods);"]
        L.append("    void add_object(const std::shared_ptr<s5run_object> &obj);")
        L.append("    void add_function(const std::shared_ptr<s5run_function> &fn);")
        L += ["    std::shared_ptr<run_result> run(const std::string &entry,",
              "                                 const std::vector<value> &args);",
              "",
              "private:",
              "    struct PendingType {",
              "        std::string name;",
              "        std::vector<std::string> methods;",
              "    };",
              "    struct PendingObject {",
              "        std::string name;",
              "        int32_t this_id;",
              "        std::string type;",
              "        std::vector<std::string> methods;",
              "    };",
              "    struct PendingFunction {",
              "        std::string name;",
              "    };",
              ""]
        for f in s.functions:
            L.append(f"    void _cb_{f.name()}(int32_t cb_id, Reader &r);")
        for c in s.classes:
            for m in c.methods():
                L.append(f"    void _cbm_{c.name()}_{m.name()}(int32_t cb_id, Reader &r);")
        for c in s.classes:
            L.append(f"    int32_t _register_instance_{c.name()}(const std::shared_ptr<{c.name()}> &obj);")
        L += ["    void _register_method_handlers();",
              "    void _connect();",
              "    void _disconnect();",
              "    void _flush_pending();",
              "    static void _read_hello(int fd);",
              "    static bool _send_message_fd(int fd, uint8_t msg_type, const std::vector<uint8_t> &body);",
              "    static bool _read_message_fd(int fd, uint8_t &msg_type, std::vector<uint8_t> &body);",
              "    static bool _read_exact_fd(int fd, void *buf, size_t n);",
              "    static bool _write_all_fd(int fd, const void *buf, size_t n);",
              "    bool _send_message(uint8_t msg_type, const std::vector<uint8_t> &body);",
              "    bool _read_message(uint8_t &msg_type, std::vector<uint8_t> &body);",
              "    void _expect_register_resp(const std::string &what);",
              "    void _dispatch_callback(std::vector<uint8_t> &body);",
              "    static long _probe_server_load(const std::shared_ptr<remote_server_info> &server);",
              "    static int _parse_error_line(const std::string &error);",
              "",
              "    std::string m_script;",
              "    std::vector<std::shared_ptr<remote_server_info>> m_servers;",
              "    int m_fd;",
              "    int32_t m_next_this_id;",
              "    std::vector<PendingType> m_pending_types;",
              "    std::vector<PendingObject> m_pending_objects;",
              "    std::vector<PendingFunction> m_pending_functions;",
              "    std::map<std::string, handler_t> m_handlers;",
              "    typedef std::function<void(int32_t, const std::shared_ptr<s5run_object> &)> registrar_t;",
              "    std::map<std::string, registrar_t> m_instance_registrars;",
              "    typedef std::function<void(const std::shared_ptr<s5run_function> &)> fn_registrar_t;",
              "    std::map<std::string, fn_registrar_t> m_function_registrars;"]
        for f in s.functions:
            L.append(f"    std::shared_ptr<GlobalFunction_{f.name()}> m_fn_{f.name()};")
        for c in s.classes:
            L.append(f"    std::map<int32_t, std::shared_ptr<{c.name()}>> m_instances_{c.name()};")
        L += ["};",
              "",
              "// ---------------------------------------------------------------------------",
              "// context::impl - address / connection",
              "// ---------------------------------------------------------------------------",
              "",
              "void context::impl::set_script(const std::string &script) {",
              "    m_script = script;",
              "}",
              "",
              "void context::impl::add_server(const std::shared_ptr<remote_server_info> &server) {",
              "    if (!server)",
              '        throw std::runtime_error("add_server: server is null");',
              "    m_servers.push_back(server);",
              "}",
              "",
              "void context::impl::_connect() {",
              "    if (m_fd >= 0)",
              "        return;",
              "    if (m_servers.empty())",
              '        throw std::runtime_error("s5run: no server added (add_server)");',
              "    size_t best = 0;",
              "    if (m_servers.size() > 1) {",
              "        // Ask every server for its load and pick the least loaded one.",
              "        long best_load = -1;",
              "        for (size_t i = 0; i < m_servers.size(); ++i) {",
              "            long load = _probe_server_load(m_servers[i]);",
              "            if (load < 0)",
              "                continue; // unreachable",
              "            if (best_load < 0 || load < best_load) {",
              "                best_load = load;",
              "                best = i;",
              "            }",
              "        }",
              "        if (best_load < 0)",
              '            throw std::runtime_error("s5run: all servers are unreachable");',
              "    }",
              "    const std::string m_host = m_servers[best]->get_host();",
              "    const uint16_t m_port = m_servers[best]->get_port();",
              "    int fd = ::socket(AF_INET, SOCK_STREAM, 0);",
              "    if (fd < 0)",
              '        throw std::runtime_error("s5run: socket failed");',
              "    sockaddr_in addr;",
              "    std::memset(&addr, 0, sizeof(addr));",
              "    addr.sin_family = AF_INET;",
              "    addr.sin_port = htons(m_port);",
              "    if (::inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr) != 1) {",
              "        ::close(fd);",
              '        throw std::runtime_error("s5run: invalid host (only IPv4 supported): " + m_host);',
              "    }",
              "    if (::connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {",
              "        ::close(fd);",
              '        throw std::runtime_error("s5run: connect failed");',
              "    }",
              "    timeval tv;",
              "    tv.tv_sec = 10;",
              "    tv.tv_usec = 0;",
              "    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));",
              "    m_fd = fd;",
              "    _read_hello(m_fd);",
              "}",
              "",
              "void context::impl::_read_hello(int fd) {",
              "    uint8_t msg_type = 0;",
              "    std::vector<uint8_t> body;",
              "    if (!_read_message_fd(fd, msg_type, body) || msg_type != M_SERVER_HELLO)",
              '        throw std::runtime_error("s5run: expected server hello");',
              "    std::string text(body.begin(), body.end());",
              '    if (text.compare(0, 6, "s5run ") != 0)',
              '        throw std::runtime_error("s5run: bad server hello: " + text);',
              "    size_t last_space = text.rfind(' ');",
              "    if (last_space == std::string::npos)",
              '        throw std::runtime_error("s5run: bad server hello: " + text);',
              "    int protocol = atoi(text.substr(last_space + 1).c_str());",
              "    if (protocol != 2)",
              '        throw std::runtime_error("s5run: server protocol version " + std::to_string(protocol)'
              ' + " is not supported (client supports 2)");',
              "}",
              "",
              "void context::impl::_disconnect() {",
              "    if (m_fd >= 0) {",
              "        ::close(m_fd);",
              "        m_fd = -1;",
              "    }",
              "}",
              "",
              "bool context::impl::_read_exact_fd(int fd, void *buf, size_t n) {",
              "    char *p = (char *)buf;",
              "    while (n > 0) {",
              "        ssize_t got = ::recv(fd, p, n, 0);",
              "        if (got == 0)",
              "            return false;",
              "        if (got < 0) {",
              "            if (errno == EINTR)",
              "                continue;",
              "            return false;",
              "        }",
              "        p += got;",
              "        n -= (size_t)got;",
              "    }",
              "    return true;",
              "}",
              "",
              "bool context::impl::_write_all_fd(int fd, const void *buf, size_t n) {",
              "    const char *p = (const char *)buf;",
              "    while (n > 0) {",
              "        ssize_t sent = ::send(fd, p, n, MSG_NOSIGNAL);",
              "        if (sent <= 0) {",
              "            if (sent < 0 && errno == EINTR)",
              "                continue;",
              "            return false;",
              "        }",
              "        p += sent;",
              "        n -= (size_t)sent;",
              "    }",
              "    return true;",
              "}",
              "",
              "bool context::impl::_send_message_fd(int fd, uint8_t msg_type, const std::vector<uint8_t> &body) {",
              "    uint32_t len = (uint32_t)(1 + body.size());",
              "    uint32_t net_len = htonl(len);",
              "    if (!_write_all_fd(fd, &net_len, sizeof(net_len)))",
              "        return false;",
              "    if (!_write_all_fd(fd, &msg_type, 1))",
              "        return false;",
              "    return body.empty() ? true : _write_all_fd(fd, &body[0], body.size());",
              "}",
              "",
              "bool context::impl::_send_message(uint8_t msg_type, const std::vector<uint8_t> &body) {",
              "    return _send_message_fd(m_fd, msg_type, body);",
              "}",
              "",
              "bool context::impl::_read_message_fd(int fd, uint8_t &msg_type, std::vector<uint8_t> &body) {",
              "    uint32_t net_len = 0;",
              "    if (!_read_exact_fd(fd, &net_len, sizeof(net_len)))",
              "        return false;",
              "    uint32_t len = ntohl(net_len);",
              "    if (len == 0 || len > 64 * 1024 * 1024)",
              "        return false;",
              "    std::vector<uint8_t> frame(len);",
              "    if (!_read_exact_fd(fd, &frame[0], len))",
              "        return false;",
              "    msg_type = frame[0];",
              "    body.assign(frame.begin() + 1, frame.end());",
              "    return true;",
              "}",
              "",
              "bool context::impl::_read_message(uint8_t &msg_type, std::vector<uint8_t> &body) {",
              "    return _read_message_fd(m_fd, msg_type, body);",
              "}",
              "",
              "void context::impl::_expect_register_resp(const std::string &what) {",
              "    uint8_t msg_type = 0;",
              "    std::vector<uint8_t> body;",
              "    if (!_read_message(msg_type, body) || msg_type != M_REGISTER_RESP)",
              '        throw std::runtime_error("s5run: " + what + " failed");',
              "    Reader r(&body[0], body.size());",
              "    if (r.get_u8() == 0)",
              "        throw std::runtime_error(\"s5run: \" + what + \" error: \" + r.get_string());",
              "}",
              "",
              "void context::impl::register_type(const std::string &name,",
              "                                 const std::vector<std::string> &methods) {",
              "    if (m_fd >= 0) {",
              "        Writer w;",
              "        w.put_string(name);",
              "        w.put_u32((uint32_t)methods.size());",
              "        for (size_t i = 0; i < methods.size(); ++i)",
              "            w.put_string(methods[i]);",
              "        _send_message(M_REGISTER_TYPE, w.data());",
              '        _expect_register_resp("register_type(" + name + ")");',
              "        return;",
              "    }",
              "    PendingType t;",
              "    t.name = name;",
              "    t.methods = methods;",
              "    m_pending_types.push_back(t);",
              "}",
              ""]
        # add_object (generic, via instance registrars)
        L += ["void context::impl::add_object(const std::shared_ptr<s5run_object> &obj) {",
              "    const std::string class_name = obj->s5run_class_name();",
              "    std::map<std::string, registrar_t>::const_iterator rit =",
              "        m_instance_registrars.find(class_name);",
              "    if (rit == m_instance_registrars.end())",
              '        throw std::runtime_error("add_object: unknown class: " + class_name);',
              "    int32_t id = ++m_next_this_id;",
              "    rit->second(id, obj);",
              "    const std::vector<std::string> methods = obj->s5run_method_names();",
              "    if (m_fd >= 0) {",
              "        Writer w;",
              "        w.put_string(class_name);",
              "        w.put_i32(id);",
              "        w.put_string(class_name);",
              "        w.put_u32((uint32_t)methods.size());",
              "        for (size_t i = 0; i < methods.size(); ++i)",
              "            w.put_string(methods[i]);",
              "        _send_message(M_REGISTER_OBJ, w.data());",
              '        _expect_register_resp("register_object(" + class_name + ")");',
              "        return;",
              "    }",
              "    PendingObject o;",
              "    o.name = class_name;",
              "    o.this_id = id;",
              "    o.type = class_name;",
              "    o.methods = methods;",
              "    m_pending_objects.push_back(o);",
              "}",
              ""]
        # add_function (generic, via function registrars)
        L += ["void context::impl::add_function(const std::shared_ptr<s5run_function> &fn) {",
              "    const std::string name = fn->s5run_function_name();",
              "    std::map<std::string, fn_registrar_t>::const_iterator rit =",
              "        m_function_registrars.find(name);",
              "    if (rit == m_function_registrars.end())",
              '        throw std::runtime_error("add_function: unknown function: " + name);',
              "    rit->second(fn);",
              "    if (m_fd >= 0) {",
              "        Writer w;",
              "        w.put_string(name);",
              "        _send_message(M_REGISTER_FUNC, w.data());",
              '        _expect_register_resp("register_function(" + name + ")");',
              "        return;",
              "    }",
              "    PendingFunction pf;",
              "    pf.name = name;",
              "    m_pending_functions.push_back(pf);",
              "}",
              ""]
        # _cb_<Function> handlers
        for f in s.functions:
            nargs = len(f.arguments())
            L += [f"void context::impl::_cb_{f.name()}(int32_t cb_id, Reader &r) {{",
                  "    uint32_t n = r.get_u32();"]
            if not _has_convert(f):
                L += [
                    "    if (n != %d) {" % nargs,
                    f"        throw std::runtime_error(\"{f.name()}: expected %d argument(s)\");" % nargs,
                    "    }",
                ]
            L += self._decode_args(f)
            call_args = ", ".join(f"a_{a.name()}" for a in f.arguments())
            if _is_void(f):
                L.append(f"    m_fn_{f.name()}->execute({call_args});")
            else:
                L.append(f"    {type_to_cpp(f.return_ref())} res = m_fn_{f.name()}->execute({call_args});")
            L += ["    Writer w;",
                  "    w.put_i32(cb_id);",
                  "    w.put_u8(1);"]
            if not _is_void(f):
                L += self._encode_result(f, "res")
            else:
                L.append("    w.put_u8(T_UNDEFINED);")
            L += ["    _send_message(M_CALLBACK_RESP, w.data());",
                  "}",
                  ""]
        # _cbm_<Class>_<Method> handlers
        for c in s.classes:
            for m in c.methods():
                conv = _has_convert(m)
                if conv:
                    check = [
                        "    if (n < 1) {",
                        f"        throw std::runtime_error(\"{c.name()}.{m.name()}: expected this_id\");",
                        "    }",
                    ]
                else:
                    m_nargs = 1 + len(m.arguments())
                    check = [
                        "    if (n != %d) {" % m_nargs,
                        ("        throw std::runtime_error(\"" + c.name() + "." + m.name()
                         + ": expected %d argument(s)\");") % m_nargs,
                        "    }",
                    ]
                L += [f"void context::impl::_cbm_{c.name()}_{m.name()}(int32_t cb_id, Reader &r) {{",
                      "    uint32_t n = r.get_u32();"]
                L += check
                L += [
                      "    int32_t this_id = (int32_t)read_int(r);",
                      f"    std::map<int32_t, std::shared_ptr<{c.name()}>>::const_iterator it =",
                      f"        m_instances_{c.name()}.find(this_id);",
                      f"    if (it == m_instances_{c.name()}.end()) {{",
                      f"        throw std::runtime_error(\"{c.name()}: unknown this_id\");",
                      "    }"]
                if conv:
                    L.append("    n -= 1;")
                L += self._decode_args(m)
                call_args = ", ".join(f"a_{a.name()}" for a in m.arguments())
                if _is_void(m):
                    L.append(f"    it->second->{m.name()}({call_args});")
                else:
                    L.append(f"    {type_to_cpp(m.return_ref())} res = it->second->{m.name()}({call_args});")
                L += ["    Writer w;",
                      "    w.put_i32(cb_id);",
                      "    w.put_u8(1);"]
                if not _is_void(m):
                    L += self._encode_result(m, "res")
                else:
                    L.append("    w.put_u8(T_UNDEFINED);")
                L += ["    _send_message(M_CALLBACK_RESP, w.data());",
                      "}",
                      ""]
        # _register_instance_<Class>
        for c in s.classes:
            L += [f"int32_t context::impl::_register_instance_{c.name()}(const std::shared_ptr<{c.name()}> &obj) {{",
                  "    int32_t id = ++m_next_this_id;",
                  f"    m_instances_{c.name()}[id] = obj;",
                  "    return id;",
                  "}",
                  ""]
        # _register_method_handlers
        L += ["void context::impl::_register_method_handlers() {"]
        for c in s.classes:
            L += [
                '    m_instance_registrars["' + c.name() + '"] = '
                "[this](int32_t id, const std::shared_ptr<s5run_object> &obj) {",
                f"        m_instances_{c.name()}[id] = std::static_pointer_cast<{c.name()}>(obj);",
                "    };",
            ]
        for f in s.functions:
            L += [
                '    m_function_registrars["' + f.name() + '"] = '
                "[this](const std::shared_ptr<s5run_function> &fn) {",
                f"        m_fn_{f.name()} = std::static_pointer_cast<GlobalFunction_{f.name()}>(fn);",
                '        m_handlers["' + f.name() + '"] = [this](int32_t cb_id, Reader &r) {',
                f"            _cb_{f.name()}(cb_id, r);",
                "        };",
                "    };",
            ]
        for c in s.classes:
            for m in c.methods():
                L += ["    m_handlers[\"" + m.name() + "\"] = [this](int32_t cb_id, Reader &r) {",
                      f"        _cbm_{c.name()}_{m.name()}(cb_id, r);",
                      "    };"]
        L += ["}",
              ""]
        # _flush_pending
        L += ["void context::impl::_flush_pending() {",
              "    for (size_t i = 0; i < m_pending_types.size(); ++i) {",
              "        const PendingType &t = m_pending_types[i];",
              "        Writer w;",
              "        w.put_string(t.name);",
              "        w.put_u32((uint32_t)t.methods.size());",
              "        for (size_t j = 0; j < t.methods.size(); ++j)",
              "            w.put_string(t.methods[j]);",
              "        _send_message(M_REGISTER_TYPE, w.data());",
              "        _expect_register_resp(\"register_type(\" + t.name + \")\");",
              "    }",
              "    m_pending_types.clear();",
              "    for (size_t i = 0; i < m_pending_objects.size(); ++i) {",
              "        const PendingObject &o = m_pending_objects[i];",
              "        Writer w;",
              "        w.put_string(o.name);",
              "        w.put_i32(o.this_id);",
              "        w.put_string(o.type);",
              "        w.put_u32((uint32_t)o.methods.size());",
              "        for (size_t j = 0; j < o.methods.size(); ++j)",
              "            w.put_string(o.methods[j]);",
              "        _send_message(M_REGISTER_OBJ, w.data());",
              "        _expect_register_resp(\"register_object(\" + o.name + \")\");",
              "    }",
              "    m_pending_objects.clear();",
              "    for (size_t i = 0; i < m_pending_functions.size(); ++i) {",
              "        const PendingFunction &pf = m_pending_functions[i];",
              "        Writer w;",
              "        w.put_string(pf.name);",
              "        _send_message(M_REGISTER_FUNC, w.data());",
              "        _expect_register_resp(\"register_function(\" + pf.name + \")\");",
              "    }",
              "    m_pending_functions.clear();",
              "}",
              ""]
        # _dispatch_callback
        L += ["void context::impl::_dispatch_callback(std::vector<uint8_t> &body) {",
              "    Reader r(&body[0], body.size());",
              "    int32_t cb_id = r.get_i32();",
              "    std::string method = r.get_string();",
              "    std::map<std::string, handler_t>::const_iterator it = m_handlers.find(method);",
              "    if (it == m_handlers.end()) {",
              "        Writer w;",
              "        w.put_i32(cb_id);",
              "        w.put_u8(0);",
              "        w.put_string(\"unknown method: \" + method);",
              "        _send_message(M_CALLBACK_RESP, w.data());",
              "        return;",
              "    }",
              "    try {",
              "        it->second(cb_id, r);",
              "    } catch (const std::exception &e) {",
              "        Writer w;",
              "        w.put_i32(cb_id);",
              "        w.put_u8(0);",
              "        w.put_string(e.what());",
              "        _send_message(M_CALLBACK_RESP, w.data());",
              "    }",
              "}",
              ""]
        # _probe_server_load
        L += ["""long context::impl::_probe_server_load(const std::shared_ptr<remote_server_info> &server) {
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
"""]
        # _parse_error_line
        L += ["""int context::impl::_parse_error_line(const std::string &error) {
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
"""]
        # run
        L += ["std::shared_ptr<run_result> context::impl::run(const std::string &entry,",
              "                                 const std::vector<value> &args) {",
              "    std::shared_ptr<run_result> out(new run_result());",
              "    clock_t started = clock();",
              "    try {",
              "        _connect();",
              "        _flush_pending();",
              "        Writer w;",
              "        w.put_string(m_script);",
              "        w.put_string(entry);",
              "        w.put_u32((uint32_t)args.size());",
              "        for (size_t i = 0; i < args.size(); ++i)",
              "            encode_value(w, args[i]);",
              "        if (!_send_message(M_EXEC_REQ, w.data()))",
              '            throw std::runtime_error("s5run: send failed");',
              "        while (true) {",
              "            uint8_t msg_type = 0;",
              "            std::vector<uint8_t> body;",
              "            if (!_read_message(msg_type, body))",
              '                throw std::runtime_error("s5run: connection closed");',
              "            if (msg_type == M_CALLBACK_REQ) {",
              "                _dispatch_callback(body);",
              "                continue;",
              "            }",
              "            if (msg_type == M_EXEC_RESP) {",
              "                Reader r(&body[0], body.size());",
              "                if (r.get_u8() == 0)",
              "                    throw std::runtime_error(r.get_string());",
              "                out->m_value = decode_value(r);",
              "                break;",
              "            }",
              '            throw std::runtime_error("s5run: unexpected message type");',
              "        }",
              "    } catch (const std::exception &e) {",
              "        out->m_has_error = true;",
              "        out->m_error = e.what();",
              "        out->m_error_line = _parse_error_line(out->m_error);",
              "    }",
              "    out->m_duration_ms = (int64_t)((double)(clock() - started) * 1000.0 / CLOCKS_PER_SEC);",
              "    return out;",
              "}"]
        return "\n".join(L)


def generate_header(schema) -> str:
    return _Cpp11Generator(schema).generate_header()


def generate_impl(schema) -> str:
    return _Cpp11Generator(schema).generate_impl()
