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

#include "s5run_generators.h"
#include "s5run_schema.h"

#include <sys/stat.h>

#include <fstream>
#include <string>
#include <vector>

namespace s5run {
namespace generators {

// -----------------------------------------------------------------------
// Type helpers
// -----------------------------------------------------------------------

static bool is_scalar_type(const std::string &t) {
  return t == "bool" || t == "int" || t == "uint" || t == "float" ||
         t == "double" || t == "string" || t == "byte";
}

static bool is_dynamic_type(const std::string &t) {
  return t == "var";
}

static bool is_void(const std::string &t) {
  return t == "void";
}

static bool is_array_type(const std::string &t) {
  return t.size() > 7 && t.substr(0, 6) == "array<" && t[t.size() - 1] == '>';
}

static std::string array_inner(const std::string &t) {
  return t.substr(6, t.size() - 7);
}

static bool is_ref_type(const std::string &t) {
  return !is_scalar_type(t) && !is_dynamic_type(t) && !is_void(t) && !is_array_type(t);
}

static bool has_convert(const std::vector<schema_argument> &args) {
  return args.size() == 1 && !args[0].convert_from.empty();
}

static std::string py_scalar_type(const std::string &t) {
  if (t == "bool") return "bool";
  if (t == "int" || t == "uint" || t == "byte") return "int";
  if (t == "float" || t == "double") return "float";
  if (t == "string") return "str";
  if (t == "var") return "Any";
  return t;
}

static std::string py_type(const std::string &t) {
  if (is_array_type(t))
    return "list[" + py_type(array_inner(t)) + "]";
  return py_scalar_type(t);
}

static std::string py_reader(const std::string &t) {
  if (t == "bool") return "_read_bool(buf)";
  if (t == "int" || t == "uint") return "_read_int(buf)";
  if (t == "float" || t == "double") return "_read_double(buf)";
  if (t == "string") return "_read_str_value(buf)";
  if (t == "byte") return "_read_byte(buf)";
  if (t == "var") return "_decode_value(buf)";
  return t;
}

static std::string py_writer_expr(const std::string &t, const std::string &expr) {
  if (t == "bool") return "_write_bool(out, " + expr + ")";
  if (t == "int" || t == "uint") return "_write_int(out, " + expr + ")";
  if (t == "float" || t == "double") return "_write_double(out, " + expr + ")";
  if (t == "string") return "_write_str_value(out, " + expr + ")";
  if (t == "byte") return "_write_byte(out, " + expr + ")";
  if (t == "var") return "_encode_value(out, " + expr + ")";
  return expr;
}

static std::string read_stmt(const std::string &t, const std::string &name) {
  if (is_scalar_type(t) || is_dynamic_type(t))
    return name + " = " + py_reader(t);
  std::string inner = array_inner(t);
  std::string elem = is_scalar_type(inner) ? py_reader(inner) : "None";
  return name + " = _read_array(buf, lambda: " + elem + ")";
}

static std::string write_stmt(const std::string &t, const std::string &expr) {
  if (is_scalar_type(t) || is_dynamic_type(t))
    return py_writer_expr(t, expr);
  std::string inner = array_inner(t);
  return "_write_array(out, " + expr + ", lambda e: " + py_writer_expr(inner, "e") + ")";
}

// -----------------------------------------------------------------------
// Callback bodies
// -----------------------------------------------------------------------

static void emit_decode_arg(std::vector<std::string> &L, const schema_argument &a) {
  const std::string &t = a.type;
  if (is_ref_type(t)) {
    L.push_back(a.name + " = _instance_by_id('" + t + "', _read_ref(buf).this_id)");
  } else if (is_array_type(t) && is_ref_type(array_inner(t))) {
    L.push_back(a.name + " = _read_ref_array(buf, '" + array_inner(t) + "')");
  } else {
    L.push_back(read_stmt(t, a.name));
  }
}

static void emit_encode_result(std::vector<std::string> &L, const std::string &t) {
  if (is_ref_type(t)) {
    L.push_back("_write_ref(out, _register_instance(res), '" + t + "')");
  } else if (is_array_type(t) && is_ref_type(array_inner(t))) {
    L.push_back("ids = [_register_instance(o) for o in res]");
    L.push_back("_write_ref_array(out, ids, '" + array_inner(t) + "')");
  } else {
    L.push_back(write_stmt(t, "res"));
  }
}

static std::string gen_function_cb(const schema_function &f, const std::string &indent) {
  std::vector<std::string> L;
  if (has_convert(f.arguments)) {
    const schema_argument &a = f.arguments[0];
    L.push_back("n = _read_count(buf)");
    L.push_back(a.name + " = [_value_to_string(_decode_value(buf)) for _ in range(n)]");
  } else {
    L.push_back("if _read_count(buf) != " + std::to_string(f.arguments.size()) + ":");
    L.push_back("    raise RuntimeError(\"" + f.name + ": wrong argument count\")");
    for (const auto &a : f.arguments)
      emit_decode_arg(L, a);
  }
  std::string call;
  for (const auto &a : f.arguments) {
    if (!call.empty())
      call += ", ";
    call += a.name;
  }
  if (is_void(f.return_type)) {
    L.push_back("fn" + f.name + ".execute(" + call + ")");
    L.push_back("_write_undefined(out)");
  } else {
    L.push_back("res = fn" + f.name + ".execute(" + call + ")");
    emit_encode_result(L, f.return_type);
  }
  std::string out;
  for (const auto &ln : L) {
    if (!out.empty())
      out += "\n";
    if (ln.empty())
      out += ln;
    else
      out += indent + ln;
  }
  return out;
}

static std::string gen_method_cb(const schema_class &c, const schema_method &m,
                                 const std::string &indent) {
  std::vector<std::string> L;
  bool conv = has_convert(m.arguments);
  if (conv) {
    const schema_argument &a0 = m.arguments[0];
    L.push_back("n_count = _read_count(buf)");
    L.push_back("if n_count < 1:");
    L.push_back("    raise RuntimeError(\"" + c.name + "." + m.name + ": expected this_id\")");
    L.push_back("obj = _instance_by_id('" + c.name + "', _read_int(buf))");
    L.push_back(a0.name + " = []");
    L.push_back("for _ in range(n_count - 1):");
    L.push_back("    " + a0.name + ".append(_value_to_string(_decode_value(buf)))");
  } else {
    L.push_back("n_count = _read_count(buf)");
    L.push_back("if n_count != " + std::to_string(1 + m.arguments.size()) + ":");
    L.push_back("    raise RuntimeError(\"" + c.name + "." + m.name + ": wrong argument count\")");
    L.push_back("obj = _instance_by_id('" + c.name + "', _read_int(buf))");
  }
  if (!conv) {
    for (const auto &a : m.arguments)
      emit_decode_arg(L, a);
  }
  std::string call;
  for (const auto &a : m.arguments) {
    if (!call.empty())
      call += ", ";
    call += a.name;
  }
  if (is_void(m.return_type)) {
    L.push_back("obj." + m.name + "(" + call + ")");
    L.push_back("_write_undefined(out)");
  } else {
    L.push_back("res = obj." + m.name + "(" + call + ")");
    emit_encode_result(L, m.return_type);
  }
  std::string out;
  for (const auto &ln : L) {
    if (!out.empty())
      out += "\n";
    if (ln.empty())
      out += ln;
    else
      out += indent + ln;
  }
  return out;
}

// -----------------------------------------------------------------------
// Static template blocks
// -----------------------------------------------------------------------

static const char *WIRE_PY_TEMPLATE = R"PY(class _Tag(IntEnum):
    UNDEFINED = 0x00
    BOOL = 0x01
    INT8 = 0x02
    INT16 = 0x03
    INT32 = 0x04
    INT64 = 0x05
    UINT8 = 0x06
    UINT16 = 0x07
    UINT32 = 0x08
    UINT64 = 0x09
    FLOAT32 = 0x0A
    FLOAT64 = 0x0B
    STRING = 0x0C
    ARRAY = 0x0D
    OBJECT = 0x0E
    REF = 0x0F


class _Msg(IntEnum):
    EXEC_REQ = 0x01
    EXEC_RESP = 0x02
    CALLBACK_REQ = 0x03
    CALLBACK_RESP = 0x04
    REGISTER_FUNC = 0x05
    REGISTER_OBJ = 0x06
    REGISTER_TYPE = 0x07
    REGISTER_RESP = 0x08
    STATS_REQ = 0x09
    STATS_RESP = 0x0A
    SERVER_HELLO = 0x0F


class _Reader:
    __slots__ = ("data", "pos")

