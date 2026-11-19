#
# MIT License
#
#           ░░░░░░░
#          ░██▓▓▓▓▒
# ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
# ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
#  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
# ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
#  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
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

"""Java code generator. Writes one .java file per class/interface."""

import os

from .s5run_typeref import S5RunCliTypeRef, s5run_parse_type

# schema primitive name -> java type
PRIMITIVES = {
    "bool":   "boolean",
    "int":    "long",
    "uint":   "long",
    "float":  "double",
    "double": "double",
    "string": "String",
    "byte":   "byte",
}

_BOXED = {"long": "Long", "boolean": "Boolean", "double": "Double", "byte": "Byte"}


def type_to_java(ref: S5RunCliTypeRef) -> str:
    if ref.is_prim():
        return PRIMITIVES[ref.name()]
    if ref.is_array():
        inner = type_to_java(s5run_parse_type(ref.inner()))
        return f"List<{_BOXED.get(inner, inner)}>"
    return ref.name()


def _encode_lines(ref, expr: str, indent: str) -> list[str]:
    if ref.is_prim():
        fn = {"bool": "putBool", "byte": "putU8",
              "int": "putU64", "uint": "putU64",
              "float": "putF64", "double": "putF64",
              "string": "putStr"}[ref.wire()]
        return [f"{indent}Wire.{fn}(b, {expr});"]
    if ref.is_array():
        inner = s5run_parse_type(ref.inner())
        return [
            f"{indent}Wire.putU32(b, {expr}.size());",
            f"{indent}for (var _e : {expr}) {{",
            *_encode_lines(inner, "_e", indent + "    "),
            f"{indent}}}",
        ]
    if ref.is_ref():
        return [f"{indent}Wire.putU64(b, {expr} != null ? {expr}.getThisId() : 0);"]
    raise ValueError(ref.name())


def _decode_lines(ref, target: str, indent: str) -> list[str]:
    if ref.is_prim():
        fn = {"bool": "getBool", "byte": "getU8",
              "int": "getU64", "uint": "getU64",
              "float": "getF64", "double": "getF64",
              "string": "getStr"}[ref.wire()]
        return [f"{indent}{target} = Wire.{fn}(b);"]
    if ref.is_array():
        inner = s5run_parse_type(ref.inner())
        inner_t = type_to_java(inner)
        unboxed = {"Long": "long", "Boolean": "boolean",
                   "Double": "double", "Byte": "byte"}.get(inner_t, inner_t)
        return [
            f"{indent}{{",
            f"{indent}    long _n = Wire.getU32(b);",
            f"{indent}    {target} = new ArrayList<>();",
            f"{indent}    for (long _i = 0; _i < _n; ++_i) {{",
            f"{indent}        {unboxed} _e;",
            *_decode_lines(inner, "_e", indent + "        "),
            f"{indent}        {target}.add(_e);",
            f"{indent}    }}",
            f"{indent}}}",
        ]
    if ref.is_ref():
        return [f"{indent}{target} = null; // ref in array: not supported yet",
                f"{indent}Wire.getU64(b);"]
    raise ValueError(ref.name())


def _write_struct(pkg_dir: str, pkg: str, name: str, fields, prefix_id: bool) -> None:
    L = ["// AUTO-GENERATED. DO NOT EDIT.", f"package {pkg};", "",
         "import java.util.*;", "import java.io.*;", "import java.nio.*;", ""]
    L.append(f"public class {name} {{")
    if prefix_id:
        L.append("    public long this_id;")
    for a in fields:
        L.append(f"    public {type_to_java(s5run_parse_type(a.type()))} {a.name()};")
    L.append("")
    L.append("    public byte[] encode() {")
    L.append("        ByteArrayOutputStream b = new ByteArrayOutputStream();")
    if prefix_id:
        L.append("        Wire.putU64(b, this_id);")
    for a in fields:
        L += _encode_lines(s5run_parse_type(a.type()), f"this.{a.name()}", "        ")
    L.append("        return b.toByteArray();")
    L.append("    }")
    L.append(f"    public static {name} decode(byte[] data) {{")
    L.append(f"        {name} r = new {name}();")
    L.append("        ByteBuffer b = ByteBuffer.wrap(data);")
    if prefix_id:
        L.append("        r.this_id = Wire.getU64(b);")
    for a in fields:
        L += _decode_lines(s5run_parse_type(a.type()), f"r.{a.name()}", "        ")
    L.append("        return r;")
    L.append("    }")
    L.append("}")
    with open(os.path.join(pkg_dir, f"{name}.java"), "w", encoding="utf-8") as f:
        f.write("\n".join(L))


