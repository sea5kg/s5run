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
from libpm import s5run_wire

ROOT = os.path.dirname(os.path.abspath(__file__))
SERVER_BIN = os.path.join(ROOT, "s5run")
CPP11_CLIENT_DIR = os.path.join(ROOT, "tests", "cpp11-client")
CPP11_CLIENT_BUILD = os.path.join(ROOT, "tmp", "cpp11-client")
CPP11_GEN_BIN = os.path.join(CPP11_CLIENT_DIR, "bin", "cpp11-client")
CPP98_CLIENT_DIR = os.path.join(ROOT, "tests", "cpp98-client")
CPP98_CLIENT_BUILD = os.path.join(ROOT, "tmp", "cpp98-client")
CPP98_CLIENT_BIN = os.path.join(CPP98_CLIENT_DIR, "bin", "cpp98-client")
JAVA21_CLIENT_DIR = os.path.join(ROOT, "tests", "java21-client")
JAVA17_CLIENT_DIR = os.path.join(ROOT, "tests", "java17-client")
JAVA11_CLIENT_DIR = os.path.join(ROOT, "tests", "java11-client")
GO_CLIENT_DIR = os.path.join(ROOT, "tests", "go-client")
GO_CLIENT_BIN = os.path.join(GO_CLIENT_DIR, "bin", "go-client")

SMOKE_TESTS_LOG = os.path.join(ROOT, "smoke_tests.log")
if os.path.isfile(SMOKE_TESTS_LOG):
    os.remove(SMOKE_TESTS_LOG)
LOG = UtilsLog("smoke_tests", SMOKE_TESTS_LOG)

class CallbackError(Exception):
    pass


# ---------------------------------------------------------------------------
# build helpers
# ---------------------------------------------------------------------------

def _iter_files(path):
    if os.path.isfile(path):
        yield path
        return
    for dirpath, _dirnames, filenames in os.walk(path):
        for fn in filenames:
            yield os.path.join(dirpath, fn)


def _newest_mtime(paths):
    newest = 0.0
    for p in paths:
        if os.path.exists(p):
            newest = max(newest, os.path.getmtime(p))
    return newest


def _run(cmd, what, cwd=ROOT):
    print("-- %s" % what, flush=True)
    ret = subprocess.call(cmd, cwd=cwd)
    if ret != 0:
        raise SystemExit("!!! FAIL: %s (exit %d)" % (what, ret))

def ensure_generated():
    UtilsSmokeTests.clean_generated_outputs(LOG.get_logger())
    LOG.get_logger().info("-> Generating client's libs ...")

    _todo_generate = {
        "cpp98": os.path.join(ROOT, "tests", "cpp98-client", "s5run_cli"),
        "cpp11": os.path.join(ROOT, "tests", "cpp11-client", "s5run_cli_example"),
        "go": os.path.join(ROOT, "tests", "go-client", "s5runcli"),
        "py": os.path.join(ROOT, "tests", "py-client"),
    }
    _langs = _todo_generate.keys()
    _langs = sorted(_langs)
    for _lang in _langs:
        LOG.get_logger().info(" ... generate-cli for %s", _lang)
        UtilsShell.run_command(
            [SERVER_BIN, "generate", "client", "--language", _lang, "--output-dir", _todo_generate[_lang]],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )

    # other languages via pys5rungen
    _todo_generate2 = [
        "java21",
        "java17",
        "java11",
    ]

    for _lang in _todo_generate2:
        LOG.get_logger().info(" ... generate-cli for %s", _lang)
        UtilsShell.run_command(
            [sys.executable, "-m", "pys5rungen", _lang],
            _log=LOG.get_logger(),
            print_output_on_exit=True
        )
    LOG.get_logger().info("-> Generating client's libs done. [PASS]")

def ensure_cpp_client():
    inputs = _iter_files(CPP11_CLIENT_DIR)
    if not os.path.exists(CPP11_GEN_BIN) or _newest_mtime(inputs) > os.path.getmtime(CPP11_GEN_BIN):
        os.makedirs(os.path.join(CPP11_CLIENT_DIR, "bin"), exist_ok=True)
        _run(["cmake", "-H" + CPP11_CLIENT_DIR, "-B" + CPP11_CLIENT_BUILD,
              "-DCMAKE_BUILD_TYPE=Release"], "configure cpp11-client", cwd=ROOT)
        _run(["cmake", "--build", CPP11_CLIENT_BUILD, "--config", "Release"], "build cpp11-client")


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


def test_py_gen_client(port):
    print("== py-gen-client: scenarios")
    ret = subprocess.call([sys.executable, "tests/py-client/test_client.py",
                           "127.0.0.1", str(port)], cwd=ROOT)
    assert ret == 0, "py-gen-client failed with code %d" % ret
    print("  PASS")


