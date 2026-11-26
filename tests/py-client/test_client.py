#!/usr/bin/env python3
"""Smoke test client built on the GENERATED s5run python client.

Usage: python3 tests/py-client/test_client.py [host] [port]
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

import libs5run_cli_example as m

# ---- data-driven scenarios (like the cpp/java/go clients) ----

from dataclasses import dataclass


@dataclass
class Scenario:
    name: str
    script_js: str
    output_console_expected: str
    output_return_expected: str
    input_arguments: list


def _split_lines(text: str) -> list:
    return [ln.rstrip("\r") for ln in text.split("\n") if ln.strip()]


def _load_scenario(base: Path) -> Scenario:
    def read(name: str, default: str = "") -> str:
        p = base / name
        return p.read_text(encoding="utf-8") if p.is_file() else default

    script = read("script.js", None)
    if script is None:
        raise RuntimeError(f"cannot find {base / 'script.js'}")
    return Scenario(
        name=base.name,
        script_js=script,
        output_console_expected=read("output_console_expected.txt"),
        output_return_expected=read("output_return_expected.txt"),
        input_arguments=_split_lines(read("input_arguments.txt")),
    )


def _list_scenarios() -> list:
    root = Path(__file__).resolve().parent.parent / "__test_data"
    out = [ _load_scenario(d) for d in sorted(root.iterdir())
            if d.is_dir() and d.name.startswith("client_") and "_scenario" in d.name ]
    if not out:
        raise RuntimeError("no client_scenarioX found in __test_data")
    return out


def _parse_input_argument(raw: str):
    try:
        return int(raw)
    except ValueError:
        pass
    if raw == "true":
        return True
    if raw == "false":
        return False
    return raw


def _check_scenario_return(name: str, got, expected_raw: str) -> None:
    expected = _split_lines(expected_raw)
    got_lines = _split_lines(m.to_string_s5run(got))
    if got_lines == expected:
        return
    n = max(len(got_lines), len(expected))
    details = "\n".join(
        f'  line {i + 1}: got "{got_lines[i] if i < len(got_lines) else "<missing>"}" '
        f'expected "{expected[i] if i < len(expected) else "<missing>"}"'
        for i in range(n)
        if (got_lines[i] if i < len(got_lines) else None) != (expected[i] if i < len(expected) else None)
    )
    raise RuntimeError(f"{name}: return value mismatch:\n{details}")


class MyFile(m.File):
    def __init__(self, size: int) -> None:
        self._size = size
        self.buffer_size = 4096

    def GetSize(self) -> int:
        return self._size

    def ReadBytes(self, readBytes: int) -> list[int]:
        return [97, 98, 99][:readBytes]

    def GetName(self) -> str:
        return "example.txt"

    def GetBufferSize(self) -> int:
        return self.buffer_size

    def SetBufferSize(self, bufferSize: int) -> None:
        self.buffer_size = bufferSize


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


def scenarios(address: str) -> None:
    for test in _list_scenarios():
        print(f"== py-gen-client: {test.name}")
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
               .set_script(test.script_js)

            MyConsole.logs.clear()
            result = m.run_script(ctx, "main",
                                  [_parse_input_argument(a) for a in test.input_arguments])
            if result.has_error:
                raise RuntimeError("unexpected error: " + result.error)
            print(f"  duration: {result.duration_ms:.1f} ms")

            # return value: compare with output_return_expected.txt
            if test.output_return_expected:
                _check_scenario_return(test.name, result.value, test.output_return_expected)
                print(f"  return value: {m.to_string_s5run(result.value).replace(chr(10), ' | ')}")
                print(f"  return json:  {m.to_string_json(result.value)}")
                print("  return value matches output_return_expected.txt")
            else:
                print("  return value: no output_return_expected.txt, skipped")

            # console output: compare with output_console_expected.txt (line per log)
            expected_console = _split_lines(test.output_console_expected)
            if MyConsole.logs != expected_console:
                n = max(len(MyConsole.logs), len(expected_console))
                details = "\n".join(
                    f'  line {i + 1}: got "{MyConsole.logs[i] if i < len(MyConsole.logs) else "<missing>"}" '
                    f'expected "{expected_console[i] if i < len(expected_console) else "<missing>"}"'
                    for i in range(n)
                    if (MyConsole.logs[i] if i < len(MyConsole.logs) else None)
                    != (expected_console[i] if i < len(expected_console) else None)
                )
                raise RuntimeError(f"{test.name}: console output mismatch:\n{details}")
            print(f"  console output matches {len(MyConsole.logs)} lines")

            # side-effect checks (callback state, to be moved into data files)
            if test.name == "client_scenario0":
                check(MyGetFile.last_path == "test.txt", "GetFile path")
                check(MyFileSystem.last_path == "test.txt", "FileSystem.GetList path")
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

    scenarios(address)
    primitives_two_servers(address)
    error_path(address)
    print("py-gen-client: all smoke tests passed")


if __name__ == "__main__":
    main()
