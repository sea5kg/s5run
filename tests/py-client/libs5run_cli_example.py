# AUTO-GENERATED. DO NOT EDIT.
# module: libs5run_cli_example
"""Generated s5run client (1.0.0). Inherit the base classes and register them on a Context."""

from __future__ import annotations

import socket
import struct
import time
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import IntEnum
from typing import Any, Callable

CLIENT_VERSION = "1.0.0"


class _Tag(IntEnum):
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


@dataclass(frozen=True, slots=True)
class Ref:
    """Reference to a remote object."""
    this_id: int
    type: str = ""


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


def _read_ref_array(buf: _Reader, cls: str) -> list:
    return _read_array(buf, lambda: _instance_by_id(cls, _read_ref(buf).this_id))


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



class File(S5RunObject):
    """Schema class 'File' — inherit and implement."""

    def s5run_class_name(self) -> str:
        return "File"

    def s5run_method_names(self) -> list[str]:
        return ["GetSize", "ReadBytes"]

    @abstractmethod
    def GetSize(self) -> int: ...
    @abstractmethod
    def ReadBytes(self, readBytes: int) -> list[int]: ...



class FileSystem(S5RunObject):
    """Schema class 'FileSystem' — inherit and implement."""

    def s5run_class_name(self) -> str:
        return "FileSystem"

    def s5run_method_names(self) -> list[str]:
        return ["GetList"]

    @abstractmethod
    def GetList(self, filepath: str) -> list[str]: ...



class console(S5RunObject):
    """Schema class 'console' — inherit and implement."""

    def s5run_class_name(self) -> str:
        return "console"

    def s5run_method_names(self) -> list[str]:
        return ["log", "warning", "error"]

    @abstractmethod
    def log(self, args: list[str]): ...
    @abstractmethod
    def warning(self, args: list[str]): ...
    @abstractmethod
    def error(self, args: list[str]): ...



class GlobalFunction_GetFile(S5RunFunction):
    """Global function 'GetFile' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "GetFile"

    @abstractmethod
    def execute(self, filepath: str) -> File: ...



class GlobalFunction_GetFileSize(S5RunFunction):
    """Global function 'GetFileSize' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "GetFileSize"

    @abstractmethod
    def execute(self, file: File) -> int: ...



class GlobalFunction_GetTimeNow(S5RunFunction):
    """Global function 'GetTimeNow' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "GetTimeNow"

    @abstractmethod
    def execute(self) -> int: ...



class GlobalFunction_HasFile(S5RunFunction):
    """Global function 'HasFile' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "HasFile"

    @abstractmethod
    def execute(self, filepath: str) -> bool: ...



class GlobalFunction_GetFileList(S5RunFunction):
    """Global function 'GetFileList' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "GetFileList"

    @abstractmethod
    def execute(self, filepath: str) -> list[str]: ...



class GlobalFunction_GetOpenedFiles(S5RunFunction):
    """Global function 'GetOpenedFiles' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "GetOpenedFiles"

    @abstractmethod
    def execute(self) -> list[File]: ...



class GlobalFunction_CloseFiles(S5RunFunction):
    """Global function 'CloseFiles' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "CloseFiles"

    @abstractmethod
    def execute(self, files: list[File]): ...



class GlobalFunction_GetPi(S5RunFunction):
    """Global function 'GetPi' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "GetPi"

    @abstractmethod
    def execute(self) -> float: ...



class GlobalFunction_GetServerPort(S5RunFunction):
    """Global function 'GetServerPort' — inherit and implement execute()."""

    def s5run_function_name(self) -> str:
        return "GetServerPort"

    @abstractmethod
    def execute(self) -> int: ...



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