def ensure_cpp98_client():
    os.makedirs(os.path.join(CPP98_CLIENT_DIR, "bin"), exist_ok=True)
    inputs = _iter_files(CPP98_CLIENT_DIR)
    if not os.path.exists(CPP98_CLIENT_BIN) or _newest_mtime(inputs) > os.path.getmtime(CPP98_CLIENT_BIN):
        _run(["cmake", "-H" + CPP98_CLIENT_DIR, "-B" + CPP98_CLIENT_BUILD,
              "-DCMAKE_BUILD_TYPE=Release"], "configure cpp98-client", cwd=ROOT)
        _run(["cmake", "--build", CPP98_CLIENT_BUILD, "--config", "Release"], "build cpp98-client")


def _have_go():
    return shutil.which("go") is not None


def test_go_client(port):
    print("== go-client: scenarios")
    if not _have_go():
        print("  SKIP: Go not found")
        return
    _run(["go", "build", "-o", GO_CLIENT_BIN, "./cmd_main.go"], "build go-client", cwd=GO_CLIENT_DIR)
    ret = subprocess.call([GO_CLIENT_BIN, "127.0.0.1", str(port)], cwd=ROOT)
    assert ret == 0, "go-client failed with code %d" % ret
    print("  PASS")



def test_cpp98_prefix_client(port):
    print("== cpp98-prefix-client: generate with prefix-filename")
    import json as _json
    schema = _json.load(open(os.path.join(ROOT, "s5run-schema.json")))
    schema["options"]["cpp98"]["prefix-filename"] = "MyLib_"
    tmp_schema = os.path.join(ROOT, "tmp", "s5run-schema-prefix.json")
    with open(tmp_schema, "w") as f:
        _json.dump(schema, f, indent=2)
    gen_dir = os.path.join(ROOT, "tmp", "cpp98-prefix")
    shutil.rmtree(gen_dir, ignore_errors=True)
    _run([SERVER_BIN, "generate", "client", "--language", "cpp98",
          "--output-dir", gen_dir, "--schema", tmp_schema], "generate cpp98 prefix")
    hdr = os.path.join(gen_dir, "MyLib_s5run_cli_cpp98.h")
    impl = os.path.join(gen_dir, "MyLib_s5run_cli_cpp98.cpp")
    assert os.path.exists(hdr), "missing prefixed header: " + hdr
    assert os.path.exists(impl), "missing prefixed impl: " + impl
    main_src = os.path.join(gen_dir, "main_prefix.cpp")
    with open(main_src, "w") as f:
        f.write('#include "MyLib_s5run_cli_cpp98.h"\n')
        f.write('#include <cstdio>\n')
        f.write('int main(int argc, char **argv) {\n')
        f.write('    if (argc < 3) { return 1; }\n')
        f.write('    s5run_cli_cpp98::context *ctx = s5run_cli_cpp98::make_context();\n')
        f.write('    s5run_cli_cpp98::remote_server_info server0;\n')
        f.write('    server0.set_address(std::string("s5run://") + argv[1] + ":" + argv[2]);\n')
        f.write('    ctx->add_server(&server0);\n')
        f.write('    ctx->set_script("function main() { return 42; }");\n')
        f.write('    s5run_cli_cpp98::run_result result = s5run_cli_cpp98::run_script(ctx, "main");\n')
        f.write('    bool ok = !result.has_error() && result.get_value().as_int() == 42;\n')
        f.write('    std::printf("%s\\n", ok ? "prefix-client-ok" : result.error().c_str());\n')
        f.write('    return ok ? 0 : 1;\n')
        f.write('}\n')
    bin_path = os.path.join(gen_dir, "cpp98_prefix_client")
    _run(["g++", "-std=c++98", main_src, impl, "-o", bin_path], "build cpp98 prefix client")
    ret = subprocess.call([bin_path, "127.0.0.1", str(port)], cwd=ROOT)
    assert ret == 0, "cpp98-prefix-client failed with code %d" % ret
    print("  PASS")


def test_cpp98_client(port):
    print("== cpp98-client: scenarios")
    ret = subprocess.call([CPP98_CLIENT_BIN, "127.0.0.1", str(port)], cwd=ROOT)
    assert ret == 0, "cpp98-client failed with code %d" % ret
    print("  PASS")


def test_java17_client(port):
    print("== java17-client: build and run")
    if not _have_jdk_version(17):
        print("  SKIP: JDK 17 not available")
        return
    env = _java_build_env(17)
    ret = subprocess.call(["./build_simple.sh", "127.0.0.1", str(port)],
                          cwd=JAVA17_CLIENT_DIR, env=env)
    assert ret == 0, "java17-client failed with code %d" % ret
    print("  PASS")


def test_java11_client(port):
    print("== java11-client: build and run")
    if not _have_jdk_version(11):
        print("  SKIP: JDK 11 not available (no JDK 11 and javac < 11)")
        return
    env = _java_build_env(11)
    ret = subprocess.call(["./build_simple.sh", "127.0.0.1", str(port)],
                          cwd=JAVA11_CLIENT_DIR, env=env)
    assert ret == 0, "java11-client failed with code %d" % ret
    print("  PASS")


