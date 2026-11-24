# AUTO-GENERATED. DO NOT EDIT.

import struct
from enum import IntEnum

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

