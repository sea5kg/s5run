# AUTO-GENERATED. DO NOT EDIT.

import struct
from dataclasses import dataclass
from typing import Any, Callable

from .wire import _Reader, _Tag, _Writer

@dataclass(frozen=True, slots=True)
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