def test_java21_client(port):
    print("== java21-client: build and run")
    if not _have_jdk_version(21):
        print("  SKIP: JDK 21 not available")
        return
    env = _java_build_env(21)
    ret = subprocess.call(["./build_simple.sh", "127.0.0.1", str(port)],
                          cwd=JAVA21_CLIENT_DIR, env=env)
    assert ret == 0, "java21-client failed with code %d" % ret
    print("  PASS")


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
    print("-- start server on :%d" % port)
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
            print("  <- callback %s%s" % (method, tuple(args)))
            try:
                s5run_wire.send_callback_result(sock, cid, dispatch(method, args))
            except CallbackError as e:
                s5run_wire.send_callback_error(sock, cid, str(e))
            continue
        if t == s5run_wire.M_EXEC_RESP:
            ok = r.u8()
            return (True, r.val()) if ok else (False, r.s())
        raise s5run_wire.WireError("unexpected msg type %d" % t)


def test_happy_path(host, port):
    print("== py-client: happy path")
    sock = s5run_wire.connect(host, port)
    s5run_wire.register_object_type(sock, "File", ["GetSize", "ReadBytes"])
    s5run_wire.register_func(sock, "GetFile")
    s5run_wire.register_func(sock, "GetTimeNow")
    print("  registered type 'File', functions GetFile/GetTimeNow")

    script = """
function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    return { size: f.GetSize(), time: t };
}
"""
    s5run_wire.exec_script(sock, script, "main", ["test.txt"])
    ok, value = run_with_dispatch(sock)
    print("  exec result: %s" % (value,))
    assert ok, "exec failed: %s" % value
    assert value == {"size": 1024, "time": 1737000000}, value
    sock.close()
    print("  PASS")


def test_queue(host, port):
    print("== py-client: job queue")
    sock = s5run_wire.connect(host, port)
    uuid = s5run_wire.submit_job(sock, "function main(a, b) { return a + b; }", "main", [2, 3])
    print("  submitted:", uuid)
    for _ in range(100):
        status, ok, payload = s5run_wire.get_job_result(sock, uuid)
        if status in (2, 3):
            break
        time.sleep(0.1)
    print("  result: status=%s payload=%s" % (status, payload))
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
    print("  PASS")


def test_server_stats(host, port):
    print("== py-client: server stats")
    sock = s5run_wire.connect(host, port)
    threads, busy, queued, clients = s5run_wire.request_stats(sock)
    print("  threads=%d busy=%d queued=%d clients=%d" % (threads, busy, queued, clients))
    assert threads >= 1, "threads must be positive"
    assert clients >= 1, "our own connection must be counted"
    sock.close()
    print("  PASS")


def test_callback_error(host, port):
    print("== py-client: callback error path")
    sock = s5run_wire.connect(host, port)
    s5run_wire.register_func(sock, "Fail")
    s5run_wire.exec_script(sock, "function main() { return Fail(); }", "main", [])
    t, r = s5run_wire.recv_msg(sock)
    assert t == s5run_wire.M_CALLBACK_REQ, "expected callback, got %d" % t
    cid = r.i32()
    method = r.s()
    assert method == "Fail", method
    print("  <- callback %s" % method)
    s5run_wire.send_callback_error(sock, cid, "boom")
    ok, err = s5run_wire.recv_exec_result(sock)
    print("  exec error: %s" % err.splitlines()[0])
    assert not ok, "expected failure, got %r" % (err,)
    assert "boom" in err, err
    sock.close()
    print("  PASS")




def test_cpp_gen_client(port):
    print("== cpp11-client: scenarios")
    ret = subprocess.call([CPP11_GEN_BIN, "127.0.0.1", str(port)], cwd=ROOT)
    assert ret == 0, "cpp11-client failed with code %d" % ret
    print("  PASS")


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

    LOG.get_logger().info("-> Start compiling...")
    UtilsSmokeTests.compile_s5run(LOG.get_logger())
    LOG.get_logger().info("-> Compiling done. [PASS]")
    ensure_generated()
    ensure_cpp_client()
    ensure_cpp98_client()

    port = _free_port()
    proc = start_server(port)
    try:
        test_happy_path("127.0.0.1", port)
        test_queue("127.0.0.1", port)
        test_server_stats("127.0.0.1", port)
        test_callback_error("127.0.0.1", port)
        test_cpp_gen_client(port)
        test_py_gen_client(port)
        test_go_client(port)
        test_cpp98_client(port)
        test_cpp98_prefix_client(port)
        test_java11_client(port)
        test_java17_client(port)
        test_java21_client(port)
    finally:
        stop_server(proc)
        UtilsSmokeTests.clean_binaries(LOG.get_logger())
    print("all smoke tests passed")


if __name__ == "__main__":
    main()