    def __init__(self, data: bytes) -> None:
        self.data = data
        self.pos = 0

    def u8(self) -> int:
        v = self.data[self.pos]
        self.pos += 1
        return v

    def u32(self) -> int:
        v = struct.unpack_from("<I", self.data, self.pos)[0]
        self.pos += 4
        return v

    def i32(self) -> int:
        v = struct.unpack_from("<i", self.data, self.pos)[0]
        self.pos += 4
        return v

    def i64(self) -> int:
        v = struct.unpack_from("<q", self.data, self.pos)[0]
        self.pos += 8
        return v

    def f64(self) -> float:
        v = struct.unpack_from("<d", self.data, self.pos)[0]
        self.pos += 8
        return v

    def raw_str(self) -> str:
        len_size = self.u8()
        n = self.u8() if len_size == 0x00 else self.u32() if len_size == 0x02 else 0
        if len_size == 0x01:
            self.pos -= 3
            n = struct.unpack_from("<H", self.data, self.pos)[0]
            self.pos += 2
        v = self.data[self.pos:self.pos + n].decode("utf-8")
        self.pos += n
        return v


class _Writer:
    __slots__ = ("buf",)

    def __init__(self) -> None:
        self.buf = bytearray()

    def u8(self, v: int) -> None:
        self.buf.append(v & 0xFF)

