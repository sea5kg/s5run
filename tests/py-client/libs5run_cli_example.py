# AUTO-GENERATED. DO NOT EDIT.
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import List, Optional, Protocol
import struct

# module: libs5run_cli_example
CLIENT_VERSION = "1.0.0"

class IFile(Protocol):
    this_id: int
    def GetSize(self) -> int: ...
    def ReadBytes(self, readBytes: int) -> List[int]: ...

class IFileSystem(Protocol):
    this_id: int
    def GetList(self, filepath: str) -> List[str]: ...

class Iconsole(Protocol):
    this_id: int
    def log(self, args: List[str]) -> "void": ...
    def warning(self, args: List[str]) -> "void": ...
    def error(self, args: List[str]) -> "void": ...

class IRunScriptGlobalFunctions(ABC):
    @abstractmethod
    def GetFile(self, filepath: str) -> "File": ...
    @abstractmethod
    def GetFileSize(self, file: "File") -> int: ...
    @abstractmethod
    def GetTimeNow(self) -> int: ...
    @abstractmethod
    def HasFile(self, filepath: str) -> bool: ...
    @abstractmethod
    def GetFileList(self, filepath: str) -> List[str]: ...
    @abstractmethod
    def GetOpenedFiles(self) -> List["File"]: ...
    @abstractmethod
    def CloseFiles(self, files: List["File"]) -> "void": ...
    @abstractmethod
    def GetPi(self) -> float: ...
    @abstractmethod
    def GetServerPort(self) -> int: ...

# ---- wire codec ----

def _put_u8(b, v):   b.append(v & 0xFF)
def _put_bool(b, v): b.append(1 if v else 0)
def _put_u32(b, v):  b += struct.pack('<I', v & 0xFFFFFFFF)
def _put_u64(b, v):  b += struct.pack('<Q', v & 0xFFFFFFFFFFFFFFFF)
def _put_f64(b, v):  b += struct.pack('<d', v)
def _put_str(b, s):
    raw = s.encode('utf-8')
    _put_u32(b, len(raw)); b += raw

class _Reader:
    def __init__(self, data): self.p = 0; self.data = data
    def u8(self):   v = self.data[self.p]; self.p += 1; return v
    def bool(self): return self.u8() != 0
    def u32(self):  v = struct.unpack_from('<I', self.data, self.p)[0]; self.p += 4; return v
    def u64(self):  v = struct.unpack_from('<Q', self.data, self.p)[0]; self.p += 8; return v
    def f64(self):  v = struct.unpack_from('<d', self.data, self.p)[0]; self.p += 8; return v
    def str(self):
        n = self.u32(); v = self.data[self.p:self.p+n].decode('utf-8'); self.p += n; return v

@dataclass
class GetFileRequest:
    filepath: str

    def encode(self) -> bytes:
        b = bytearray()
        _put_str(b, self.filepath)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetFileRequest':
        r = _Reader(data)
        return GetFileRequest(filepath=r.str())

@dataclass
class GetFileSizeRequest:
    file: "File"

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.file.this_id)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetFileSizeRequest':
        r = _Reader(data)
        return GetFileSizeRequest(file=r.u64())

@dataclass
class GetFileSizeResponse:
    value: int

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.value)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetFileSizeResponse':
        r = _Reader(data)
        return GetFileSizeResponse(value=r.u64())

@dataclass
class GetTimeNowRequest:
    pass

    def encode(self) -> bytes:
        b = bytearray()
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetTimeNowRequest':
        r = _Reader(data)
        return GetTimeNowRequest()

@dataclass
class GetTimeNowResponse:
    value: int

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.value)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetTimeNowResponse':
        r = _Reader(data)
        return GetTimeNowResponse(value=r.u64())

@dataclass
class HasFileRequest:
    filepath: str

    def encode(self) -> bytes:
        b = bytearray()
        _put_str(b, self.filepath)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'HasFileRequest':
        r = _Reader(data)
        return HasFileRequest(filepath=r.str())

@dataclass
class HasFileResponse:
    value: bool

    def encode(self) -> bytes:
        b = bytearray()
        _put_bool(b, self.value)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'HasFileResponse':
        r = _Reader(data)
        return HasFileResponse(value=r.bool())

@dataclass
class GetFileListRequest:
    filepath: str

    def encode(self) -> bytes:
        b = bytearray()
        _put_str(b, self.filepath)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetFileListRequest':
        r = _Reader(data)
        return GetFileListRequest(filepath=r.str())

