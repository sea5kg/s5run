# AUTO-GENERATED. DO NOT EDIT.

from __future__ import annotations

import socket
import struct
import time
from typing import Any, Callable

from .types import RemoteServerInfo, RunResult, S5RunFunction, S5RunObject
from .variable import (
    Ref, _decode_value, _encode_value, _read_array, _read_bool, _read_byte,
    _read_count, _read_double, _read_int, _read_ref,
    _read_str_value, _value_to_string, _write_array, _write_bool, _write_byte,
    _write_double, _write_int, _write_ref, _write_ref_array,
    _write_str_value, _write_undefined,
)
from .wire import _Msg, _Reader, _Writer

S5RUN_PROTOCOL_VERSION = 3




class Context:
    """Cacheable execution context: registrations, servers and the script."""

    def __init__(self) -> None:
        self._class_samples: dict[str, S5RunObject] = {}
        self._handlers: dict[str, Callable] = {
            "GetSize": self._cbm_File_GetSize,
            "ReadBytes": self._cbm_File_ReadBytes,
            "GetName": self._cbm_File_GetName,
            "GetBufferSize": self._cbm_File_GetBufferSize,
            "SetBufferSize": self._cbm_File_SetBufferSize,
            "File": self._ctor_File,
            "GetList": self._cbm_FileSystem_GetList,
            "FileSystem": self._ctor_FileSystem,
            "log": self._cbm_console_log,
            "warning": self._cbm_console_warning,
            "error": self._cbm_console_error,
            "console": self._ctor_console,
            "SetValueByKey": self._cbm_KeyValueStorage_SetValueByKey,
            "GetValueByKey": self._cbm_KeyValueStorage_GetValueByKey,
            "KeyValueStorage": self._ctor_KeyValueStorage,
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
        self._class_samples[name] = sample
        if self._sock is not None:
            self._send_register_type(name, sample.s5run_method_names(),
                                     sample.s5run_property_descriptors())
        else:
            self._pending_types.append((name, sample.s5run_method_names(),
                                        sample.s5run_property_descriptors()))
        return self

    def add_object(self, obj: S5RunObject) -> "Context":
        cls = obj.s5run_class_name()
        self._next_id += 1
        self._instances.setdefault(cls, {})[self._next_id] = obj
        if self._sock is not None:
            self._send_register_object(cls, self._next_id, obj.s5run_method_names(),
                                       obj.s5run_property_descriptors())
        else:
            self._pending_objects.append((cls, self._next_id, obj.s5run_method_names(),
                                          obj.s5run_property_descriptors()))
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


    def _cbm_File_GetName(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 1:
            raise RuntimeError("File.GetName: wrong argument count")
        obj = _instance_by_id('File', _read_int(buf))
        res = obj.GetName()
        _write_str_value(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_File_GetBufferSize(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 1:
            raise RuntimeError("File.GetBufferSize: wrong argument count")
        obj = _instance_by_id('File', _read_int(buf))
        res = obj.GetBufferSize()
        _write_int(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_File_SetBufferSize(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 2:
            raise RuntimeError("File.SetBufferSize: wrong argument count")
        obj = _instance_by_id('File', _read_int(buf))
        bufferSize = _read_int(buf)
        obj.SetBufferSize(bufferSize)
        _write_undefined(out)
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


    def _cbm_KeyValueStorage_SetValueByKey(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 3:
            raise RuntimeError("KeyValueStorage.SetValueByKey: wrong argument count")
        obj = _instance_by_id('KeyValueStorage', _read_int(buf))
        name = _read_str_value(buf)
        value = _decode_value(buf)
        obj.SetValueByKey(name, value)
        _write_undefined(out)
        self._send(_Msg.CALLBACK_RESP, out)


    def _cbm_KeyValueStorage_GetValueByKey(self, cb_id: int, buf: _Reader) -> None:
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        n_count = _read_count(buf)
        if n_count != 2:
            raise RuntimeError("KeyValueStorage.GetValueByKey: wrong argument count")
        obj = _instance_by_id('KeyValueStorage', _read_int(buf))
        name = _read_str_value(buf)
        res = obj.GetValueByKey(name)
        _encode_value(out, res)
        self._send(_Msg.CALLBACK_RESP, out)


    def _ctor_File(self, cb_id: int, buf: _Reader) -> None:
        # 'new File()' from a script
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        sample = self._class_samples.get("File")
        if sample is None:
            raise RuntimeError("File: no class sample registered (add_class)")
        inst = sample.s5run_construct()
        if inst is None:
            raise RuntimeError("File: not constructible (override s5run_construct)")
        obj_id = self._register_instance(inst)
        out.u8(0x0F) # T_REF
        out.i32(obj_id)
        out.raw_str("File")
        self._send(_Msg.CALLBACK_RESP, out)


    def _ctor_FileSystem(self, cb_id: int, buf: _Reader) -> None:
        # 'new FileSystem()' from a script
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        sample = self._class_samples.get("FileSystem")
        if sample is None:
            raise RuntimeError("FileSystem: no class sample registered (add_class)")
        inst = sample.s5run_construct()
        if inst is None:
            raise RuntimeError("FileSystem: not constructible (override s5run_construct)")
        obj_id = self._register_instance(inst)
        out.u8(0x0F) # T_REF
        out.i32(obj_id)
        out.raw_str("FileSystem")
        self._send(_Msg.CALLBACK_RESP, out)


    def _ctor_console(self, cb_id: int, buf: _Reader) -> None:
        # 'new console()' from a script
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        sample = self._class_samples.get("console")
        if sample is None:
            raise RuntimeError("console: no class sample registered (add_class)")
        inst = sample.s5run_construct()
        if inst is None:
            raise RuntimeError("console: not constructible (override s5run_construct)")
        obj_id = self._register_instance(inst)
        out.u8(0x0F) # T_REF
        out.i32(obj_id)
        out.raw_str("console")
        self._send(_Msg.CALLBACK_RESP, out)


    def _ctor_KeyValueStorage(self, cb_id: int, buf: _Reader) -> None:
        # 'new KeyValueStorage()' from a script
        out = _Writer()
        out.i32(cb_id)
        out.u8(1)
        sample = self._class_samples.get("KeyValueStorage")
        if sample is None:
            raise RuntimeError("KeyValueStorage: no class sample registered (add_class)")
        inst = sample.s5run_construct()
        if inst is None:
            raise RuntimeError("KeyValueStorage: not constructible (override s5run_construct)")
        obj_id = self._register_instance(inst)
        out.u8(0x0F) # T_REF
        out.i32(obj_id)
        out.raw_str("KeyValueStorage")
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
        _read_hello(sock)

    def _flush_pending(self) -> None:
        for name, methods, props in self._pending_types:
            self._send_register_type(name, methods, props)
        self._pending_types.clear()
        for name, this_id, methods, props in self._pending_objects:
            self._send_register_object(name, this_id, methods, props)
        self._pending_objects.clear()
        for name in self._pending_functions:
            self._send_register_function(name)
        self._pending_functions.clear()

    def _send_register_type(self, name: str, methods: list[str],
                            props: list[tuple[str, str, str]]) -> None:
        out = _Writer()
        out.raw_str(name)
        out.u32(len(methods))
        for m in methods:
            out.raw_str(m)
        out.u32(len(props))
        for (pn, pg, ps) in props:
            out.raw_str(pn)
            out.raw_str(pg)
            out.raw_str(ps)
        self._send(_Msg.REGISTER_TYPE, out)
        self._expect_register_resp("register_type(" + name + ")")

    def _send_register_object(self, name: str, this_id: int, methods: list[str],
                              props: list[tuple[str, str, str]]) -> None:
        out = _Writer()
        out.raw_str(name)
        out.i32(this_id)
        out.raw_str(name)
        out.u32(len(methods))
        for m in methods:
            out.raw_str(m)
        out.u32(len(props))
        for (pn, pg, ps) in props:
            out.raw_str(pn)
            out.raw_str(pg)
            out.raw_str(ps)
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
    if int(parts[2]) != S5RUN_PROTOCOL_VERSION:
        raise RuntimeError(
            f"s5run: server protocol version {parts[2]} is not supported (client supports {S5RUN_PROTOCOL_VERSION})")


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