    def u32(self, v: int) -> None:
        self.buf += struct.pack("<I", v & 0xFFFFFFFF)

    def i32(self, v: int) -> None:
        self.buf += struct.pack("<i", v)

    def i64(self, v: int) -> None:
        self.buf += struct.pack("<q", v)

    def f64(self, v: float) -> None:
        self.buf += struct.pack("<d", v)

    def raw_str(self, s: str) -> None:
        raw = s.encode("utf-8")
        if len(raw) <= 0xFF:
            self.u8(0x00)
            self.u8(len(raw))
        elif len(raw) <= 0xFFFF:
            self.u8(0x01)
            self.buf += struct.pack("<H", len(raw))
        else:
            self.u8(0x02)
            self.u32(len(raw))
        self.buf += raw
)PY";

static const char *VARIABLE_PY_TEMPLATE = R"PY(@dataclass(frozen=True, slots=True)
class Ref:
    """Reference to a remote object."""
    this_id: int
    type: str = ""


# ---- value codec ----

def _read_int(buf: _Reader) -> int:
    return {
        _Tag.INT8: lambda: struct.unpack("<b", buf.data[buf.pos:buf.pos + 1])[0] * 0 or buf.i64(),
    }.get(_Tag(0), None) is None and _read_int_impl(buf)


def _read_int_impl(buf: _Reader) -> int:
    tag = buf.u8()
    match _Tag(tag):
        case _Tag.INT8:
            v = struct.unpack_from("<b", buf.data, buf.pos)[0]
            buf.pos += 1
            return v
        case _Tag.INT16:
            v = struct.unpack_from("<h", buf.data, buf.pos)[0]
            buf.pos += 2
            return v
        case _Tag.INT32 | _Tag.UINT32:
            return buf.i32()
        case _Tag.INT64 | _Tag.UINT64:
            return buf.i64()
        case _Tag.UINT8:
            return buf.u8()
        case _Tag.UINT16:
            v = struct.unpack_from("<H", buf.data, buf.pos)[0]
            buf.pos += 2
            return v
    raise RuntimeError("wire: expected an integer")


def _read_bool(buf: _Reader) -> bool:
    if buf.u8() != _Tag.BOOL:
        raise RuntimeError("wire: expected a bool")
    return buf.u8() != 0


def _read_double(buf: _Reader) -> float:
    tag = buf.u8()
    if tag == _Tag.FLOAT32:
        v = struct.unpack_from("<f", buf.data, buf.pos)[0]
        buf.pos += 4
        return v
    if tag == _Tag.FLOAT64:
        return buf.f64()
    raise RuntimeError("wire: expected a double")


def _read_str_value(buf: _Reader) -> str:
    if buf.u8() != _Tag.STRING:
        raise RuntimeError("wire: expected a string")
    return buf.raw_str()


def _read_byte(buf: _Reader) -> int:
    if buf.u8() != _Tag.UINT8:
        raise RuntimeError("wire: expected a byte")
    return buf.u8()


def _read_ref(buf: _Reader) -> Ref:
    if buf.u8() != _Tag.REF:
        raise RuntimeError("wire: expected an object reference")
    this_id = buf.i32()
    return Ref(this_id, buf.raw_str())


def _read_array(buf: _Reader, read_elem: Callable[[], Any]) -> list:
    if buf.u8() != _Tag.ARRAY:
        raise RuntimeError("wire: expected an array")
    return [read_elem() for _ in range(buf.u32())]


def _decode_value(buf: _Reader) -> Any:
    tag = buf.u8()
    match _Tag(tag):
        case _Tag.UNDEFINED:
            return None
        case _Tag.BOOL:
            return buf.u8() != 0
        case _Tag.STRING:
            return buf.raw_str()
        case _Tag.FLOAT32 | _Tag.FLOAT64:
            buf.pos -= 1
            return _read_double(buf)
        case _Tag.ARRAY:
            buf.pos -= 1
            return _read_array(buf, lambda: _decode_value(buf))
        case _Tag.OBJECT:
            out: dict[str, Any] = {}
            for _ in range(buf.u32()):
                key = _read_str_value(buf)
                out[key] = _decode_value(buf)
            return out
        case _Tag.REF:
            buf.pos -= 1
            return _read_ref(buf)
        case _:
            buf.pos -= 1
            return _read_int_impl(buf)


def _value_to_string(v: Any) -> str:
    match v:
        case None:
            return "null"
        case bool():
            return "true" if v else "false"
        case int() | float():
            return str(v)
        case str():
            return v
    raise RuntimeError("wire: expected a scalar argument")


def _write_int(out: _Writer, v: int) -> None:
    if -128 <= v <= 127:
        out.u8(_Tag.INT8)
        out.u8(v)
    elif -32768 <= v <= 32767:
        out.u8(_Tag.INT16)
        out.buf += struct.pack("<h", v)
    elif -2147483648 <= v <= 2147483647:
        out.u8(_Tag.INT32)
        out.i32(v)
    else:
        out.u8(_Tag.INT64)
        out.i64(v)


def _write_bool(out: _Writer, v: bool) -> None:
    out.u8(_Tag.BOOL)
    out.u8(1 if v else 0)


def _write_double(out: _Writer, v: float) -> None:
    out.u8(_Tag.FLOAT64)
    out.f64(v)


def _write_str_value(out: _Writer, v: str) -> None:
    out.u8(_Tag.STRING)
    out.raw_str(v)


def _write_byte(out: _Writer, v: int) -> None:
    out.u8(_Tag.UINT8)
    out.u8(v)


def _write_undefined(out: _Writer) -> None:
    out.u8(_Tag.UNDEFINED)


def _write_ref(out: _Writer, this_id: int, cls: str) -> None:
    out.u8(_Tag.REF)
    out.i32(this_id)
    out.raw_str(cls)


def _write_array(out: _Writer, items: list, write_elem: Callable[[Any], None]) -> None:
    out.u8(_Tag.ARRAY)
    out.u32(len(items))
    for e in items:
        write_elem(e)


def _write_ref_array(out: _Writer, ids: list, cls: str) -> None:
    _write_array(out, ids, lambda i: _write_ref(out, i, cls))


def _encode_value(out: _Writer, v: Any) -> None:
    match v:
        case None:
            out.u8(_Tag.UNDEFINED)
        case bool():
            _write_bool(out, v)
        case int():
            _write_int(out, v)
        case float():
            _write_double(out, v)
        case str():
            _write_str_value(out, v)
        case Ref():
            _write_ref(out, v.this_id, v.type)
        case list():
            _write_array(out, v, lambda e: _encode_value(out, e))
        case dict():
            out.u8(_Tag.OBJECT)
            out.u32(len(v))
            for k, e in v.items():
                _write_str_value(out, k)
                _encode_value(out, e)
        case _:
            raise RuntimeError(f"cannot encode value: {type(v).__name__}")


def _read_count(buf: _Reader) -> int:
    return buf.u32()
)PY";

