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

"""Java 17 client generator: modern standalone client over the binary protocol."""

import os

from .s5run_typeref import S5RunCliTypeRef, s5run_parse_type

PRIMITIVES = {
    "bool": "boolean",
    "int": "long",
    "uint": "long",
    "float": "double",
    "double": "double",
    "string": "String",
    "byte": "byte",
}

_BOXED = {"long": "Long", "boolean": "Boolean", "double": "Double", "byte": "Byte"}


def type_to_java(ref: S5RunCliTypeRef) -> str:
    if ref.is_prim():
        return PRIMITIVES[ref.name()]
    if ref.is_array():
        inner = type_to_java(s5run_parse_type(ref.inner()))
        return f"List<{_BOXED.get(inner, inner)}>"
    if ref.is_ref():
        return ref.name()
    return ref.name()


def _boxed(t: str) -> str:
    return _BOXED.get(t, t)


def _param_type(ref: S5RunCliTypeRef) -> str:
    t = type_to_java(ref)
    return t if t in ("long", "int", "boolean", "double", "byte") else t


def _is_void(fm) -> bool:
    return fm.return_type() == "void"


def _has_convert(fm) -> bool:
    args = fm.arguments()
    return len(args) == 1 and bool(args[0].convert_from())


class _Java11Generator:

    def __init__(self, schema):
        self.schema = schema
        self.pkg = schema.java11_package

    # ------------------------------------------------------------------
    # file helpers
    # ------------------------------------------------------------------

    def _write(self, pkg_dir: str, name: str, body: str) -> None:
        imports = "import java.util.*;\n\n" if "List<" in body or "Map<" in body else ""
        header = (
            "// AUTO-GENERATED. DO NOT EDIT.\n"
            f"package {self.pkg};\n\n"
            f"{imports}"
        )
        with open(os.path.join(pkg_dir, f"{name}.java"), "w", encoding="utf-8") as f:
            f.write(header + body.strip() + "\n")

    # ------------------------------------------------------------------
    # typed read/write expressions
    # ------------------------------------------------------------------

    def _read_expr(self, ref) -> str:
        if ref.is_prim():
            name = ref.name()
            if name in ("int", "uint"):
                return "r.readLongInt()"
            if name == "bool":
                return "r.readBool()"
            if name in ("float", "double"):
                return "r.readDouble()"
            if name == "string":
                return "r.strValue()"
            if name == "byte":
                return "r.readByteNum()"
        return f"Wire.readList{self._suffix(ref.name())}(r)"

    def _write_stmt(self, ref, expr: str) -> str:
        if ref.is_prim():
            name = ref.name()
            if name in ("int", "uint"):
                return f"w.int64({expr});"
            if name == "bool":
                return f"w.boolValue({expr});"
            if name in ("float", "double"):
                return f"w.f64Value({expr});"
            if name == "string":
                return f"w.strValue({expr});"
            if name == "byte":
                return f"w.byteValue((int) {expr});"
        return f"Wire.writeList{self._suffix(ref.name())}(w, {expr});"

    def _suffix(self, type_str: str) -> str:
        ref = s5run_parse_type(type_str)
        if ref.is_prim():
            cap = {"bool": "Bool", "int": "Long", "uint": "Long", "float": "Double",
                   "double": "Double", "string": "Str", "byte": "Byte"}[ref.name()]
            return cap
        return "Of" + self._suffix(ref.inner())

    def _list_helpers_needed(self) -> list:
        out = []
        seen = set()

        def visit(fm):
            for a in fm.arguments():
                if a.convert_from():
                    continue
                t = a.type()
                r = s5run_parse_type(t)
                if r.is_array() and not s5run_parse_type(r.inner()).is_ref():
                    if t not in seen:
                        seen.add(t)
                        out.append(t)
            if not _is_void(fm):
                r = fm.return_ref()
                if r.is_array() and not s5run_parse_type(r.inner()).is_ref():
                    t = fm.return_type()
                    if t not in seen:
                        seen.add(t)
                        out.append(t)

        for c in self.schema.classes:
            for m in c.methods():
                visit(m)
        for f in self.schema.functions:
            visit(f)
        return out

    def _needs_refs(self) -> bool:
        def uses(fm):
            for a in fm.arguments():
                if a.convert_from():
                    continue
                r = a.type_ref()
                if r.is_ref():
                    return True
                if r.is_array() and s5run_parse_type(r.inner()).is_ref():
                    return True
            r = fm.return_ref()
            if r.is_ref() or (r.is_array() and s5run_parse_type(r.inner()).is_ref()):
                return True
            return False

        for c in self.schema.classes:
            for m in c.methods():
                if uses(m):
                    return True
        for f in self.schema.functions:
            if uses(f):
                return True
        return False

    # ------------------------------------------------------------------
    # per-schema decode/encode blocks for Context
    # ------------------------------------------------------------------

    def _decode_args(self, fm) -> str:
        lines = []
        if _has_convert(fm):
            a = fm.arguments()[0]
            lines.append(f"            long n_{a.name()} = r.u32();")
            lines.append(f"            List<String> {a.name()} = new ArrayList<>((int) n_{a.name()});")
            lines.append(f"            for (long i = 0; i < n_{a.name()}; i++)")
            lines.append(f"                {a.name()}.add(valueToString(Wire.decodeValue(r)));")
            return "\n".join(lines)
        for a in fm.arguments():
            ref = a.type_ref()
            if ref.is_array() and s5run_parse_type(ref.inner()).is_ref():
                cls = s5run_parse_type(ref.inner()).name()
                lines += [
                    "            if (r.u8() != Wire.T_ARRAY) throw new RuntimeException(\"wire: expected array\");",
                    f"            int cnt_{a.name()} = (int) r.u32();",
                    f"            List<{cls}> {a.name()} = new ArrayList<>(cnt_{a.name()});",
                    f"            for (int i = 0; i < cnt_{a.name()}; i++) {{",
                    f"                int refId = readRefId(r);",
                    f"                {cls} obj = instanceById(\"{cls}\", refId);",
                    f"                {a.name()}.add(obj);",
                    "            }",
                ]
            elif ref.is_ref():
                cls = ref.name()
                lines += [
                    f"            int refId_{a.name()} = readRefId(r);",
                    f"            {cls} {a.name()} = instanceById(\"{cls}\", refId_{a.name()});",
                ]
            else:
                t = type_to_java(ref)
                lines.append(f"            {t} {a.name()} = {self._read_expr(ref)};")
        return "\n".join(lines)

    def _call_args(self, fm) -> str:
        return ", ".join(a.name() for a in fm.arguments())

    def _encode_result(self, fm) -> str:
        if _is_void(fm):
            return "            w.u8(Wire.T_UNDEFINED);"
        ref = fm.return_ref()
        if ref.is_array() and s5run_parse_type(ref.inner()).is_ref():
            cls = s5run_parse_type(ref.inner()).name()
            return "\n".join([
                "            w.u8(Wire.T_ARRAY);",
                f"            w.u32(res.size());",
                f"            for ({cls} item : res) {{",
                f"                int id = registerInstance(item);",
                "                w.u8(Wire.T_REF);",
                "                w.i32(id);",
                f"                w.str(\"{cls}\");",
                "            }",
            ])
        if ref.is_ref():
            cls = ref.name()
            return "\n".join([
                f"            int objId = registerInstance(res);",
                "            w.u8(Wire.T_REF);",
                "            w.i32(objId);",
                f"            w.str(\"{cls}\");",
            ])
        return "            " + self._write_stmt(ref, "res")

    # ------------------------------------------------------------------
    # files
    # ------------------------------------------------------------------

    def generate(self, java17_output_dir: str) -> None:
        pkg_dir = os.path.join(java17_output_dir, *self.pkg.split("."))
        os.makedirs(pkg_dir, exist_ok=True)

        self._write(pkg_dir, "Wire", self._wire())
        self._write(pkg_dir, "Value", self._value())
        self._write(pkg_dir, "S5RunObject", self._s5run_object())
        self._write(pkg_dir, "S5RunFunction", self._s5run_function())
        for c in self.schema.classes:
            self._write(pkg_dir, c.name(), self._schema_class(c))
        for f in self.schema.functions:
            self._write(pkg_dir, f"GlobalFunction_{f.name()}", self._schema_function(f))
        self._write(pkg_dir, "RemoteServerInfo", self._remote_server_info())
        self._write(pkg_dir, "RunResult", self._run_result())
        self._write(pkg_dir, "Context", self._context())
        self._write(pkg_dir, "ClientVersion", self._client_version())

    def _client_version(self) -> str:
        return f'''
public final class ClientVersion {{
    public static final String VERSION = "{self.schema.client_version}";

    public static String get() {{
        return VERSION;
    }}
    private ClientVersion() {{}}
}}
'''

    def _wire(self) -> str:
        list_helpers = []
        for type_str in self._list_helpers_needed():
            ref = s5run_parse_type(type_str)
            inner = s5run_parse_type(ref.inner())
            sfx = self._suffix(type_str)
            inner_t = _boxed(type_to_java(inner))
            list_helpers.append(f'''
    public static List<{inner_t}> readList{sfx}(Reader r) {{
        if (r.u8() != T_ARRAY) throw new RuntimeException("wire: expected array");
        int n = (int) r.u32();
        List<{inner_t}> out = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {{
            out.add({self._read_expr(inner)});
        }}
        return out;
    }}

    public static void writeList{sfx}(Writer w, List<{inner_t}> v) {{
        w.u8(T_ARRAY);
        w.u32(v.size());
        for ({_param_type(inner)} e : v) {{
            {self._write_stmt(inner, "e")}
        }}
    }}''')

        encode_value = '''
    public static void encodeValue(Writer w, Value v) {
        if (v instanceof Value.Null) {
            w.u8(T_UNDEFINED);
        } else if (v instanceof Value.Bool) {
            w.u8(T_BOOL); w.bool(((Value.Bool) v).v());
        } else if (v instanceof Value.Int) {
            w.int64(((Value.Int) v).v());
        } else if (v instanceof Value.Dbl) {
            w.u8(T_FLOAT64); w.f64(((Value.Dbl) v).v());
        } else if (v instanceof Value.Str) {
            w.strValue(((Value.Str) v).v());
        } else if (v instanceof Value.Arr) {
            Value.Arr a = (Value.Arr) v;
            w.u8(T_ARRAY);
            w.u32(a.v().size());
            for (Value e : a.v()) encodeValue(w, e);
        } else if (v instanceof Value.Obj) {
            Value.Obj o = (Value.Obj) v;
            w.u8(T_OBJECT);
            w.u32(o.v().size());
            for (Map.Entry<String, Value> en : o.v().entrySet()) {
                w.strValue(en.getKey());
                encodeValue(w, en.getValue());
            }
        } else if (v instanceof Value.Ref) {
            Value.Ref rf = (Value.Ref) v;
            w.u8(T_REF);
            w.i32(rf.thisId());
            w.str(rf.type());
        }
    }

    public static Value decodeValue(Reader r) {
        int tag = r.u8();
        switch (tag) {
            case T_UNDEFINED: return Value.Null.INSTANCE;
            case T_BOOL: return new Value.Bool(r.readBoolRaw());
            case T_INT8: return new Value.Int(r.readI8());
            case T_INT16: return new Value.Int(r.readI16());
            case T_INT32: case T_UINT32: return new Value.Int(r.i32());
            case T_INT64: case T_UINT64: return new Value.Int(r.i64());
            case T_UINT8: return new Value.Int(r.u8());
            case T_UINT16: return new Value.Int(r.u16());
            case T_FLOAT32: return new Value.Dbl(r.f32());
            case T_FLOAT64: return new Value.Dbl(r.f64());
            case T_STRING: return new Value.Str(r.str());
            case T_ARRAY: {
                int n = (int) r.u32();
                List<Value> a = new ArrayList<>(n);
                for (int i = 0; i < n; i++) a.add(decodeValue(r));
                return new Value.Arr(a);
            }
            case T_OBJECT: {
                int n = (int) r.u32();
                Map<String, Value> o = new LinkedHashMap<>();
                for (int i = 0; i < n; i++) o.put(r.strValue(), decodeValue(r));
                return new Value.Obj(o);
            }
            case T_REF: return new Value.Ref(r.i32(), r.str());
            default: throw new RuntimeException("wire: unknown value tag " + tag);
        }
    }'''

        value_to_string = '''
    public static String valueToString(Value v) {
        if (v instanceof Value.Null) return "null";
        if (v instanceof Value.Bool) return Boolean.toString(((Value.Bool) v).v());
        if (v instanceof Value.Int) return Long.toString(((Value.Int) v).v());
        if (v instanceof Value.Dbl) return Double.toString(((Value.Dbl) v).v());
        if (v instanceof Value.Str) return ((Value.Str) v).v();
        throw new RuntimeException("wire: expected a scalar argument");
    }'''

        return f'''
import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.*;

public final class Wire {{
    public static final int
        T_UNDEFINED = 0x00, T_BOOL = 0x01, T_INT8 = 0x02, T_INT16 = 0x03,
        T_INT32 = 0x04, T_INT64 = 0x05, T_UINT8 = 0x06, T_UINT16 = 0x07,
        T_UINT32 = 0x08, T_UINT64 = 0x09, T_FLOAT32 = 0x0A, T_FLOAT64 = 0x0B,
        T_STRING = 0x0C, T_ARRAY = 0x0D, T_OBJECT = 0x0E, T_REF = 0x0F;

    public static final int
        M_EXEC_REQ = 0x01, M_EXEC_RESP = 0x02, M_CALLBACK_REQ = 0x03,
        M_CALLBACK_RESP = 0x04, M_REGISTER_FUNC = 0x05, M_REGISTER_OBJ = 0x06,
        M_REGISTER_TYPE = 0x07, M_REGISTER_RESP = 0x08,
        M_STATS_REQ = 0x09, M_STATS_RESP = 0x0A, M_SERVER_HELLO = 0x0F;

    public static final class Writer {{
        private final ByteArrayOutputStream buf = new ByteArrayOutputStream();

        public byte[] toByteArray() {{ return buf.toByteArray(); }}

        public void u8(int v) {{ buf.write(v & 0xFF); }}

        public void u32(long v) {{
            for (int i = 0; i < 4; i++) buf.write((int) ((v >>> (8 * i)) & 0xFF));
        }}

        public void u64(long v) {{
            for (int i = 0; i < 8; i++) buf.write((int) ((v >>> (8 * i)) & 0xFF));
        }}

        public void i32(int v) {{ u32(v & 0xFFFFFFFFL); }}
        public void int64(long v) {{
            if (v >= Byte.MIN_VALUE && v <= Byte.MAX_VALUE) {{ u8(T_INT8); u8((int) v); }}
            else if (v >= Short.MIN_VALUE && v <= Short.MAX_VALUE) {{ u8(T_INT16); u16((int) v); }}
            else if (v >= Integer.MIN_VALUE && v <= Integer.MAX_VALUE) {{ u8(T_INT32); u32(v); }}
            else {{ u8(T_INT64); u64(v); }}
        }}

        public void u16(int v) {{
            buf.write(v & 0xFF);
            buf.write((v >> 8) & 0xFF);
        }}

        public void f64(double v) {{ u64(Double.doubleToLongBits(v)); }}
        public void f32(float v) {{ u32(Float.floatToIntBits(v) & 0xFFFFFFFFL); }}

        public void bool(boolean v) {{ u8(v ? 1 : 0); }}

        public void strValue(String s) {{
            u8(T_STRING);
            str(s);
        }}

        public void boolValue(boolean v) {{
            u8(T_BOOL);
            bool(v);
        }}

        public void byteValue(int v) {{
            u8(T_UINT8);
            u8(v);
        }}

        public void f64Value(double v) {{
            u8(T_FLOAT64);
            f64(v);
        }}

        public void str(String s) {{
            byte[] b = s.getBytes(StandardCharsets.UTF_8);
            if (b.length <= 0xFF) {{ u8(0x00); u8(b.length); }}
            else if (b.length <= 0xFFFF) {{ u8(0x01); u16(b.length); }}
            else {{ u8(0x02); u32(b.length); }}
            buf.write(b, 0, b.length);
        }}
    }}

    public static final class Reader {{
        private final ByteBuffer buf;

        public Reader(byte[] data) {{ buf = ByteBuffer.wrap(data); }}

        public int u8() {{ return buf.get() & 0xFF; }}
        public int u16() {{ return (buf.get() & 0xFF) | ((buf.get() & 0xFF) << 8); }}
        public long u32() {{
            long v = 0;
            for (int i = 0; i < 4; i++) v |= (buf.get() & 0xFFL) << (8 * i);
            return v;
        }}
        public long u64() {{
            long v = 0;
            for (int i = 0; i < 8; i++) v |= (buf.get() & 0xFFL) << (8 * i);
            return v;
        }}
        public int i32() {{ return (int) u32(); }}
        public long i64() {{ return u64(); }}
        public int readI8() {{ return (byte) u8(); }}
        public int readI16() {{ return (short) u16(); }}
        public long readLongInt() {{
            int tag = u8();
            switch (tag) {{
                case T_INT8: return (byte) u8();
                case T_INT16: return (short) u16();
                case T_INT32: case T_UINT32: return (long) i32();
                case T_INT64: case T_UINT64: return i64();
                case T_UINT8: return u8();
                case T_UINT16: return u16();
                default: throw new RuntimeException("wire: expected an integer");
            }}
        }}
        public byte readByteNum() {{
            if (u8() != T_UINT8) throw new RuntimeException("wire: expected a byte");
            return (byte) u8();
        }}
        public boolean readBool() {{
            if (u8() != T_BOOL) throw new RuntimeException("wire: expected a bool");
            return u8() != 0;
        }}
        public boolean readBoolRaw() {{ return u8() != 0; }}
        public double f32() {{ return Float.intBitsToFloat(i32()); }}
        public double f64() {{ return Double.longBitsToDouble(u64()); }}

        public String strValue() {{
            if (u8() != T_STRING) throw new RuntimeException("wire: expected a string");
            return str();
        }}

        public String str() {{
            int lenSize = u8();
            int n;
            switch (lenSize) {{
                case 0x00: n = u8(); break;
                case 0x01: n = u16(); break;
                case 0x02: n = (int) u32(); break;
                default: throw new RuntimeException("wire: bad string length size");
            }}
            byte[] b = new byte[n];
            buf.get(b);
            return new String(b, StandardCharsets.UTF_8);
        }}
    }}
{encode_value}
{value_to_string}
{"".join(list_helpers)}
    private Wire() {{}}
}}
'''

    def _value(self) -> str:
        return '''
public interface Value {
    final class Null implements Value {
        public static final Null INSTANCE = new Null();
        private Null() {}
    }
    final class Bool implements Value {
        final boolean val;
        public Bool(boolean v) { this.val = v; }
        public boolean v() { return val; }
    }
    final class Int implements Value {
        final long val;
        public Int(long v) { this.val = v; }
        public long v() { return val; }
    }
    final class Dbl implements Value {
        final double val;
        public Dbl(double v) { this.val = v; }
        public double v() { return val; }
    }
    final class Str implements Value {
        final String val;
        public Str(String v) { this.val = v; }
        public String v() { return val; }
    }
    final class Arr implements Value {
        final List<Value> val;
        public Arr(List<Value> v) { this.val = v; }
        public List<Value> v() { return val; }
    }
    final class Obj implements Value {
        final Map<String, Value> val;
        public Obj(Map<String, Value> v) { this.val = v; }
        public Map<String, Value> v() { return val; }
    }
    final class Ref implements Value {
        final int thisId;
        final String type;
        public Ref(int thisId, String type) { this.thisId = thisId; this.type = type; }
        public int thisId() { return thisId; }
        public String type() { return type; }
    }
}
'''

    def _s5run_object(self) -> str:
        return '''
public interface S5RunObject {
    String s5runClassName();
    List<String> s5runMethodNames();
}
'''

    def _s5run_function(self) -> str:
        return '''
public interface S5RunFunction {
    String s5runFunctionName();
}
'''

    def _schema_class(self, c) -> str:
        methods = []
        for m in c.methods():
            ret = "void" if _is_void(m) else type_to_java(m.return_ref())
            args = ", ".join(f"{_param_type(a.type_ref())} {a.name()}" for a in m.arguments())
            methods.append(f"    {ret} {m.name()}({args});")
        method_names = ", ".join(f'"{m.name()}"' for m in c.methods())
        return f'''
public interface {c.name()} extends S5RunObject {{
    @Override
    default String s5runClassName() {{ return "{c.name()}"; }}

    @Override
    default List<String> s5runMethodNames() {{ return List.of({method_names}); }}

{chr(10).join(methods)}
}}
'''

    def _schema_function(self, f) -> str:
        ret = "void" if _is_void(f) else type_to_java(f.return_ref())
        args = ", ".join(f"{_param_type(a.type_ref())} {a.name()}" for a in f.arguments())
        return f'''
public interface GlobalFunction_{f.name()} extends S5RunFunction {{
    @Override
    default String s5runFunctionName() {{ return "{f.name()}"; }}

    {ret} execute({args});
}}
'''

    def _remote_server_info(self) -> str:
        return '''
public final class RemoteServerInfo {
    private String address = "";
    private String apiKey = "";
    private String host = "";
    private int port;

    public RemoteServerInfo address(String address) {
        final String prefix = "s5run://";
        if (!address.startsWith(prefix)) {
            throw new IllegalArgumentException("expected format s5run://host:port");
        }
        String rest = address.substring(prefix.length());
        int colon = rest.lastIndexOf(':');
        if (colon <= 0 || colon + 1 >= rest.length()) {
            throw new IllegalArgumentException("expected format s5run://host:port");
        }
        this.address = address;
        this.host = rest.substring(0, colon);
        this.port = Integer.parseInt(rest.substring(colon + 1));
        return this;
    }

    public RemoteServerInfo apiKey(String apiKey) {
        this.apiKey = apiKey;
        return this;
    }

    public String getAddress() { return address; }
    public String getApiKey() { return apiKey; }
    public String getHost() { return host; }
    public int getPort() { return port; }
}
'''

    def _run_result(self) -> str:
        return '''
public final class RunResult {
    private final boolean hasError;
    private final String error;
    private final int errorLine;
    private final long durationMs;
    private final Value value;

    public RunResult(boolean hasError, String error, int errorLine, long durationMs, Value value) {
        this.hasError = hasError;
        this.error = error;
        this.errorLine = errorLine;
        this.durationMs = durationMs;
        this.value = value;
    }

    public boolean hasError() { return hasError; }
    public String error() { return error; }
    public int errorLine() { return errorLine; }
    public long durationMs() { return durationMs; }
    public Value value() { return value; }

    public static RunResult ok(Value value, long durationMs) {
        return new RunResult(false, "", 0, durationMs, value);
    }

    public static RunResult fail(String error, long durationMs) {
        String msg = error == null ? "unknown error" : error;
        return new RunResult(true, msg, parseErrorLine(msg), durationMs, Value.Null.INSTANCE);
    }

    static int parseErrorLine(String error) {
        if (error == null) return 0;
        int pos = 0;
        while (pos < error.length()) {
            int colon = error.indexOf(':', pos);
            if (colon < 0) return 0;
            int p1 = colon + 1;
            if (p1 >= error.length() || !Character.isDigit(error.charAt(p1))) {
                pos = p1;
                continue;
            }
            int p2 = p1;
            while (p2 < error.length() && Character.isDigit(error.charAt(p2))) p2++;
            if (p2 + 1 < error.length() && error.charAt(p2) == ':' && Character.isDigit(error.charAt(p2 + 1))) {
                return Integer.parseInt(error.substring(p1, p2));
            }
            pos = p2;
        }
        return 0;
    }
}
'''

    def _context(self) -> str:
        s = self.schema
        # --- per-class instance handler init ---
        method_handlers = []
        for c in s.classes:
            for m in c.methods():
                method_handlers.append(
                    f'        handlers.put("{m.name()}", (cbId, r) -> cbm_{c.name()}_{m.name()}(cbId, r));'
                )
        # --- function callbacks ---
        fn_callbacks = []
        for f in s.functions:
            decode = self._decode_args(f)
            call = self._call_args(f)
            if _is_void(f):
                call_res = f"as{f.name()}().execute({call});"
                encode = "            w.u8(Wire.T_UNDEFINED);"
            else:
                call_res = f"var res = as{f.name()}().execute({call});"
                encode = self._encode_result(f)
            if _has_convert(f):
                head = ""
            else:
                nargs = len(f.arguments())
                head = (f"            if (r.u32() != {nargs})"
                        f" throw new RuntimeException(\"{f.name()}: wrong argument count\");")
            fn_callbacks.append(f'''
    private void cb_{f.name()}(int cbId, Wire.Reader r) throws Exception {{
        {head}
        {decode}
        {call_res}
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
{encode}
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }}''')

        # --- class method callbacks ---
        m_callbacks = []
        for c in s.classes:
            for m in c.methods():
                decode = self._decode_args(m)
                call = self._call_args(m)
                conv = _has_convert(m)
                if conv:
                    head = "            long n = r.u32();\n            n -= 1;"
                else:
                    expected = 1 + len(m.arguments())
                    head = (f"            if (r.u32() != {expected})"
                            f" throw new RuntimeException(\"{c.name()}.{m.name()}: wrong argument count\");")
                if _is_void(m):
                    call_res = f"obj.{m.name()}({call});"
                    encode = "            w.u8(Wire.T_UNDEFINED);"
                else:
                    call_res = f"var res = obj.{m.name()}({call});"
                    encode = self._encode_result(m)
                # convert methods decode args use the n variable
                if conv:
                    decode = decode.replace(f"long n_{m.arguments()[0].name()} = r.u32();", "long n_ignored = 0;")
                    decode = "\n".join(
                        ln.replace(f"n_{m.arguments()[0].name()}", "n") for ln in decode.splitlines()
                    )
                m_callbacks.append(f'''
    private void cbm_{c.name()}_{m.name()}(int cbId, Wire.Reader r) throws Exception {{
        {head}
        int thisId = (int) r.readLongInt();
        {c.name()} obj = instanceById("{c.name()}", thisId);
{decode}
        {call_res}
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
{encode}
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }}''')

        # --- function as* helpers (safe casts) ---
        as_helpers = []
        for f in s.functions:
            as_helpers.append(
                f"    private GlobalFunction_{f.name()} as{f.name()}() {{\n"
                f"        return (GlobalFunction_{f.name()}) functions.get(\"{f.name()}\");\n"
                f"    }}"
            )

        needs_refs = self._needs_refs()

        cases = "\n".join(
            f'            case "{f.name()}": cb_{f.name()}(cbId, r); return;'
            for f in self.schema.functions
        )

        return f'''
import java.io.*;
import java.net.Socket;
import java.util.*;

public final class Context implements AutoCloseable {{
    @FunctionalInterface
    interface ThrowingHandler {{
        void handle(int cbId, Wire.Reader r) throws Exception;
    }}

    private final Map<String, ThrowingHandler> handlers = new HashMap<>();
    private final Map<String, S5RunFunction> functions = new HashMap<>();
    private final Map<String, Map<Integer, S5RunObject>> instances = new HashMap<>();
    private final Map<String, List<String>> knownClasses = new HashMap<>();
    private final List<RemoteServerInfo> servers = new ArrayList<>();
    private final List<PendingType> pendingTypes = new ArrayList<>();
    private final List<PendingObject> pendingObjects = new ArrayList<>();
    private final List<String> pendingFunctions = new ArrayList<>();
    private String script = "";
    private int nextThisId = 0;
    private Socket socket;
    private InputStream in;
    private OutputStream out;

    private static final class PendingType {{
        final String name;
        final List<String> methods;
        PendingType(String name, List<String> methods) {{ this.name = name; this.methods = methods; }}
    }}

    private static final class PendingObject {{
        final String name;
        final int thisId;
        final List<String> methods;
        PendingObject(String name, int thisId, List<String> methods) {{ this.name = name; this.thisId = thisId; this.methods = methods; }}
    }}

    public Context() {{
{chr(10).join("        " + h for h in method_handlers)}
    }}

    public static String clientVersion() {{
        return ClientVersion.VERSION;
    }}

    public Context setScript(String script) {{
        this.script = script;
        return this;
    }}

    public Context addServer(RemoteServerInfo server) {{
        Objects.requireNonNull(server, "server");
        servers.add(server);
        return this;
    }}

    public Context addClass(S5RunObject sample) {{
        return addClass(sample.s5runClassName(), sample.s5runMethodNames());
    }}

    private Context addClass(String name, List<String> methods) {{
        if (socket != null) {{
            try {{
                sendRegisterType(name, methods);
            }} catch (Exception e) {{
                throw new RuntimeException(e);
            }}
        }} else {{
            pendingTypes.add(new PendingType(name, methods));
        }}
        return this;
    }}

    public Context addObject(S5RunObject obj) {{
        String cls = obj.s5runClassName();
        int id = ++nextThisId;
        instances.computeIfAbsent(cls, k -> new HashMap<>()).put(id, obj);
        if (socket != null) {{
            try {{
                sendRegisterObject(cls, id, obj.s5runMethodNames());
            }} catch (Exception e) {{
                throw new RuntimeException(e);
            }}
        }} else {{
            pendingObjects.add(new PendingObject(cls, id, obj.s5runMethodNames()));
        }}
        return this;
    }}

    public Context addFunction(S5RunFunction fn) {{
        functions.put(fn.s5runFunctionName(), fn);
        String name = fn.s5runFunctionName();
        handlers.put(name, (cbId, r) -> dispatchByName(name, cbId, r));
        if (socket != null) {{
            try {{
                sendRegisterFunction(fn.s5runFunctionName());
            }} catch (Exception e) {{
                throw new RuntimeException(e);
            }}
        }} else {{
            pendingFunctions.add(fn.s5runFunctionName());
        }}
        return this;
    }}

    // ------------------------------------------------------------------
    // public entry point
    // ------------------------------------------------------------------

    public static RunResult runScript(Context ctx, String entry, List<Value> args) {{
        long started = System.currentTimeMillis();
        try {{
            ctx.connect();
            ctx.flushPending();
            Wire.Writer w = new Wire.Writer();
            w.str(ctx.script);
            w.str(entry);
            w.u32(args.size());
            for (Value a : args) Wire.encodeValue(w, a);
            ctx.sendMessage(Wire.M_EXEC_REQ, w.toByteArray());
            while (true) {{
                Frame f = ctx.readFrame();
                if (f.msgType == Wire.M_CALLBACK_REQ) {{
                    ctx.dispatchCallback(f.body);
                    continue;
                }}
                if (f.msgType == Wire.M_EXEC_RESP) {{
                    Wire.Reader r = new Wire.Reader(f.body);
                    if (r.u8() == 0) {{
                        throw new RuntimeException(r.str());
                    }}
                    return RunResult.ok(Wire.decodeValue(r), System.currentTimeMillis() - started);
                }}
                throw new RuntimeException("s5run: unexpected message type");
            }}
        }} catch (Exception e) {{
            return RunResult.fail(e.getMessage(), System.currentTimeMillis() - started);
        }}
    }}

    // ------------------------------------------------------------------
    // callbacks
    // ------------------------------------------------------------------

{chr(10).join(fn_callbacks)}

{chr(10).join(m_callbacks)}

{chr(10).join(as_helpers)}

    private void dispatchByName(String name, int cbId, Wire.Reader r) throws Exception {{
        switch (name) {{
{cases}
            default: throw new RuntimeException("unknown function: " + name);
        }}
    }}

    private void dispatchCallback(byte[] body) throws Exception {{
        Wire.Reader r = new Wire.Reader(body);
        int cbId = r.i32();
        String method = r.str();
        ThrowingHandler handler = handlers.get(method);
        if (handler == null) {{
            Wire.Writer w = new Wire.Writer();
            w.i32(cbId);
            w.u8(0);
            w.str("unknown method: " + method);
            sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
            return;
        }}
        try {{
            handler.handle(cbId, r);
        }} catch (Exception e) {{
            Wire.Writer w = new Wire.Writer();
            w.i32(cbId);
            w.u8(0);
            w.str(String.valueOf(e.getMessage()));
            sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
        }}
    }}

    @SuppressWarnings("unchecked")
    private <T extends S5RunObject> T instanceById(String cls, int thisId) {{
        T obj = (T) instances.getOrDefault(cls, Map.of()).get(thisId);
        if (obj == null) {{
            throw new RuntimeException(cls + ": unknown this_id " + thisId);
        }}
        return obj;
    }}

    private int registerInstance(S5RunObject obj) {{
        int id = ++nextThisId;
        instances.computeIfAbsent(obj.s5runClassName(), k -> new HashMap<>()).put(id, obj);
        return id;
    }}

    private int readRefId(Wire.Reader r) {{
        if (r.u8() != Wire.T_REF) throw new RuntimeException("wire: expected an object reference");
        int id = r.i32();
        r.str(); // skip the type name
        return id;
    }}

    private static String valueToString(Value v) {{
        return Wire.valueToString(v);
    }}

    // ------------------------------------------------------------------
    // connection
    // ------------------------------------------------------------------

    private void connect() throws Exception {{
        if (socket != null) return;
        if (servers.isEmpty()) {{
            throw new RuntimeException("s5run: no server added (addServer)");
        }}
        int best = 0;
        if (servers.size() > 1) {{
            long bestLoad = -1;
            for (int i = 0; i < servers.size(); i++) {{
                long load = probeServerLoad(servers.get(i));
                if (load < 0) continue;
                if (bestLoad < 0 || load < bestLoad) {{
                    bestLoad = load;
                    best = i;
                }}
            }}
            if (bestLoad < 0) throw new RuntimeException("s5run: all servers are unreachable");
        }}
        RemoteServerInfo srv = servers.get(best);
        socket = new Socket(srv.getHost(), srv.getPort());
        socket.setTcpNoDelay(true);
        socket.setSoTimeout(10000);
        in = socket.getInputStream();
        out = socket.getOutputStream();
        readHello(in, out);
    }}

    private static void readHello(InputStream in, OutputStream out) throws Exception {{
        byte[] head = in.readNBytes(5);
        if (head.length < 5) throw new RuntimeException("s5run: expected server hello");
        int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
        int msgType = head[4] & 0xFF;
        byte[] body = in.readNBytes(len - 1);
        if (msgType != Wire.M_SERVER_HELLO || body.length < len - 1)
            throw new RuntimeException("s5run: expected server hello");
        String text = new String(body, java.nio.charset.StandardCharsets.UTF_8);
        String[] parts = text.split(" ");
        if (parts.length != 3 || !parts[0].equals("s5run"))
            throw new RuntimeException("s5run: bad server hello: " + text);
        int protocol = Integer.parseInt(parts[2]);
        if (protocol != 2)
            throw new RuntimeException("s5run: server protocol version " + protocol
                    + " is not supported (client supports 2)");
    }}

    private static long probeServerLoad(RemoteServerInfo server) {{
        try (Socket probe = new Socket()) {{
            probe.connect(new java.net.InetSocketAddress(server.getHost(), server.getPort()), 2000);
            probe.setTcpNoDelay(true);
            probe.setSoTimeout(2000);
            OutputStream po = probe.getOutputStream();
            InputStream pi = probe.getInputStream();
            readHello(pi, po);
            writeFrame(po, Wire.M_STATS_REQ, new byte[0]);
            byte[] head = pi.readNBytes(5);
            if (head.length < 5) return -1;
            int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
            int msgType = head[4] & 0xFF;
            byte[] body = pi.readNBytes(len - 1);
            if (msgType != Wire.M_STATS_RESP || body.length < 16) return -1;
            Wire.Reader r = new Wire.Reader(body);
            r.u32(); // threads
            long busy = r.u32();
            long queued = r.u32();
            return busy + queued;
        }} catch (Exception e) {{
            return -1;
        }}
    }}

    private void flushPending() throws Exception {{
        for (PendingType t : new ArrayList<>(pendingTypes)) {{
            sendRegisterType(t.name, t.methods);
        }}
        pendingTypes.clear();
        for (PendingObject o : new ArrayList<>(pendingObjects)) {{
            sendRegisterObject(o.name, o.thisId, o.methods);
        }}
        pendingObjects.clear();
        for (String name : new ArrayList<>(pendingFunctions)) {{
            sendRegisterFunction(name);
        }}
        pendingFunctions.clear();
    }}

    private void sendRegisterType(String name, List<String> methods) throws Exception {{
        Wire.Writer w = new Wire.Writer();
        w.str(name);
        w.u32(methods.size());
        for (String m : methods) w.str(m);
        sendMessage(Wire.M_REGISTER_TYPE, w.toByteArray());
        expectRegisterResp("register_type(" + name + ")");
    }}

    private void sendRegisterObject(String name, int thisId, List<String> methods) throws Exception {{
        Wire.Writer w = new Wire.Writer();
        w.str(name);
        w.i32(thisId);
        w.str(name);
        w.u32(methods.size());
        for (String m : methods) w.str(m);
        sendMessage(Wire.M_REGISTER_OBJ, w.toByteArray());
        expectRegisterResp("register_object(" + name + ")");
    }}

    private void sendRegisterFunction(String name) throws Exception {{
        Wire.Writer w = new Wire.Writer();
        w.str(name);
        sendMessage(Wire.M_REGISTER_FUNC, w.toByteArray());
        expectRegisterResp("register_function(" + name + ")");
    }}

    private void expectRegisterResp(String what) throws Exception {{
        Frame f = readFrame();
        if (f.msgType != Wire.M_REGISTER_RESP) {{
            throw new RuntimeException("s5run: " + what + " failed");
        }}
        Wire.Reader r = new Wire.Reader(f.body);
        if (r.u8() == 0) {{
            throw new RuntimeException("s5run: " + what + " error: " + r.str());
        }}
    }}

    /** Closes the connection; the context can be reused afterwards. */
    @Override
    public void close() {{
        if (socket != null) {{
            try {{
                socket.close();
            }} catch (IOException ignored) {{
            }}
            socket = null;
            in = null;
            out = null;
        }}
    }}

    // ------------------------------------------------------------------
    // framing
    // ------------------------------------------------------------------

    private static final class Frame {{
        final int msgType;
        final byte[] body;
        Frame(int msgType, byte[] body) {{ this.msgType = msgType; this.body = body; }}
    }}

    private void sendMessage(int msgType, byte[] body) throws Exception {{
        writeFrame(out, msgType, body);
    }}

    private static void writeFrame(OutputStream os, int msgType, byte[] body) throws Exception {{
        int len = 1 + body.length;
        os.write((len >> 24) & 0xFF);
        os.write((len >> 16) & 0xFF);
        os.write((len >> 8) & 0xFF);
        os.write(len & 0xFF);
        os.write(msgType);
        os.write(body);
        os.flush();
    }}

    private Frame readFrame() throws Exception {{
        byte[] head = in.readNBytes(5);
        if (head.length < 5) throw new RuntimeException("s5run: connection closed");
        int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
        if (len <= 0 || len > 64 * 1024 * 1024) throw new RuntimeException("s5run: bad frame length");
        int msgType = head[4] & 0xFF;
        byte[] body = in.readNBytes(len - 1);
        if (body.length < len - 1) throw new RuntimeException("s5run: connection closed");
        return new Frame(msgType, body);
    }}
}}
'''


def generate(schema, _output_dir: str) -> None:
    _Java11Generator(schema).generate(_output_dir)
