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

# Smoke tests for the s5run server binary protocol.
#
# Usage:
#   python3 smoke_tests.py              # regenerate client code + build server/cpp11-client
#                                       # if needed, start server on a free port, run all
#   python3 smoke_tests.py <port>       # run only python scenarios against 127.0.0.1:<port>
#   python3 smoke_tests.py <host> <port>
import glob
import json
import os
import shutil
import socket
import subprocess
import sys
import time
from libpm import UtilsSmokeTests
from libpm import UtilsLog
from libpm import UtilsShell
from libpm import FolderSwitcher
from libpm import s5run_wire

ROOT = os.path.dirname(os.path.abspath(__file__))
SERVER_BIN = os.path.join(ROOT, "s5run")

SMOKE_TESTS_LOG = os.path.join(ROOT, "smoke_tests.log")
if os.path.isfile(SMOKE_TESTS_LOG):
    os.remove(SMOKE_TESTS_LOG)
LOG = UtilsLog("smoke_tests", SMOKE_TESTS_LOG)

class CallbackError(Exception):
    pass


def test_py_gen_client(port):
    LOG.get_logger().info(" -> python client test: py-client")
    LOG.get_logger().info(" ... py-client: scenarios")
    client_dir = os.path.join(ROOT, "tests", "py-client")
    with FolderSwitcher(LOG.get_logger(), client_dir) as _:
        ret, _ = UtilsShell.run_command(
            [sys.executable, "-u", "test_client.py", "127.0.0.1", str(port)],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
    assert ret == 0, "py-gen-client failed with code %d" % ret
    LOG.get_logger().info(" ... py-client: [PASS]")


def test_cpp_gen_client(cpp_version, port):
    LOG.get_logger().info(" -> cpp client test: %s", cpp_version)
    client_dir = os.path.join(ROOT, "tests", cpp_version + "-client")
    build_dir = os.path.join(ROOT, "tmp", cpp_version + "-client")
    client_bin = os.path.join(client_dir, "bin", cpp_version + "-client")
    with FolderSwitcher(LOG.get_logger(), client_dir) as _:
        UtilsShell.run_command(
            ["cmake", "-H" + client_dir, "-B" + build_dir,
              "-DCMAKE_BUILD_TYPE=Release"],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
        UtilsShell.run_command(
            ["cmake", "--build", build_dir, "--config", "Release"],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )

    LOG.get_logger().info(" ... Builded client for %s [PASS]", cpp_version)
    LOG.get_logger().info(" ... %s-client: scenarios", cpp_version)
    with FolderSwitcher(LOG.get_logger(), client_dir) as _:
        ret, _ = UtilsShell.run_command(
            [client_bin, "127.0.0.1", str(port)],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
        assert ret == 0, cpp_version + "-client failed with code %d" % ret
    LOG.get_logger().info(" ... %s-client: [PASS]", cpp_version)


def test_cpp98_provider(port):
    LOG.get_logger().info(" -> cpp provider test: cpp98-provider")
    provider_dir = os.path.join(ROOT, "tests", "cpp98-provider")
    build_dir = os.path.join(ROOT, "tmp", "cpp98-provider")
    provider_bin = os.path.join(provider_dir, "bin", "cpp98-provider")
    with FolderSwitcher(LOG.get_logger(), provider_dir) as _:
        UtilsShell.run_command(
            ["cmake", "-H" + provider_dir, "-B" + build_dir,
              "-DCMAKE_BUILD_TYPE=Release"],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
        UtilsShell.run_command(
            ["cmake", "--build", build_dir, "--config", "Release"],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
        LOG.get_logger().info(" ... cpp98-provider: scenarios")
        ret, _ = UtilsShell.run_command(
            [provider_bin, "127.0.0.1", str(port)],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
        assert ret == 0, "cpp98-provider failed with code %d" % ret
    LOG.get_logger().info(" ... cpp98-provider: [PASS]")


def test_go_client(port):
    LOG.get_logger().info(" -> golang client test: go-client")
    _have_go = shutil.which("go") is not None
    if not _have_go:
        LOG.get_logger().info(" ... [SKIP] Go not found")
        return
    client_dir = os.path.join(ROOT, "tests", "go-client")
    client_bin = os.path.join(client_dir, "bin", "go-client")
    with FolderSwitcher(LOG.get_logger(), client_dir) as _:
        ret, _ = UtilsShell.run_command(
            ["go", "build", "-o", client_bin, "./cmd_main.go"],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
        LOG.get_logger().info(" ... Builded client for go-client [PASS]")
    LOG.get_logger().info(" ... go-client: scenarios")
    ret, _ = UtilsShell.run_command(
        [client_bin, "127.0.0.1", str(port)],
        _log=LOG.get_logger(),
        print_output_on_exit=True
    )
    assert ret == 0, "go-client failed with code %d" % ret
    LOG.get_logger().info(" ... go-client: [PASS]")


def _have_jdk():
    return all(shutil.which(cmd) for cmd in ("javac", "jar", "java"))


def _jdk_path(version):
    """JAVA_HOME override for a specific Java version, e.g. _jdk_path(11)."""
    env = os.environ.get(f"JAVA{version}_HOME", "")
    if env and os.path.isfile(os.path.join(env, "bin", "javac")):
        return env
    candidates = [
        f"/usr/lib/jvm/java-{version}-openjdk-amd64",
        f"/usr/lib/jvm/java-{version}-openjdk",
        f"/usr/lib/jvm/java-{version}-openjdk-arm64",
    ]
    for c in candidates:
        if os.path.isfile(os.path.join(c, "bin", "javac")):
            return c
    return None


def _have_jdk_version(version):
    """True if this Java version target is buildable: a matching JDK is
    installed OR the default javac is new enough to cross-compile via
    --release."""
    if _jdk_path(version):
        return True
    if _have_jdk():
        try:
            out = subprocess.check_output(["javac", "--version"], stderr=subprocess.STDOUT, text=True)
            major = int(out.strip().split()[1].split(".")[0])
            return major >= version
        except (ValueError, subprocess.CalledProcessError, FileNotFoundError):
            return False
    return False


def _java_build_env(version):
    """Env with JAVA_HOME and JAVA_RELEASE for the target version.

    Uses the system javac with --release N when no dedicated JDK<N> is
    installed (cross-compilation)."""
    env = os.environ.copy()
    jdk = _jdk_path(version)
    if jdk:
        env["JAVA_HOME"] = jdk
        env["PATH"] = os.path.join(jdk, "bin") + os.pathsep + env["PATH"]
        env["JAVA_RELEASE"] = str(version)
    elif _have_jdk():
        # cross-compile through the default (newer) javac
        env["JAVA_RELEASE"] = str(version)
    return env


def test_java_client(java_version, port):
    java_x = int(java_version.split("java")[1])
    LOG.get_logger().info(" -> java client test: java%s", str(java_x))
    if not _have_jdk_version(java_x):
        print("  SKIP: JDK " + str(java_x) + " not available")
        return
    env = _java_build_env(java_x)
    client_dir = os.path.join(ROOT, "tests", "java" + str(java_x) + "-client")
    ret = -1
    with FolderSwitcher(LOG.get_logger(), client_dir) as _:
        ret, _ = UtilsShell.run_command(
            ["./build_simple.sh", "127.0.0.1", str(port)],
            _log=LOG.get_logger(),
            print_output_on_exit=True,
            env=env
        )
        LOG.get_logger().info(" ... Builded client for java%s [PASS]", str(java_x))
        LOG.get_logger().info(" ... java%s-client: scenarios", str(java_x))
        ret, _ = UtilsShell.run_command(
            ["java", "-jar", "s5run-client-1.0.0.jar", "127.0.0.1", str(port)],
            _log=LOG.get_logger(),
            print_output_on_exit=True,
            env=env
        )
    assert ret == 0, "java" + str(java_x) + "-client failed with code %d" % ret
    LOG.get_logger().info(" ... java%s-client: [PASS]", str(java_x))


# ---------------------------------------------------------------------------
# server management
# ---------------------------------------------------------------------------

def _free_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", 0))
    port = s.getsockname()[1]
    s.close()
    return port


def start_server(port):
    LOG.get_logger().info("-> Start server on %d port... ", port)
    log = open(os.path.join(ROOT, "tmp", "smoke-server.log"), "w")
    queue_db = os.path.join(ROOT, "tmp", "smoke-queue.sqlite3")
    for suffix in ("", "-wal", "-shm"):
        try:
            os.remove(queue_db + suffix)
        except OSError:
            pass
    proc = subprocess.Popen([SERVER_BIN, "server", "--listen", "127.0.0.1:" + str(port),
                             "--threads", "2", "--queue", "--db", queue_db,
                             "--ttl", "60"], cwd=ROOT,
                            stdout=log, stderr=subprocess.STDOUT)
    deadline = time.time() + 10
    while time.time() < deadline:
        try:
            s = socket.create_connection(("127.0.0.1", port), timeout=0.5)
            s.close()
            return proc
        except OSError:
            if proc.poll() is not None:
                raise SystemExit("!!! FAIL: server exited with code %d" % proc.returncode)
            time.sleep(0.05)
    proc.terminate()
    raise SystemExit("!!! FAIL: server did not start in 10s")


def stop_server(proc):
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()


# ---------------------------------------------------------------------------
# python client scenarios
# ---------------------------------------------------------------------------

def dispatch(method, args):
    if method == "GetTimeNow":
        return 1737000000
    if method == "GetFile":
        return s5run_wire.Ref(42, "File")
    if method == "GetSize":
        return 1024
    raise CallbackError("unknown method " + method)


def run_with_dispatch(sock):
    while True:
        t, r = s5run_wire.recv_msg(sock)
        if t == s5run_wire.M_CALLBACK_REQ:
            cid = r.i32()
            method = r.s()
            args = [r.val() for _ in range(r.u32())]
            # print("  <- callback %s%s" % (method, tuple(args)))
            try:
                s5run_wire.send_callback_result(sock, cid, dispatch(method, args))
            except CallbackError as e:
                s5run_wire.send_callback_error(sock, cid, str(e))
            continue
        if t == s5run_wire.M_EXEC_RESP:
            ok = r.u8()
            return (True, r.val()) if ok else (False, r.s())
        raise s5run_wire.WireError("unexpected msg type %d" % t)


def test_run_command(port):
    LOG.get_logger().info("-> Run command line client s5run run...")

    with FolderSwitcher(LOG.get_logger(), ROOT) as _:
        ret, out = UtilsShell.run_command(
            [SERVER_BIN, "run", "127.0.0.1:" + str(port),
                os.path.join(ROOT, "tests", "test_run_script.js"), "1", "2"],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
    assert ret == 0, f"s5run run failed: ${out}"
    assert "hello from test_run_script.js" in out, out
    assert "a = 1, b = 2" in out, out
    assert "a + b = 3" in out, out
    assert "[warning] this is a warning message" in out
    assert "{args: [1, 2], loopTotal: 55, ok: true, sum: 3}" in out, out
    LOG.get_logger().info(" ... Run script test_run_script.js via command line [PASS]")


def test_happy_path(host, port):
    LOG.get_logger().info("-> Check happy path...")
    sock = s5run_wire.connect(host, port)
    s5run_wire.register_object_type(sock, "File", ["GetSize", "ReadBytes"])
    s5run_wire.register_func(sock, "GetFile")
    s5run_wire.register_func(sock, "GetTimeNow")
    # print("  registered type 'File', functions GetFile/GetTimeNow")

    script = """
function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    return { size: f.GetSize(), time: t };
}
"""
    s5run_wire.exec_script(sock, script, "main", ["test.txt"])
    ok, value = run_with_dispatch(sock)
    # print("  exec result: %s" % (value,))
    assert ok, "exec failed: %s" % value
    assert value == {"size": 1024, "time": 1737000000}, value
    sock.close()
    LOG.get_logger().info(" ... [PASS]")


def test_queue(host, port):
    LOG.get_logger().info("-> Check job queue...")
    sock = s5run_wire.connect(host, port)
    uuid = s5run_wire.submit_job(sock, "function main(a, b) { return a + b; }", "main", [2, 3])
    # print("  submitted:", uuid)
    for _ in range(100):
        status, ok, payload = s5run_wire.get_job_result(sock, uuid)
        if status in (2, 3):
            break
        time.sleep(0.1)
    # print("  result: status=%s payload=%s" % (status, payload))
    assert status == 2 and ok and payload == 5

    uuid2 = s5run_wire.submit_job(sock, "function main() { return nope(); }", "main", [])
    for _ in range(100):
        status2, ok2, payload2 = s5run_wire.get_job_result(sock, uuid2)
        if status2 in (2, 3):
            break
        time.sleep(0.1)
    assert status2 == 3 and not ok2 and "nope" in payload2

    status3, _, _ = s5run_wire.get_job_result(sock, "deadbeef-0000")
    assert status3 == 0xFF, status3
    sock.close()
    LOG.get_logger().info(" ... [PASS]")


def test_server_stats(host, port):
    LOG.get_logger().info("-> Check server stats...")
    sock = s5run_wire.connect(host, port)
    threads, busy, queued, clients = s5run_wire.request_stats(sock)
    # print("  threads=%d busy=%d queued=%d clients=%d" % (threads, busy, queued, clients))
    assert threads >= 1, "threads must be positive"
    assert clients >= 1, "our own connection must be counted"
    sock.close()
    LOG.get_logger().info(" ... stats [PASS]")


def test_callback_error(host, port):
    LOG.get_logger().info("-> Check callback error path...")
    sock = s5run_wire.connect(host, port)
    s5run_wire.register_func(sock, "Fail")
    s5run_wire.exec_script(sock, "function main() { return Fail(); }", "main", [])
    t, r = s5run_wire.recv_msg(sock)
    assert t == s5run_wire.M_CALLBACK_REQ, "expected callback, got %d" % t
    cid = r.i32()
    method = r.s()
    assert method == "Fail", method
    # print("  <- callback %s" % method)
    s5run_wire.send_callback_error(sock, cid, "boom")
    ok, err = s5run_wire.recv_exec_result(sock)
    # print("  exec error: %s" % err.splitlines()[0])
    assert not ok, "expected failure, got %r" % (err,)
    assert "boom" in err, err
    sock.close()
    LOG.get_logger().info(" ... [PASS]")


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    os.makedirs(os.path.join(ROOT, "tmp"), exist_ok=True)

    if len(sys.argv) >= 2:
        host = sys.argv[1]
        port = int(sys.argv[2]) if len(sys.argv) > 2 else int(sys.argv[1])
        test_happy_path(host, port)
        test_callback_error(host, port)
        print("all smoke tests passed")
        return

    UtilsSmokeTests.step_compile_s5run(LOG.get_logger())
    UtilsSmokeTests.step_clean_generated_outputs(LOG.get_logger())
    UtilsSmokeTests.step_generated_client_code(LOG.get_logger())
    UtilsSmokeTests.step_generated_providers_code(LOG.get_logger())

    port = _free_port()
    proc = start_server(port)
    try:
        test_happy_path("127.0.0.1", port)
        test_queue("127.0.0.1", port)
        test_server_stats("127.0.0.1", port)
        test_callback_error("127.0.0.1", port)
        test_run_command(port)
        test_cpp_gen_client("cpp98", port)
        test_cpp98_provider(port)
        test_cpp_gen_client("cpp11", port)
        test_py_gen_client(port)
        test_go_client(port)
        test_java_client("java11", port)
        test_java_client("java17", port)
        test_java_client("java21", port)
    finally:
        stop_server(proc)
        UtilsSmokeTests.step_clean_binaries(LOG.get_logger())
    print("all smoke tests passed")


if __name__ == "__main__":
    main()
