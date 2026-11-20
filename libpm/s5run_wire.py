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

"""
Wire codec and protocol helpers for s5run smoke tests.
"""

import socket
import struct

M_EXEC_REQ = 0x01
M_EXEC_RESP = 0x02
M_CALLBACK_REQ = 0x03
M_CALLBACK_RESP = 0x04
M_REGISTER_FUNC = 0x05
M_REGISTER_OBJ = 0x06
M_REGISTER_TYPE = 0x07
M_REGISTER_RESP = 0x08
M_STATS_REQ = 0x09
M_STATS_RESP = 0x0A
M_JOB_SUBMIT = 0x0B
M_JOB_SUBMIT_RESP = 0x0C
M_JOB_RESULT_REQ = 0x0D
M_JOB_RESULT_RESP = 0x0E
M_SERVER_HELLO = 0x0F

SUPPORTED_PROTOCOL_VERSION = 2

T_UNDEF = 0x00
T_BOOL = 0x01
T_INT8 = 0x02
T_INT16 = 0x03
T_INT32 = 0x04
T_INT64 = 0x05
T_UINT8 = 0x06
T_UINT16 = 0x07
T_UINT32 = 0x08
T_UINT64 = 0x09
T_FLOAT32 = 0x0A
T_FLOAT64 = 0x0B
T_STRING = 0x0C
T_ARRAY = 0x0D
T_OBJECT = 0x0E
T_REF = 0x0F

S_LEN_1 = 0x00
S_LEN_2 = 0x01
S_LEN_4 = 0x02


class WireError(Exception):
    """ WireError """
    pass


class Writer:
    """ Writer """
    def __init__(self):
        self.b = bytearray()

    def u8(self, v):
        self.b.append(v & 0xFF)

    def u16(self, v):
        self.b += struct.pack('<H', v & 0xFFFF)

    def u32(self, v):
        self.b += struct.pack('<I', v & 0xFFFFFFFF)

    def i32(self, v):
        self.b += struct.pack('<i', v)

    def s(self, v):
        raw = v.encode()
        if len(raw) <= 0xFF:
            self.b.append(S_LEN_1)
            self.u8(len(raw))
        elif len(raw) <= 0xFFFF:
            self.b.append(S_LEN_2)
            self.u16(len(raw))
        else:
            self.b.append(S_LEN_4)
            self.u32(len(raw))
        self.b += raw

    def val(self, v):
        if v is None:
            self.b.append(T_UNDEF)
        elif isinstance(v, bool):
            self.b.append(T_BOOL)
            self.b.append(1 if v else 0)
        elif isinstance(v, int):
            if -(2 ** 31) <= v < 2 ** 31:
                self.b.append(T_INT32)
                self.b += struct.pack('<i', v)
            else:
                self.b.append(T_INT64)
                self.b += struct.pack('<q', v)
        elif isinstance(v, float):
            self.b.append(T_FLOAT64)
            self.b += struct.pack('<d', v)
        elif isinstance(v, str):
            self.b.append(T_STRING)
            self.s(v)
        elif isinstance(v, list):
            self.b.append(T_ARRAY)
            self.u32(len(v))
            for e in v:
                self.val(e)
        elif isinstance(v, Ref):
            self.b.append(T_REF)
            self.i32(v.this_id)
            self.s(v.type)
        else:
            raise WireError("cannot encode: %r" % (v,))


class Ref:
    def __init__(self, this_id, type):
        self.this_id = this_id
        self.type = type

    def __repr__(self):
        return "Ref(%d, %r)" % (self.this_id, self.type)


class Reader:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def u8(self):
        v = self.d[self.p]
        self.p += 1
        return v

    def u16(self):
        v = struct.unpack_from('<H', self.d, self.p)[0]
        self.p += 2
        return v

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.p)[0]
        self.p += 4
        return v

    def i32(self):
        v = struct.unpack_from('<i', self.d, self.p)[0]
        self.p += 4
        return v

    def s(self):
        sz = self.u8()
        if sz == S_LEN_1:
            n = self.u8()
        elif sz == S_LEN_2:
            n = self.u16()
        elif sz == S_LEN_4:
            n = self.u32()
        else:
            raise WireError("bad string length size %02X" % sz)
        v = self.d[self.p:self.p + n].decode()
        self.p += n
        return v

    def val(self):
        t = self.u8()
        if t == T_UNDEF:
            return None
        if t == T_BOOL:
            return self.u8() != 0
        if t == T_INT8:
            v = struct.unpack_from('<b', self.d, self.p)[0]
            self.p += 1
            return v
        if t == T_INT16:
            v = struct.unpack_from('<h', self.d, self.p)[0]
            self.p += 2
            return v
        if t == T_INT32:
            return self.i32()
        if t == T_INT64:
            v = struct.unpack_from('<q', self.d, self.p)[0]
            self.p += 8
            return v
        if t == T_UINT8:
            return self.u8()
        if t == T_UINT16:
            return self.u16()
        if t == T_UINT32:
            return self.u32()
        if t == T_UINT64:
            v = struct.unpack_from('<Q', self.d, self.p)[0]
            self.p += 8
            return v
        if t == T_FLOAT32:
            v = struct.unpack_from('<f', self.d, self.p)[0]
            self.p += 4
            return v
        if t == T_FLOAT64:
            v = struct.unpack_from('<d', self.d, self.p)[0]
            self.p += 8
            return v
        if t == T_STRING:
            return self.s()
        if t == T_ARRAY:
            return [self.val() for _ in range(self.u32())]
        if t == T_OBJECT:
            out = {}
            for _ in range(self.u32()):
                kt = self.u8()
                if kt != T_STRING:
                    raise WireError("bad object key tag %02X" % kt)
                k = self.s()
                out[k] = self.val()
            return out
        if t == T_REF:
            this_id = self.i32()
            return Ref(this_id, self.s())
        raise WireError("bad value tag %02X" % t)