static const char *ROOTS_TEMPLATE = R"PY(


class S5RunObject(ABC):
    """Root of all generated classes."""

    @abstractmethod
    def s5run_class_name(self) -> str: ...

    @abstractmethod
    def s5run_method_names(self) -> list[str]: ...


class S5RunFunction(ABC):
    """Root of all generated global functions."""

    @abstractmethod
    def s5run_function_name(self) -> str: ...
)PY";

static const char *REMOTE_SERVER_TEMPLATE = R"PY(


class RemoteServerInfo:
    """Settings of a remote s5run server (fluent)."""

    def __init__(self) -> None:
        self._address = ""
        self._api_key = ""
        self._host = ""
        self._port = 0

    def address(self, address: str) -> "RemoteServerInfo":
        prefix = "s5run://"
        if not address.startswith(prefix):
            raise ValueError("expected format s5run://host:port")
        rest = address[len(prefix):]
        host, _, port = rest.rpartition(":")
        if not host or not port:
            raise ValueError("expected format s5run://host:port")
        self._address = address
        self._host = host
        self._port = int(port)
        return self

    def api_key(self, api_key: str) -> "RemoteServerInfo":
        self._api_key = api_key
        return self

    @property
    def host(self) -> str:
        return self._host

    @property
    def port(self) -> int:
        return self._port
)PY";

static const char *RUN_RESULT_TEMPLATE = R"PY(


@dataclass(frozen=True, slots=True)
class RunResult:
    """Result of a script run."""
    has_error: bool
    error: str = ""
    error_line: int = 0
    duration_ms: float = 0.0
    value: Any = None

    @classmethod
    def ok(cls, value: Any, duration_ms: float) -> "RunResult":
        return cls(False, "", 0, duration_ms, value)

    @classmethod
    def fail(cls, error: str, duration_ms: float) -> "RunResult":
        return cls(True, error, _parse_error_line(error), duration_ms, None)


