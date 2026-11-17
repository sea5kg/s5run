"""C++ code generator.

Produces two strings: header (.h) and implementation (.cpp).
No global mutable state — pure functions over Schema.
"""

from .types import parse


def _encode_lines(ref, expr: str, indent: str) -> list[str]:
    if ref.is_prim():
        fn = {"bool": "put_bool", "u8": "put_u8",
              "i64": "put_u64", "u64": "put_u64",
              "f64": "put_f64", "str": "put_str"}[ref.wire()]
        return [f"{indent}wire::{fn}(b, {expr});"]
    if ref.is_array():
        inner = parse(ref.inner)
        return [
            f"{indent}wire::put_u32(b, (uint32_t){expr}.size());",
            f"{indent}for (const auto& _e : {expr}) {{",
            *_encode_lines(inner, "_e", indent + "    "),
            f"{indent}}}",
        ]
    if ref.is_iface():
        return [f"{indent}wire::put_u64(b, {expr} ? {expr}->this_id : 0);"]
    raise ValueError(f"cannot encode {ref.name}")


def _decode_lines(ref, target: str, indent: str) -> list[str]:
    if ref.is_prim():
        fn = {"bool": "get_bool", "u8": "get_u8",
              "i64": "get_u64", "u64": "get_u64",
              "f64": "get_f64", "str": "get_str"}[ref.wire()]
        return [f"{indent}{target} = wire::{fn}(p);"]
    if ref.is_array():
        inner = parse(ref.inner)
        elem_ty = inner.cpp()
        return [
            f"{indent}{{",
            f"{indent}    uint32_t _n = wire::get_u32(p);",
            f"{indent}    {target}.reserve(_n);",
            f"{indent}    for (uint32_t _i = 0; _i < _n; ++_i) {{",
            f"{indent}        {elem_ty} _e{{}};",
            *_decode_lines(inner, "_e", indent + "        "),
            f"{indent}        {target}.push_back(std::move(_e));",
            f"{indent}    }}",
            f"{indent}}}",
        ]
    if ref.is_iface():
        return [f"{indent}(void)wire::get_u64(p); // iface ref: fill externally"]
    raise ValueError(f"cannot decode {ref.name}")


def _emit_struct_impl(out: list, struct_name: str, fields, prefix_id: bool) -> None:
    out.append(f"std::vector<uint8_t> {struct_name}::encode() const {{")
    out.append("    std::vector<uint8_t> b;")
    if prefix_id:
        out.append("    wire::put_u64(b, this_id);")
    for a in fields:
        out += _encode_lines(parse(a.type), f"this->{a.name}", "    ")
    out.append("    return b;")
    out.append("}")
    out.append("")
    out.append(f"{struct_name} {struct_name}::decode(const std::vector<uint8_t>& b) {{")
    out.append(f"    {struct_name} r;")
    out.append("    const uint8_t* p = b.data();")
    if prefix_id:
        out.append("    r.this_id = wire::get_u64(p);")
    for a in fields:
        out += _decode_lines(parse(a.type), f"r.{a.name}", "    ")
    out.append("    return r;")
    out.append("}")
    out.append("")


def generate_header(schema) -> str:
    ns = schema.namespace
    L = ["// AUTO-GENERATED. DO NOT EDIT.", "#pragma once",
         "#include <cstdint>", "#include <memory>",
         "#include <string>", "#include <vector>",
         "", f"namespace {ns} {{", ""]

    for c in schema.classes:
        L.append(f"class {c.name};")
        L.append(f"class {c.aggregate_interface};")
    L.append("class IRunScriptGlobalFunctions;")
    L.append("")

    for c in schema.classes:
        L.append(f"class {c.aggregate_interface} {{")
        L.append("public:")
        L.append(f"    virtual ~{c.aggregate_interface}() = default;")
        for m in c.methods:
            ret = m.return_ref.cpp()
            args = ", ".join(f"{a.type_ref().cpp()} {a.name}" for a in m.arguments)
            L.append(f"    virtual {ret} {m.name}({args}) = 0;")
        L.append("")
        L.append("    uint64_t this_id = 0;")
        L.append("};")
        L.append("")

    L.append("class IRunScriptGlobalFunctions {")
    L.append("public:")
    L.append("    virtual ~IRunScriptGlobalFunctions() = default;")
    for f in schema.functions:
        ret = f.return_ref.cpp()
        args = ", ".join(f"{a.type_ref().cpp()} {a.name}" for a in f.arguments)
        L.append(f"    virtual {ret} {f.name}({args}) = 0;")
    L.append("};")
    L.append("")

    for f in schema.functions:
        L.append(f"struct {f.name}Request {{")
        for a in f.arguments:
            L.append(f"    {a.type_ref().cpp()} {a.name};")
        L.append("    std::vector<uint8_t> encode() const;")
        L.append(f"    static {f.name}Request decode(const std::vector<uint8_t>&);")
        L.append("};")
        L.append("")
        if not f.return_ref.is_iface():
            L.append(f"struct {f.name}Response {{")
            L.append(f"    {f.return_ref.cpp()} value;")
            L.append("    std::vector<uint8_t> encode() const;")
            L.append(f"    static {f.name}Response decode(const std::vector<uint8_t>&);")
            L.append("};")
            L.append("")

    for c in schema.classes:
        for m in c.methods:
            L.append(f"struct {m.name}Request {{")
            L.append("    uint64_t this_id = 0;")
            for a in m.arguments:
                L.append(f"    {a.type_ref().cpp()} {a.name};")
            L.append("    std::vector<uint8_t> encode() const;")
            L.append(f"    static {m.name}Request decode(const std::vector<uint8_t>&);")
            L.append("};")
            L.append("")
            if not m.return_ref.is_iface():
                L.append(f"struct {m.name}Response {{")
                L.append(f"    {m.return_ref.cpp()} value;")
                L.append("    std::vector<uint8_t> encode() const;")
                L.append(f"    static {m.name}Response decode(const std::vector<uint8_t>&);")
                L.append("};")
                L.append("")

    L.append(f"}} // namespace {ns}")
    return "\n".join(L)