def _write_wire(pkg_dir: str, pkg: str) -> None:
    L = ["// AUTO-GENERATED. DO NOT EDIT.", f"package {pkg};", "",
         "import java.io.*;", "import java.nio.*;",
         "import java.nio.charset.StandardCharsets;", "",
         "public final class Wire {",
         "    private Wire() {}",
         "    public static void putU8(ByteArrayOutputStream b, byte v) { b.write(v); }",
         "    public static void putBool(ByteArrayOutputStream b, boolean v) { b.write(v ? 1 : 0); }",
         "    public static void putU32(ByteArrayOutputStream b, long v) {",
         "        for (int i = 0; i < 4; ++i) b.write((int)((v >> (8*i)) & 0xFF));",
         "    }",
         "    public static void putU64(ByteArrayOutputStream b, long v) {",
         "        for (int i = 0; i < 8; ++i) b.write((int)((v >> (8*i)) & 0xFF));",
         "    }",
         "    public static void putF64(ByteArrayOutputStream b, double v) {",
         "        putU64(b, Double.doubleToRawLongBits(v));",
         "    }",
         "    public static void putStr(ByteArrayOutputStream b, String s) {",
         "        byte[] raw = s.getBytes(StandardCharsets.UTF_8);",
         "        putU32(b, raw.length); b.writeBytes(raw);",
         "    }",
         "    public static byte    getU8(ByteBuffer b)   { return b.get(); }",
         "    public static boolean getBool(ByteBuffer b) { return b.get() != 0; }",
         "    public static long    getU32(ByteBuffer b) {",
         "        long v = 0; for (int i = 0; i < 4; ++i) v |= (long)(b.get() & 0xFF) << (8*i);",
         "        return v;",
         "    }",
         "    public static long    getU64(ByteBuffer b) {",
         "        long v = 0; for (int i = 0; i < 8; ++i) v |= (long)(b.get() & 0xFF) << (8*i);",
         "        return v;",
         "    }",
         "    public static double  getF64(ByteBuffer b) {",
         "        return Double.longBitsToDouble(getU64(b));",
         "    }",
         "    public static String  getStr(ByteBuffer b) {",
         "        long n = getU32(b); byte[] raw = new byte[(int)n]; b.get(raw);",
         "        return new String(raw, StandardCharsets.UTF_8);",
         "    }",
         "}"]
    with open(os.path.join(pkg_dir, "Wire.java"), "w", encoding="utf-8") as f:
        f.write("\n".join(L))


def _write_client_version(pkg_dir: str, pkg: str, version: str) -> None:
    L = ["// AUTO-GENERATED. DO NOT EDIT.", f"package {pkg};", "",
         "public class ClientVersion {",
         f'    public static final String VERSION = "{version}";',
         "",
         "    public static String get() {",
         "        return VERSION;",
         "    }",
         "}"]
    with open(os.path.join(pkg_dir, "ClientVersion.java"), "w", encoding="utf-8") as f:
        f.write("\n".join(L))


def generate(schema, java_output_dir: str) -> None:
    pkg = schema.java_package
    pkg_dir = os.path.join(java_output_dir, *pkg.split("."))
    os.makedirs(pkg_dir, exist_ok=True)

    for c in schema.classes:
        L = ["// AUTO-GENERATED. DO NOT EDIT.", f"package {pkg};", "",
             "import java.util.*;", "",
             f"public interface {c.name()} {{",
             "    long getThisId();",
             "    void setThisId(long id);"]
        for m in c.methods():
            ret = type_to_java(m.return_ref())
            args = ", ".join(f"{type_to_java(a.type_ref())} {a.name()}" for a in m.arguments())
            L.append(f"    {ret} {m.name()}({args});")
        L.append("}")
        with open(os.path.join(pkg_dir, f"{c.name()}.java"),
                  "w", encoding="utf-8") as f:
            f.write("\n".join(L))

    L = ["// AUTO-GENERATED. DO NOT EDIT.", f"package {pkg};", "",
         "import java.util.*;", "",
         "public interface IRunScriptGlobalFunctions {"]
    for f in schema.functions:
        ret = type_to_java(f.return_ref())
        args = ", ".join(f"{type_to_java(a.type_ref())} {a.name()}" for a in f.arguments())
        L.append(f"    {ret} {f.name()}({args});")
    L.append("}")
    with open(os.path.join(pkg_dir, "IRunScriptGlobalFunctions.java"),
              "w", encoding="utf-8") as f:
        f.write("\n".join(L))

    _write_client_version(pkg_dir, pkg, schema.client_version)

    _write_wire(pkg_dir, pkg)

    for f in schema.functions:
        _write_struct(pkg_dir, pkg, f"{f.name()}Request", f.arguments(), prefix_id=False)
        if not f.return_ref().is_ref() and not f.return_ref().is_void():
            _write_struct(pkg_dir, pkg, f"{f.name()}Response",
                          [_ValueField(f.return_ref().name())], prefix_id=False)

    for c in schema.classes:
        for m in c.methods():
            _write_struct(pkg_dir, pkg, f"{m.name()}Request", m.arguments(), prefix_id=True)
            if not m.return_ref().is_ref() and not m.return_ref().is_void():
                _write_struct(pkg_dir, pkg, f"{m.name()}Response",
                              [_ValueField(m.return_ref().name())], prefix_id=False)


class _ValueField:
    def __init__(self, type_name: str):
        self.__name = "value"
        self.__type = type_name

    def name(self) -> str:
        return self.__name

    def type(self) -> str:
        return self.__type