class Context:
    """Cacheable execution context: registrations, servers and the script."""

    def __init__(self) -> None:
        self._handlers: dict[str, Callable] = {
            "GetSize": self._cbm_File_GetSize,
            "ReadBytes": self._cbm_File_ReadBytes,
            "GetList": self._cbm_FileSystem_GetList,
            "log": self._cbm_console_log,
            "warning": self._cbm_console_warning,
            "error": self._cbm_console_error,
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


    def _cb_GetFile(self, cb_id: int, buf: _Reader) -> None:
        fnGetFile = self._functions["GetFile"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 1:
            raise RuntimeError("GetFile: wrong argument count")
        filepath = _read_str_value(buf)
        res = fnGetFile.execute(filepath)
        _write_ref(out, _register_instance(res), 'File')
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_GetFileSize(self, cb_id: int, buf: _Reader) -> None:
        fnGetFileSize = self._functions["GetFileSize"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 1:
            raise RuntimeError("GetFileSize: wrong argument count")
        file = _instance_by_id('File', _read_ref(buf).this_id)
        res = fnGetFileSize.execute(file)
        _write_int(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_GetTimeNow(self, cb_id: int, buf: _Reader) -> None:
        fnGetTimeNow = self._functions["GetTimeNow"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 0:
            raise RuntimeError("GetTimeNow: wrong argument count")
        res = fnGetTimeNow.execute()
        _write_int(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_HasFile(self, cb_id: int, buf: _Reader) -> None:
        fnHasFile = self._functions["HasFile"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 1:
            raise RuntimeError("HasFile: wrong argument count")
        filepath = _read_str_value(buf)
        res = fnHasFile.execute(filepath)
        _write_bool(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_GetFileList(self, cb_id: int, buf: _Reader) -> None:
        fnGetFileList = self._functions["GetFileList"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 1:
            raise RuntimeError("GetFileList: wrong argument count")
        filepath = _read_str_value(buf)
        res = fnGetFileList.execute(filepath)
        _write_array(out, res, lambda e: _write_str_value(out, e))
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_GetOpenedFiles(self, cb_id: int, buf: _Reader) -> None:
        fnGetOpenedFiles = self._functions["GetOpenedFiles"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 0:
            raise RuntimeError("GetOpenedFiles: wrong argument count")
        res = fnGetOpenedFiles.execute()
        ids = [_register_instance(o) for o in res]
        _write_ref_array(out, ids, 'File')
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_CloseFiles(self, cb_id: int, buf: _Reader) -> None:
        fnCloseFiles = self._functions["CloseFiles"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 1:
            raise RuntimeError("CloseFiles: wrong argument count")
        files = _read_ref_array(buf, 'File')
        fnCloseFiles.execute(files)
        _write_undefined(out)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_GetPi(self, cb_id: int, buf: _Reader) -> None:
        fnGetPi = self._functions["GetPi"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 0:
            raise RuntimeError("GetPi: wrong argument count")
        res = fnGetPi.execute()
        _write_double(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cb_GetServerPort(self, cb_id: int, buf: _Reader) -> None:
        fnGetServerPort = self._functions["GetServerPort"]
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        if _read_count(buf) != 0:
            raise RuntimeError("GetServerPort: wrong argument count")
        res = fnGetServerPort.execute()
        _write_int(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_File_GetSize(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 1:
            raise RuntimeError("File.GetSize: wrong argument count")
        obj = _instance_by_id('File', _read_int(buf))
        res = obj.GetSize()
        _write_int(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_File_ReadBytes(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 2:
            raise RuntimeError("File.ReadBytes: wrong argument count")
        obj = _instance_by_id('File', _read_int(buf))
        readBytes = _read_int(buf)
        res = obj.ReadBytes(readBytes)
        _write_array(out, res, lambda e: _write_byte(out, e))
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_FileSystem_GetList(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 2:
            raise RuntimeError("FileSystem.GetList: wrong argument count")
        obj = _instance_by_id('FileSystem', _read_int(buf))
        filepath = _read_str_value(buf)
        res = obj.GetList(filepath)
        _write_array(out, res, lambda e: _write_str_value(out, e))
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_console_log(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count < 1:
            raise RuntimeError("console.log: expected this_id")
        obj = _instance_by_id('console', _read_int(buf))
        args = []
        for _ in range(n_count - 1):
            args.append(_value_to_string(_decode_value(buf)))
        obj.log(args)
        _write_undefined(out)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_console_warning(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count < 1:
            raise RuntimeError("console.warning: expected this_id")
        obj = _instance_by_id('console', _read_int(buf))
        args = []
        for _ in range(n_count - 1):
            args.append(_value_to_string(_decode_value(buf)))
        obj.warning(args)
        _write_undefined(out)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_console_error(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count < 1:
            raise RuntimeError("console.error: expected this_id")
        obj = _instance_by_id('console', _read_int(buf))
        args = []
        for _ in range(n_count - 1):
            args.append(_value_to_string(_decode_value(buf)))
        obj.error(args)
        _write_undefined(out)
        self._send(_Msg.CALLBACK_RESP, out)


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


def _probe_server_load(server: RemoteServerInfo) -> int:
    try:
        with socket.create_connection((server.host, server.port), timeout=2.0) as sock:
            sock.settimeout(2.0)
            payload = bytes([_Msg.STATS_REQ])
            sock.sendall(struct.pack(">I", len(payload)) + payload)
            head = sock.recv(5)
            if len(head) < 5:
                return -1
            (length,) = struct.unpack(">I", head[:4])
            body = sock.recv(length - 1)
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


def client_version() -> str:
    return CLIENT_VERSION


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

