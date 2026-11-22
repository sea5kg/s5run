#!/usr/bin/env python3
"""Smoke test client built on the GENERATED s5run python client.

Usage: python3 tests/py-client/test_client.py [host] [port]
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import libs5run_cli_example as m

K_SCRIPT = """
function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    var b = f.ReadBytes(3);
    console.log("hello", path, 1, 2);
    var sizeByRef = GetFileSize(f);
    var opened = GetOpenedFiles();
    var openedSum = 0;
    for (var i = 0; i < opened.length; ++i)
        openedSum += opened[i].GetSize();
    CloseFiles(opened);
    var fileList = FileSystem.GetList(path);
    return {
        size: f.GetSize(),
        sizeByRef: sizeByRef,
        openedCount: opened.length,
        openedSum: openedSum,
        fileListCount: fileList.length,
        time: t,
        bytes: b.length
    };
}
"""


class MyFile(m.File):
    def __init__(self, size: int) -> None:
        self._size = size

    def GetSize(self) -> int:
        return self._size

    def ReadBytes(self, readBytes: int) -> list[int]:
        return [97, 98, 99][:readBytes]

    def GetName(self) -> str:
        return "example.txt"

    def GetBufferSize(self) -> int:
        return 4096

    def SetBufferSize(self, bufferSize: int) -> None:
        pass


class MyGetFile(m.GlobalFunction_GetFile):
    last_path = ""

    def execute(self, filepath: str) -> m.File:
        MyGetFile.last_path = filepath
        return MyFile(1024)


class MyGetFileSize(m.GlobalFunction_GetFileSize):
    def execute(self, file: m.File) -> int:
        return file.GetSize()


class MyGetTimeNow(m.GlobalFunction_GetTimeNow):
    def execute(self) -> int:
        return 1737000000


class MyHasFile(m.GlobalFunction_HasFile):
    def execute(self, filepath: str) -> bool:
        raise RuntimeError("boom-hasfile")


class MyGetOpenedFiles(m.GlobalFunction_GetOpenedFiles):
    def execute(self) -> list[m.File]:
        return [MyFile(100), MyFile(200)]


class MyCloseFiles(m.GlobalFunction_CloseFiles):
    closed_sizes: list[int] = []

    def execute(self, files: list[m.File]) -> None:
        MyCloseFiles.closed_sizes = [f.GetSize() for f in files]


class MyGetPi(m.GlobalFunction_GetPi):
    def execute(self) -> float:
        return 3.14


class MyGetServerPort(m.GlobalFunction_GetServerPort):
    def execute(self) -> int:
        return 8080


class MyConsole(m.console):
    logs: list[str] = []

    def log(self, args: list[str]) -> None:
        MyConsole.logs.append(" ".join(args))

    def warning(self, args: list[str]) -> None:
        pass

    def error(self, args: list[str]) -> None:
        pass


class MyFileSystem(m.FileSystem):
    last_path = ""

    def GetList(self, filepath: str) -> list[str]:
        MyFileSystem.last_path = filepath
        return ["a.txt", "b.txt", "c.txt"]


def check(cond: bool, what: str) -> None:
    if not cond:
        raise RuntimeError(f"unexpected {what}")


def happy_path(address: str) -> None:
    print("== py-gen-client: happy path")
    with m.Context() as ctx:
        ctx.add_class(MyFile(1024)) \
           .add_function(MyGetFile()) \
           .add_function(MyGetFileSize()) \
           .add_function(MyGetTimeNow()) \
           .add_function(MyGetOpenedFiles()) \
           .add_function(MyCloseFiles()) \
           .add_function(MyGetPi()) \
           .add_function(MyGetServerPort()) \
           .add_object(MyFileSystem()) \
           .add_object(MyConsole()) \
           .add_server(m.RemoteServerInfo().address(address).api_key("some api key runner")) \
           .set_script(K_SCRIPT)

        result = m.run_script(ctx, "main", ["test.txt"])
        if result.has_error:
            raise RuntimeError("unexpected error: " + result.error)
        print(f"  duration: {result.duration_ms:.1f} ms")

        check(result.value["size"] == 1024, "size")
        check(result.value["sizeByRef"] == 1024, "sizeByRef")
        check(result.value["openedCount"] == 2, "openedCount")
        check(result.value["openedSum"] == 300, "openedSum")
        check(result.value["fileListCount"] == 3, "fileListCount")
        check(result.value["time"] == 1737000000, "time")
        check(MyGetFile.last_path == "test.txt", "GetFile path")
        check(MyFileSystem.last_path == "test.txt", "FileSystem.GetList path")
        check(MyConsole.logs[0] == "hello test.txt 1 2", "console.log")
        check(MyCloseFiles.closed_sizes == [100, 200], "CloseFiles sizes")
    print("  PASS")


def primitives_two_servers(address: str) -> None:
    print("== py-gen-client: primitives (two servers)")
    ctx = m.Context() \
        .add_function(MyGetPi()) \
        .add_function(MyGetServerPort()) \
        .add_server(m.RemoteServerInfo().address(address)) \
        .add_server(m.RemoteServerInfo().address(address)) \
        .set_script("function main() { return { pi: GetPi(), port: GetServerPort() }; }")
    result = m.run_script(ctx, "main", [])
    if result.has_error:
        raise RuntimeError("unexpected error: " + result.error)
    check(3.13 < result.value["pi"] < 3.15, "pi")
    check(result.value["port"] == 8080, "port")
    ctx.close()
    print("  PASS")


def error_path(address: str) -> None:
    print("== py-gen-client: error path")
    ctx = m.Context() \
        .add_function(MyHasFile()) \
        .add_server(m.RemoteServerInfo().address(address)) \
        .set_script('function main() {\n  return HasFile("x");\n}')
    result = m.run_script(ctx, "main", [])
    check(result.has_error, "expected error")
    print(f"  error line: {result.error_line}")
    check("boom-hasfile" in result.error, "boom-hasfile in error")
    check(result.error_line == 2, "error line")
    ctx.close()
    print("  PASS")


def main() -> None:
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 7777
    address = f"s5run://{host}:{port}"

    check(m.client_version() == "1.0.0", "client version")

    happy_path(address)
    primitives_two_servers(address)
    error_path(address)
    print("py-gen-client: all smoke tests passed")


if __name__ == "__main__":
    main()