def _parse_error_line(error: str) -> int:
    """Extracts LINE from ':LINE:COL' positions in engine errors."""
    import re
    m = re.search(r":(\d+):\d+", error)
    return int(m.group(1)) if m else 0
)PY";

// -----------------------------------------------------------------------
// Schema-dependent blocks
// -----------------------------------------------------------------------

static std::string generate_schema_class(const schema_class &c) {
  std::string methods;
  for (const auto &m : c.methods) {
    std::string ret = is_void(m.return_type) ? "" : " -> " + py_type(m.return_type);
    std::string args;
    for (const auto &a : m.arguments)
      args += ", " + a.name + ": " + py_type(a.type);
    methods += "    @abstractmethod\n    def " + m.name + "(self" + args + ")" + ret + ": ...\n";
  }
  std::string names;
  for (const auto &m : c.methods) {
    if (!names.empty())
      names += ", ";
    names += "\"" + m.name + "\"";
  }
  return "\n\n\nclass " + c.name + "(S5RunObject):\n"
         "    \"\"\"Schema class '" + c.name + "' — inherit and implement.\"\"\"\n\n"
         "    def s5run_class_name(self) -> str:\n"
         "        return \"" + c.name + "\"\n\n"
         "    def s5run_method_names(self) -> list[str]:\n"
         "        return [" + names + "]\n\n"
         + methods;
}

static std::string generate_schema_function(const schema_function &f) {
  std::string ret = is_void(f.return_type) ? "" : " -> " + py_type(f.return_type);
  std::string args;
  for (const auto &a : f.arguments)
    args += ", " + a.name + ": " + py_type(a.type);
  return "\n\n\nclass GlobalFunction_" + f.name + "(S5RunFunction):\n"
         "    \"\"\"Global function '" + f.name + "' — inherit and implement execute().\"\"\"\n\n"
         "    def s5run_function_name(self) -> str:\n"
         "        return \"" + f.name + "\"\n\n"
         "    @abstractmethod\n"
         "    def execute(self" + args + ")" + ret + ": ...\n";
}