@dataclass
class GetFileListResponse:
    value: List[str]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u32(b, len(self.value))
        for _e in self.value:
            _put_str(b, _e)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetFileListResponse':
        r = _Reader(data)
        return GetFileListResponse(value=[r.str() for _ in range(r.u32())])

@dataclass
class GetOpenedFilesRequest:
    pass

    def encode(self) -> bytes:
        b = bytearray()
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetOpenedFilesRequest':
        r = _Reader(data)
        return GetOpenedFilesRequest()

@dataclass
class GetOpenedFilesResponse:
    value: List["File"]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u32(b, len(self.value))
        for _e in self.value:
            _put_u64(b, _e.this_id)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetOpenedFilesResponse':
        r = _Reader(data)
        return GetOpenedFilesResponse(value=[r.u64() for _ in range(r.u32())])

@dataclass
class CloseFilesRequest:
    files: List["File"]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u32(b, len(self.files))
        for _e in self.files:
            _put_u64(b, _e.this_id)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'CloseFilesRequest':
        r = _Reader(data)
        return CloseFilesRequest(files=[r.u64() for _ in range(r.u32())])

@dataclass
class GetPiRequest:
    pass

    def encode(self) -> bytes:
        b = bytearray()
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetPiRequest':
        r = _Reader(data)
        return GetPiRequest()

@dataclass
class GetPiResponse:
    value: float

    def encode(self) -> bytes:
        b = bytearray()
        _put_f64(b, self.value)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetPiResponse':
        r = _Reader(data)
        return GetPiResponse(value=r.f64())

@dataclass
class GetServerPortRequest:
    pass

    def encode(self) -> bytes:
        b = bytearray()
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetServerPortRequest':
        r = _Reader(data)
        return GetServerPortRequest()

@dataclass
class GetServerPortResponse:
    value: int

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.value)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetServerPortResponse':
        r = _Reader(data)
        return GetServerPortResponse(value=r.u64())

@dataclass
class GetSizeRequest:
    this_id: int
    pass

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.this_id)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetSizeRequest':
        r = _Reader(data)
        return GetSizeRequest(this_id=r.u64())

@dataclass
class GetSizeResponse:
    value: int

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.value)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetSizeResponse':
        r = _Reader(data)
        return GetSizeResponse(value=r.u64())

@dataclass
class ReadBytesRequest:
    this_id: int
    readBytes: int

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.this_id)
        _put_u64(b, self.readBytes)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'ReadBytesRequest':
        r = _Reader(data)
        return ReadBytesRequest(this_id=r.u64(), readBytes=r.u64())

@dataclass
class ReadBytesResponse:
    value: List[int]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u32(b, len(self.value))
        for _e in self.value:
            _put_u8(b, _e)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'ReadBytesResponse':
        r = _Reader(data)
        return ReadBytesResponse(value=[r.u8() for _ in range(r.u32())])

@dataclass
class GetListRequest:
    this_id: int
    filepath: str

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.this_id)
        _put_str(b, self.filepath)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetListRequest':
        r = _Reader(data)
        return GetListRequest(this_id=r.u64(), filepath=r.str())

@dataclass
class GetListResponse:
    value: List[str]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u32(b, len(self.value))
        for _e in self.value:
            _put_str(b, _e)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'GetListResponse':
        r = _Reader(data)
        return GetListResponse(value=[r.str() for _ in range(r.u32())])

@dataclass
class logRequest:
    this_id: int
    args: List[str]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.this_id)
        _put_u32(b, len(self.args))
        for _e in self.args:
            _put_str(b, _e)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'logRequest':
        r = _Reader(data)
        return logRequest(this_id=r.u64(), args=[r.str() for _ in range(r.u32())])

@dataclass
class warningRequest:
    this_id: int
    args: List[str]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.this_id)
        _put_u32(b, len(self.args))
        for _e in self.args:
            _put_str(b, _e)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'warningRequest':
        r = _Reader(data)
        return warningRequest(this_id=r.u64(), args=[r.str() for _ in range(r.u32())])

@dataclass
class errorRequest:
    this_id: int
    args: List[str]

    def encode(self) -> bytes:
        b = bytearray()
        _put_u64(b, self.this_id)
        _put_u32(b, len(self.args))
        for _e in self.args:
            _put_str(b, _e)
        return bytes(b)

    @staticmethod
    def decode(data: bytes) -> 'errorRequest':
        r = _Reader(data)
        return errorRequest(this_id=r.u64(), args=[r.str() for _ in range(r.u32())])
