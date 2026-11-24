# AUTO-GENERATED. DO NOT EDIT.

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Any




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
        return ["GetSize", "ReadBytes", "GetName", "GetBufferSize", "SetBufferSize"]

    @abstractmethod
    def GetSize(self) -> int: ...
    @abstractmethod
    def ReadBytes(self, readBytes: int) -> list[int]: ...
    @abstractmethod
    def GetName(self) -> str: ...
    @abstractmethod
    def GetBufferSize(self) -> int: ...
    @abstractmethod
    def SetBufferSize(self, bufferSize: int): ...



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



class KeyValueStorage(S5RunObject):
    """Schema class 'KeyValueStorage' — inherit and implement."""

    def s5run_class_name(self) -> str:
        return "KeyValueStorage"

    def s5run_method_names(self) -> list[str]:
        return ["SetValueByKey", "GetValueByKey"]

    @abstractmethod
    def SetValueByKey(self, name: str, value: Any): ...
    @abstractmethod
    def GetValueByKey(self, name: str): ...



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