static std::string generate_context(const schema &s) {
  std::string method_handlers;
  for (const auto &c : s.classes) {
    for (const auto &m : c.methods) {
      if (!method_handlers.empty())
        method_handlers += "\n";
      method_handlers += "            \"" + m.name + "\": self._cbm_" + c.name + "_" + m.name + ",";
    }
  }

  std::string fn_defs;
  bool first_block = true;
  for (const auto &f : s.functions) {
    fn_defs += std::string(first_block ? "\n" : "\n\n")
               + "    def _cb_" + f.name + "(self, cb_id: int, buf: _Reader) -> None:\n"
               "        fn" + f.name + " = self._functions[\"" + f.name + "\"]\n"
               "        out = _Writer()\n"
               "        out.i32(cb_id)\n"
               "        out.u8(1)\n"
               + gen_function_cb(f, "        ") + "\n"
               "        self._send(_Msg.CALLBACK_RESP, out)\n";
    first_block = false;
  }
  for (const auto &c : s.classes) {
    for (const auto &m : c.methods) {
      fn_defs += std::string(first_block ? "\n" : "\n\n")
                 + "    def _cbm_" + c.name + "_" + m.name + "(self, cb_id: int, buf: _Reader) -> None:\n"
                 "        out = _Writer()\n"
                 "        out.i32(cb_id)\n"
                 "        out.u8(1)\n"
                 + gen_method_cb(c, m, "        ") + "\n"
                 "        self._send(_Msg.CALLBACK_RESP, out)\n";
      first_block = false;
    }
  }

  std::string out = R"PY(


class Context:
    """Cacheable execution context: registrations, servers and the script."""

    def __init__(self) -> None:
        self._handlers: dict[str, Callable] = {
)PY";
  out += method_handlers;
  out += R"PY(
        }
        self._functions: dict[str, S5RunFunction] = {}
        self._instances: dict[str, dict[int, S5RunObject]] = {}
        self._servers: list[RemoteServerInfo] = []
        self._pending_types: list[tuple[str, list[str]]] = []
        self._pending_objects: list[tuple[str, int, list[str]]] = []
        self._pending_functions: list[str] = []
        self._script = ""
        self._next_id = 0
        self._sock: socket.socket | None = None

    # ---- fluent registration ----

    def set_script(self, script: str) -> "Context":
        self._script = script
        return self

    def add_server(self, server: RemoteServerInfo) -> "Context":
        self._servers.append(server)
        return self

    def add_class(self, sample: S5RunObject) -> "Context":
        name = sample.s5run_class_name()
        if self._sock is not None:
            self._send_register_type(name, sample.s5run_method_names())
        else:
            self._pending_types.append((name, sample.s5run_method_names()))
        return self

    def add_object(self, obj: S5RunObject) -> "Context":
        cls = obj.s5run_class_name()
        self._next_id += 1
        self._instances.setdefault(cls, {})[self._next_id] = obj
        if self._sock is not None:
            self._send_register_object(cls, self._next_id, obj.s5run_method_names())
        else:
            self._pending_objects.append((cls, self._next_id, obj.s5run_method_names()))
        return self

    def add_function(self, fn: S5RunFunction) -> "Context":
        name = fn.s5run_function_name()
        self._functions[name] = fn
        self._handlers[name] = getattr(self, f"_cb_{name}")
        if self._sock is not None:
            self._send_register_function(name)
        else:
            self._pending_functions.append(name)
        return self

    # ---- internals ----

    def _instance_by_id(self, cls: str, this_id: int) -> Any:
        obj = self._instances.get(cls, {}).get(this_id)
        if obj is None:
            raise RuntimeError(f"{cls}: unknown this_id {this_id}")
        return obj

    def _register_instance(self, obj: S5RunObject) -> int:
        self._next_id += 1
        self._instances.setdefault(obj.s5run_class_name(), {})[self._next_id] = obj
        return self._next_id

)PY";
  out += fn_defs;
  out += R"PY(

    def _dispatch_callback(self, body: bytes) -> None:
        buf = _Reader(body)
        cb_id = buf.i32()
        method = buf.raw_str()
        handler = self._handlers.get(method)
        if handler is None:
            out = _Writer()
            out.i32(cb_id)
            out.u8(0)
            out.raw_str("unknown method: " + method)
            self._send(_Msg.CALLBACK_RESP, out)
            return
        try:
            handler(cb_id, buf)
        except Exception as e:  # noqa: BLE001
            out = _Writer()
            out.i32(cb_id)
            out.u8(0)
            out.raw_str(str(e))
            self._send(_Msg.CALLBACK_RESP, out)

    def _connect(self) -> None:
        if self._sock is not None:
            return
        if not self._servers:
            raise RuntimeError("s5run: no server added (add_server)")
        best = 0
        if len(self._servers) > 1:
            best_load = -1
            for i, srv in enumerate(self._servers):
                load = _probe_server_load(srv)
                if load < 0:
                    continue
                if best_load < 0 or load < best_load:
                    best_load = load
                    best = i
            if best_load < 0:
                raise RuntimeError("s5run: all servers are unreachable")
        srv = self._servers[best]
        sock = socket.create_connection((srv.host, srv.port), timeout=10.0)
        self._sock = sock
        _read_hello(sock)

    def _flush_pending(self) -> None:
        for name, methods in self._pending_types:
            self._send_register_type(name, methods)
        self._pending_types.clear()
        for name, this_id, methods in self._pending_objects:
            self._send_register_object(name, this_id, methods)
        self._pending_objects.clear()
        for name in self._pending_functions:
            self._send_register_function(name)
        self._pending_functions.clear()

    def _send_register_type(self, name: str, methods: list[str]) -> None:
        out = _Writer()
        out.raw_str(name)
        out.u32(len(methods))
        for m in methods:
            out.raw_str(m)
        self._send(_Msg.REGISTER_TYPE, out)
        self._expect_register_resp("register_type(" + name + ")")

    def _send_register_object(self, name: str, this_id: int, methods: list[str]) -> None:
        out = _Writer()
        out.raw_str(name)
        out.i32(this_id)
        out.raw_str(name)
        out.u32(len(methods))
        for m in methods:
            out.raw_str(m)
        self._send(_Msg.REGISTER_OBJ, out)
        self._expect_register_resp("register_object(" + name + ")")

    def _send_register_function(self, name: str) -> None:
        out = _Writer()
        out.raw_str(name)
        self._send(_Msg.REGISTER_FUNC, out)
        self._expect_register_resp("register_function(" + name + ")")

    def _expect_register_resp(self, what: str) -> None:
        msg_type, body = self._read_message()
        if msg_type != _Msg.REGISTER_RESP:
            raise RuntimeError("s5run: " + what + " failed")
        buf = _Reader(body)
        if buf.u8() == 0:
            raise RuntimeError("s5run: " + what + " error: " + buf.raw_str())

    # ---- framing ----

    def _send(self, msg: _Msg, out: _Writer) -> None:
        assert self._sock is not None
        payload = bytes([msg]) + bytes(out.buf)
        self._sock.sendall(struct.pack(">I", len(payload)) + payload)

    def _read_exact(self, n: int) -> bytes:
        assert self._sock is not None
        chunks = bytearray()
        while len(chunks) < n:
            chunk = self._sock.recv(n - len(chunks))
            if not chunk:
                raise RuntimeError("s5run: connection closed")
            chunks += chunk
        return bytes(chunks)

    def _read_message(self) -> tuple[int, bytes]:
        head = self._read_exact(5)
        (length,) = struct.unpack(">I", head[:4])
        if length <= 0 or length > 64 * 1024 * 1024:
            raise RuntimeError("s5run: bad frame length")
        body = self._read_exact(length - 1)
        return head[4], body

    def close(self) -> None:
        if self._sock is not None:
            self._sock.close()
            self._sock = None

    def __enter__(self) -> "Context":
        return self

    def __exit__(self, *_exc: object) -> None:
        self.close()


