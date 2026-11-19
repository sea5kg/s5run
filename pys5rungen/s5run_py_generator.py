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

"""Python code generator."""

from .s5run_typeref import S5RunCliTypeRef, s5run_parse_type

# schema primitive name -> python type
PRIMITIVES = {
    "bool":   "bool",
    "int":    "int",
    "uint":   "int",
    "float":  "float",
    "double": "float",
    "string": "str",
    "byte":   "int",
}


def type_to_py(ref: S5RunCliTypeRef) -> str:
    if ref.is_prim():
        return PRIMITIVES[ref.name()]
    if ref.is_array():
        return f"List[{type_to_py(s5run_parse_type(ref.inner()))}]"
    return f'"{ref.name()}"'


def _encode_lines(ref, expr: str, indent: str) -> list[str]:
    if ref.is_prim():
        fn = {"bool": "_put_bool", "byte": "_put_u8",
              "int": "_put_u64", "uint": "_put_u64",
              "float": "_put_f64", "double": "_put_f64",
              "string": "_put_str"}[ref.wire()]
        return [f"{indent}{fn}(b, {expr})"]
    if ref.is_array():
        inner = s5run_parse_type(ref.inner())
        return [
            f"{indent}_put_u32(b, len({expr}))",
            f"{indent}for _e in {expr}:",
            *_encode_lines(inner, "_e", indent + "    "),
        ]
    if ref.is_ref():
        return [f"{indent}_put_u64(b, {expr}.this_id)"]
    raise ValueError(ref.name())


def _decode_expr(ref, reader: str) -> str:
    if ref.is_prim():
        fn = {"bool": "bool", "byte": "u8", "int": "u64",
              "uint": "u64", "float": "f64", "double": "f64",
              "string": "str"}[ref.wire()]
        return f"{reader}.{fn}()"
    if ref.is_array():
        inner = s5run_parse_type(ref.inner())
        return f"[{_decode_expr(inner, reader)} for _ in range({reader}.u32())]"
    if ref.is_ref():
        return f"{reader}.u64()"
    raise ValueError(ref.name())


def _emit_dataclass(out: list, name: str, fields, prefix_id: bool) -> None:
    out.append("@dataclass")
    out.append(f"class {name}:")
    if prefix_id:
        out.append("    this_id: int")
    if not fields:
        out.append("    pass")
    for a in fields:
        out.append(f"    {a.name()}: {type_to_py(s5run_parse_type(a.type()))}")
    out.append("")
    out.append("    def encode(self) -> bytes:")
    out.append("        b = bytearray()")
    if prefix_id:
        out.append("        _put_u64(b, self.this_id)")
    for a in fields:
        out += _encode_lines(s5run_parse_type(a.type()), f"self.{a.name()}", "        ")
    out.append("        return bytes(b)")
    out.append("")
    out.append("    @staticmethod")
    out.append(f"    def decode(data: bytes) -> '{name}':")
    out.append("        r = _Reader(data)")
    parts = []
    if prefix_id:
        parts.append("this_id=r.u64()")
    for a in fields:
        parts.append(f"{a.name()}={_decode_expr(s5run_parse_type(a.type()), 'r')}")
    out.append(f"        return {name}({', '.join(parts)})")
    out.append("")


def generate(schema) -> str:
    L = ["# AUTO-GENERATED. DO NOT EDIT.",
         "from abc import ABC, abstractmethod",
         "from dataclasses import dataclass, field",
         "from typing import List, Optional, Protocol",
         "import struct",
         "",
         f"# module: {schema.py_module}",
         ""]

    for c in schema.classes:
        L.append(f"class {c.aggregate_interface()}(Protocol):")
        L.append("    this_id: int")
        for m in c.methods():
            ret = type_to_py(m.return_ref())
            args = ", ".join(f"{a.name()}: {type_to_py(a.type_ref())}" for a in m.arguments())
            sig = f"self, {args}" if args else "self"
            L.append(f"    def {m.name()}({sig}) -> {ret}: ...")
        L.append("")

    L.append("class IRunScriptGlobalFunctions(ABC):")
    for f in schema.functions:
        ret = type_to_py(f.return_ref())
        args = ", ".join(f"{a.name()}: {type_to_py(a.type_ref())}" for a in f.arguments())
        sig = f"self, {args}" if args else "self"
        L.append("    @abstractmethod")
        L.append(f"    def {f.name()}({sig}) -> {ret}: ...")
    L.append("")

    L += ["# ---- wire codec ----", "",
          "def _put_u8(b, v):   b.append(v & 0xFF)",
          "def _put_bool(b, v): b.append(1 if v else 0)",
          "def _put_u32(b, v):  b += struct.pack('<I', v & 0xFFFFFFFF)",
          "def _put_u64(b, v):  b += struct.pack('<Q', v & 0xFFFFFFFFFFFFFFFF)",
          "def _put_f64(b, v):  b += struct.pack('<d', v)",
          "def _put_str(b, s):",
          "    raw = s.encode('utf-8')",
          "    _put_u32(b, len(raw)); b += raw",
          "",
          "class _Reader:",
          "    def __init__(self, data): self.p = 0; self.data = data",
          "    def u8(self):   v = self.data[self.p]; self.p += 1; return v",
          "    def bool(self): return self.u8() != 0",
          "    def u32(self):  v = struct.unpack_from('<I', self.data, self.p)[0]; self.p += 4; return v",
          "    def u64(self):  v = struct.unpack_from('<Q', self.data, self.p)[0]; self.p += 8; return v",
          "    def f64(self):  v = struct.unpack_from('<d', self.data, self.p)[0]; self.p += 8; return v",
          "    def str(self):",
          "        n = self.u32(); v = self.data[self.p:self.p+n].decode('utf-8'); self.p += n; return v",
          ""]

    for f in schema.functions:
        _emit_dataclass(L, f"{f.name()}Request", f.arguments(), prefix_id=False)
        if not f.return_ref().is_ref() and not f.return_ref().is_void():
            _emit_dataclass(L, f"{f.name()}Response",
                            [_ValueField(f.return_ref().name())], prefix_id=False)

    for c in schema.classes:
        for m in c.methods():
            _emit_dataclass(L, f"{m.name()}Request", m.arguments(), prefix_id=True)
            if not m.return_ref().is_ref() and not m.return_ref().is_void():
                _emit_dataclass(L, f"{m.name()}Response",
                                [_ValueField(m.return_ref().name())], prefix_id=False)

    return "\n".join(L)


class _ValueField:
    def __init__(self, type_name: str):
        self.__name = "value"
        self.__type = type_name

    def name(self) -> str:
        return self.__name

    def type(self) -> str:
        return self.__type