def generate_impl(schema) -> str:
    ns = schema.namespace
    L = ["// AUTO-GENERATED. DO NOT EDIT.",
         f'#include "{ns}.h"',
         "#include <cstring>",
         "",
         f"namespace {ns} {{",
         "",
         "namespace wire {",
         "void put_u8(std::vector<uint8_t>& b, uint8_t v)  { b.push_back(v); }",
         "void put_bool(std::vector<uint8_t>& b, bool v)    { b.push_back(v ? 1 : 0); }",
         "void put_u32(std::vector<uint8_t>& b, uint32_t v) {",
         "    for (int i = 0; i < 4; ++i) b.push_back((v >> (8*i)) & 0xFF);",
         "}",
         "void put_u64(std::vector<uint8_t>& b, uint64_t v) {",
         "    for (int i = 0; i < 8; ++i) b.push_back((v >> (8*i)) & 0xFF);",
         "}",
         "void put_f64(std::vector<uint8_t>& b, double v) {",
         "    uint64_t u; std::memcpy(&u, &v, 8); put_u64(b, u);",
         "}",
         "void put_str(std::vector<uint8_t>& b, const std::string& s) {",
         "    put_u32(b, (uint32_t)s.size());",
         "    b.insert(b.end(), s.begin(), s.end());",
         "}",
         "",
         "uint8_t  get_u8 (const uint8_t*& p) { return *p++; }",
         "bool     get_bool(const uint8_t*& p) { return *p++ != 0; }",
         "uint32_t get_u32(const uint8_t*& p) {",
         "    uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= (uint32_t)p[i] << (8*i);",
         "    p += 4; return v;",
         "}",
         "uint64_t get_u64(const uint8_t*& p) {",
         "    uint64_t v = 0; for (int i = 0; i < 8; ++i) v |= (uint64_t)p[i] << (8*i);",
         "    p += 8; return v;",
         "}",
         "double   get_f64(const uint8_t*& p) {",
         "    uint64_t u = get_u64(p); double d; std::memcpy(&d, &u, 8); return d;",
         "}",
         "std::string get_str(const uint8_t*& p) {",
         "    uint32_t n = get_u32(p); std::string s((const char*)p, n); p += n; return s;",
         "}",
         "} // namespace wire",
         ""]

    for f in schema.functions:
        _emit_struct_impl(L, f"{f.name}Request", f.arguments, prefix_id=False)
        if not f.return_ref.is_iface():
            _emit_struct_impl(
                L, f"{f.name}Response",
                [_ValueField(f.return_ref.name)], prefix_id=False)

    for c in schema.classes:
        for m in c.methods:
            _emit_struct_impl(L, f"{m.name}Request", m.arguments, prefix_id=True)
            if not m.return_ref.is_iface():
                _emit_struct_impl(
                    L, f"{m.name}Response",
                    [_ValueField(m.return_ref.name)], prefix_id=False)

    L.append(f"}} // namespace {ns}")
    return "\n".join(L)


class _ValueField:
    """Adapter so _emit_struct_impl can accept a single 'value' field."""
    def __init__(self, type_name: str):
        self.name = "value"
        self.type = type_name