def _recv_exact(sock: socket.socket, n: int) -> bytes:
    chunks = bytearray()
    while len(chunks) < n:
        chunk = sock.recv(n - len(chunks))
        if not chunk:
            raise RuntimeError("s5run: connection closed")
        chunks += chunk
    return bytes(chunks)


def _read_hello(sock: socket.socket) -> None:
    """Consume the server greeting "s5run <version> <protocol>"."""
    import struct as _struct
    head = _recv_exact(sock, 5)
    (length,) = _struct.unpack(">I", head[:4])
    if head[4] != _Msg.SERVER_HELLO:
        raise RuntimeError("s5run: expected server hello")
    body = sock.recv(length - 1)
    text = body.decode("utf-8")
    parts = text.split(" ")
    if len(parts) != 3 or parts[0] != "s5run":
        raise RuntimeError(f"s5run: bad server hello: {text!r}")
    if int(parts[2]) != 2:
        raise RuntimeError(
            f"s5run: server protocol version {parts[2]} is not supported (client supports 2)")


def _probe_server_load(server: RemoteServerInfo) -> int:
    try:
        with socket.create_connection((server.host, server.port), timeout=2.0) as sock:
            sock.settimeout(2.0)
            payload = bytes([_Msg.STATS_REQ])
            _read_hello(sock)
            sock.sendall(struct.pack(">I", len(payload)) + payload)
            head = _recv_exact(sock, 5)
            (length,) = struct.unpack(">I", head[:4])
            body = _recv_exact(sock, length - 1)
            if head[4] != _Msg.STATS_RESP or len(body) < 16:
                return -1
            buf = _Reader(body)
            buf.u32()  # threads
            busy = buf.u32()
            queued = buf.u32()
            return busy + queued
    except OSError:
        return -1


# module-level helpers used by generated callbacks
_INSTANCE_HOLDER: list[Context] = []


def _instance_by_id(cls: str, this_id: int) -> Any:
    return _INSTANCE_HOLDER[0]._instance_by_id(cls, this_id)


def _register_instance(obj: S5RunObject) -> int:
    return _INSTANCE_HOLDER[0]._register_instance(obj)


def _read_ref_array(buf: _Reader, cls: str) -> list:
    return _read_array(buf, lambda: _instance_by_id(cls, _read_ref(buf).this_id))


def run_script(ctx: Context, entry: str, args: list[Any] | None = None) -> RunResult:
    """Runs the context script; never raises — errors land in RunResult."""
    started = time.monotonic()

    def done(result: RunResult) -> RunResult:
        return RunResult(result.has_error, result.error, result.error_line,
                         (time.monotonic() - started) * 1000.0, result.value)

    try:
        _INSTANCE_HOLDER.clear()
        _INSTANCE_HOLDER.append(ctx)
        ctx._connect()
        ctx._flush_pending()
        out = _Writer()
        out.raw_str(ctx._script)
        out.raw_str(entry)
        out.u32(len(args or []))
        for a in args or []:
            _encode_value(out, a)
        ctx._send(_Msg.EXEC_REQ, out)
        while True:
            msg_type, body = ctx._read_message()
            if msg_type == _Msg.CALLBACK_REQ:
                ctx._dispatch_callback(body)
                continue
            if msg_type == _Msg.EXEC_RESP:
                buf = _Reader(body)
                if buf.u8() == 0:
                    return done(RunResult.fail(buf.raw_str(), 0.0))
                return done(RunResult.ok(_decode_value(buf), 0.0))
            raise RuntimeError("s5run: unexpected message type")
    except Exception as e:  # noqa: BLE001
        return done(RunResult.fail(str(e), 0.0))
)PY";
  return out;
}

// -----------------------------------------------------------------------
// Generator
// -----------------------------------------------------------------------