def connect(host, port):
    """Connect and consume the server hello; returns the socket."""
    sock = socket.create_connection((host, port))
    read_hello(sock)
    return sock


def read_hello(sock):
    """Consume the server greeting: "s5run <version> <protocol>".

    Returns (server_version, protocol_version); raises on mismatch."""
    t, r = recv_msg(sock)
    if t != M_SERVER_HELLO:
        raise WireError("expected server hello, got msg type %d" % t)
    text = r.d[r.p:].decode("utf-8")
    parts = text.split(" ")
    if len(parts) != 3 or parts[0] != "s5run":
        raise WireError("bad server hello: %r" % text)
    protocol = int(parts[2])
    if protocol != SUPPORTED_PROTOCOL_VERSION:
        raise WireError("server protocol %d, client supports %d"
                        % (protocol, SUPPORTED_PROTOCOL_VERSION))
    return parts[1], protocol


def send_msg(sock, msg_type, body):
    frame = bytes([msg_type]) + bytes(body)
    sock.sendall(struct.pack('>I', len(frame)) + frame)


def recv_msg(sock):
    raw = b''
    while len(raw) < 4:
        c = sock.recv(4 - len(raw))
        if not c:
            raise WireError("eof while reading frame length")
        raw += c
    ln = struct.unpack('>I', raw)[0]
    data = b''
    while len(data) < ln:
        c = sock.recv(ln - len(data))
        if not c:
            raise WireError("eof while reading frame body")
        data += c
    return data[0], Reader(data[1:])


def request_stats(sock):
    """Ask the server for load stats: (threads, busy, queued, clients)."""
    send_msg(sock, M_STATS_REQ, [])
    t, r = recv_msg(sock)
    if t != M_STATS_RESP:
        raise WireError("expected stats response, got msg type %d" % t)
    return r.u32(), r.u32(), r.u32(), r.u32()


def submit_job(sock, script, entry, args):
    """Submit a queue job; returns its uuid."""
    w = Writer()
    w.s(script)
    w.s(entry)
    w.u32(len(args))
    for a in args:
        w.val(a)
    send_msg(sock, M_JOB_SUBMIT, w.b)
    t, r = recv_msg(sock)
    if t != M_JOB_SUBMIT_RESP:
        raise WireError("expected job submit response, got %d" % t)
    if r.u8() != 1:
        raise WireError("job submit failed: " + r.s())
    return r.s()


def get_job_result(sock, uuid):
    """Ask for a job status: (status, ok, value|error); status: 0/1/2/3/255."""
    w = Writer()
    w.s(uuid)
    send_msg(sock, M_JOB_RESULT_REQ, w.b)
    t, r = recv_msg(sock)
    if t != M_JOB_RESULT_RESP:
        raise WireError("expected job result response, got %d" % t)
    status = r.u8()
    if status in (2, 3):
        ok = r.u8()
        return status, ok, (r.val() if ok else r.s())
    return status, None, None


def _expect_register_resp(sock, what):
    t, r = recv_msg(sock)
    if t != M_REGISTER_RESP:
        raise WireError("%s: unexpected msg type %d" % (what, t))
    ok = r.u8()
    err = r.s()
    if not ok:
        raise WireError("%s failed: %s" % (what, err))


def register_func(sock, name):
    w = Writer()
    w.s(name)
    send_msg(sock, M_REGISTER_FUNC, w.b)
    _expect_register_resp(sock, "register_func(%r)" % name)


def register_object_type(sock, name, methods):
    w = Writer()
    w.s(name)
    w.u32(len(methods))
    for m in methods:
        w.s(m)
    send_msg(sock, M_REGISTER_TYPE, w.b)
    _expect_register_resp(sock, "register_object_type(%r)" % name)


def register_object(sock, name, this_id, type_name, methods):
    w = Writer()
    w.s(name)
    w.i32(this_id)
    w.s(type_name)
    w.u32(len(methods))
    for m in methods:
        w.s(m)
    send_msg(sock, M_REGISTER_OBJ, w.b)
    _expect_register_resp(sock, "register_object(%r)" % name)


def exec_script(sock, script, entry, args):
    w = Writer()
    w.s(script)
    w.s(entry)
    w.u32(len(args))
    for a in args:
        w.val(a)
    send_msg(sock, M_EXEC_REQ, w.b)


def recv_callback(sock):
    t, r = recv_msg(sock)
    if t != M_CALLBACK_REQ:
        raise WireError(f"expected callback, got msg type {t}")
    cid = r.i32()
    method = r.s()
    args = [r.val() for _ in range(r.u32())]
    return cid, method, args


def send_callback_result(sock, cid, value):
    w = Writer()
    w.i32(cid)
    w.u8(1)
    w.val(value)
    send_msg(sock, M_CALLBACK_RESP, w.b)


def send_callback_error(sock, cid, error):
    w = Writer()
    w.i32(cid)
    w.u8(0)
    w.s(error)
    send_msg(sock, M_CALLBACK_RESP, w.b)


def recv_exec_result(sock):
    """Serve incoming callbacks with `dispatch` until M_EXEC_RESP arrives.

    dispatch(method, args) must return the result value or raise
    CallbackError(message)."""
    t, r = recv_msg(sock)
    if t != M_EXEC_RESP:
        raise WireError(f"expected exec result, got msg type {t}")
    ok = r.u8()
    if ok:
        return True, r.val()
    return False, r.s()