class generator_python : public IGenerator {
public:
  std::vector<std::string> generate(const schema &s,
                                     const std::string &output_dir) override;
};

std::vector<std::string> generator_python::generate(const schema &s,
                                                    const std::string &output_dir) {
  std::vector<std::string> written;
  std::string module = s.py() ? s.py()->get_module_name() : "";
  if (module.empty())
    module = "s5run_cli";

  ::mkdir(output_dir.c_str(), 0755);
  std::string pkg_dir = output_dir + "/" + module;
  ::mkdir(pkg_dir.c_str(), 0755);

  std::string version = s.client_version;

  // ---- __init__.py: public API ----
  std::string init;
  init += "# AUTO-GENERATED. DO NOT EDIT.\n";
  init += "# package: " + module + "\n";
  init += "\"\"\"Generated s5run client (" + version + "). "
          "Inherit the base classes and register them on a Context.\"\"\"\n\n";
  init += "CLIENT_VERSION = \"" + version + "\"\n\n\n";
  init += "def client_version() -> str:\n";
  init += "    return CLIENT_VERSION\n\n\n";
  init += "from .variable import Ref\n";
  init += "from .types import RemoteServerInfo, RunResult, S5RunFunction, S5RunObject\n";
  for (const auto &c : s.classes)
    init += "from .types import " + c.name + "\n";
  for (const auto &f : s.functions)
    init += "from .types import GlobalFunction_" + f.name + "\n";
  init += "from .context import Context, run_script\n";

  // ---- wire.py: framing primitives ----
  std::string wire;
  wire += "# AUTO-GENERATED. DO NOT EDIT.\n";
  wire += "\n";
  wire += "import struct\n";
  wire += "from enum import IntEnum\n";
  wire += "\n";
  wire += WIRE_PY_TEMPLATE;
  wire += "\n";

  // ---- variable.py: dynamic value codec ----
  std::string variable;
  variable += "# AUTO-GENERATED. DO NOT EDIT.\n";
  variable += "\n";
  variable += "import struct\n";
  variable += "from dataclasses import dataclass\n";
  variable += "from typing import Any, Callable\n";
  variable += "\n";
  variable += "from .wire import _Reader, _Tag, _Writer\n";
  variable += "\n";
  variable += VARIABLE_PY_TEMPLATE;
  variable += "\n";

  // ---- types.py: roots, schema classes, functions, results ----
  std::string types;
  types += "# AUTO-GENERATED. DO NOT EDIT.\n";
  types += "\n";
  types += "from __future__ import annotations\n";
  types += "\n";
  types += "from abc import ABC, abstractmethod\n";
  types += "from dataclasses import dataclass\n";
  types += "from typing import Any\n";
  types += "\n";
  types += ROOTS_TEMPLATE;
  for (const auto &c : s.classes)
    types += generate_schema_class(c);
  for (const auto &f : s.functions)
    types += generate_schema_function(f);
  types += REMOTE_SERVER_TEMPLATE;
  types += RUN_RESULT_TEMPLATE;
  types += "\n";

  // ---- context.py: execution context and callbacks ----
  std::string context;
  context += "# AUTO-GENERATED. DO NOT EDIT.\n";
  context += "\n";
  context += "from __future__ import annotations\n";
  context += "\n";
  context += "import socket\n";
  context += "import struct\n";
  context += "import time\n";
  context += "from typing import Any, Callable\n";
  context += "\n";
  context += "from .types import RemoteServerInfo, RunResult, S5RunFunction, S5RunObject\n";
  context += "from .variable import (\n";
  context += "    Ref, _decode_value, _encode_value, _read_array, _read_bool, _read_byte,\n";
  context += "    _read_count, _read_double, _read_int, _read_ref,\n";
  context += "    _read_str_value, _value_to_string, _write_array, _write_bool, _write_byte,\n";
  context += "    _write_double, _write_int, _write_ref, _write_ref_array,\n";
  context += "    _write_str_value, _write_undefined,\n";
  context += ")\n";
  context += "from .wire import _Msg, _Reader, _Writer\n";
  context += "\n";
  context += generate_context(s);
  context += "\n";

  // write files
  struct file_part {
    const char *name;
    const std::string *body;
  };
  file_part parts[] = {
    {"__init__.py", &init},
    {"wire.py", &wire},
    {"variable.py", &variable},
    {"types.py", &types},
    {"context.py", &context},
  };
  for (const auto &p : parts) {
    std::string path = pkg_dir + "/" + p.name;
    std::ofstream f(path.c_str());
    f << *p.body;
    f.close();
    written.push_back(path);
  }
  return written;
}

std::shared_ptr<IGenerator> make_generator_python() {
  return std::make_shared<generator_python>();
}

} // namespace generators
} // namespace s5run
