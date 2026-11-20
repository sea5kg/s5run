#!/usr/bin/env python3

# Smoke tests for the s5run server binary protocol.
#
# Usage:
#   python3 smoke_tests.py              # regen client code + build server/cpp11-client
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

ROOT = os.path.dirname(os.path.abspath(__file__))
SERVER_BIN = os.path.join(ROOT, "s5run")
CPP_CLIENT_DIR = os.path.join(ROOT, "tests", "cpp11-client")
CPP_CLIENT_BUILD = os.path.join(ROOT, "tmp", "cpp11-client")
CPP_GEN_BIN = os.path.join(CPP_CLIENT_DIR, "bin", "cpp11-client")
JAVA_CLIENT_DIR = os.path.join(ROOT, "tests", "java-client")

sys.path.insert(0, ROOT)
import tests.libs5runtests.s5run_wire as w


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


def _generated_outputs():
    with open(os.path.join(ROOT, "s5run-client.json"), "r", encoding="utf-8") as f:
        cfg = json.load(f)
    options = cfg.get("options", {})
    ns = ""
    for name, key in (("cpp11", "namespace"), ("java", "package"), ("py", "module")):
        ns = (options.get(name) or {}).get(key, "")
        if ns:
            break
    rel = lambda p: os.path.join(ROOT, p)
    outputs = []
    if "cpp11" in options:
        outputs += [
            rel(options["cpp11"]["output-dir"]) + ns + ".h",
            rel(options["cpp11"]["output-dir"]) + ns + ".cpp",
        ]
    if "py" in options:
        module = (options["py"].get("module") or ns)
        outputs.append(rel(options["py"]["output-dir"]) + module + ".py")
    if "java" in options:
        package = (options["java"].get("package") or ns).replace(".", "/")
        outputs += glob.glob(rel(options["java"]["output-dir"]) + package + "/*.java")
    return outputs


def ensure_generated():
    inputs = [os.path.join(ROOT, "s5run-client.json"),
              os.path.join(ROOT, "pys5rungen")]
    inputs += _iter_files(os.path.join(ROOT, "libs5run_client_code_generator"))
    outputs = _generated_outputs()
    if not outputs or _newest_mtime(inputs) > _newest_mtime(outputs):
        _run([sys.executable, "-m", "pys5rungen", "all"], "regenerate client code")


def ensure_server():
    inputs = [os.path.join(ROOT, "CMakeLists.txt"),
              os.path.join(ROOT, "build_simple.sh")]
    inputs += _iter_files(os.path.join(ROOT, "src"))
    if not os.path.exists(SERVER_BIN) or _newest_mtime(inputs) > os.path.getmtime(SERVER_BIN):
        _run(["./build_simple.sh"], "build server")


def ensure_cpp_client():
    inputs = _iter_files(CPP_CLIENT_DIR)
    if not os.path.exists(CPP_GEN_BIN) or _newest_mtime(inputs) > os.path.getmtime(CPP_GEN_BIN):
        _run(["cmake", "-H" + CPP_CLIENT_DIR, "-B" + CPP_CLIENT_BUILD,
              "-DCMAKE_BUILD_TYPE=Release"], "configure cpp11-client", cwd=ROOT)
        _run(["cmake", "--build", CPP_CLIENT_BUILD, "--config", "Release"], "build cpp11-client")


def _have_jdk():
    return all(shutil.which(cmd) for cmd in ("javac", "jar", "java"))


def test_java_client(port):
    print("== java-client: build and run")
    if not _have_jdk():
        print("  SKIP: JDK not found")
        return
    _run(["./build_simple.sh", "127.0.0.1", str(port)],
         "compile and run java-client", cwd=JAVA_CLIENT_DIR)
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
    proc = subprocess.Popen([SERVER_BIN, "--listen", "127.0.0.1:" + str(port),
                             "--threads", "2"], cwd=ROOT,
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
        return w.Ref(42, "File")
    if method == "GetSize":
        return 1024
    raise CallbackError("unknown method " + method)


def run_with_dispatch(sock):
    while True:
        t, r = w.recv_msg(sock)
        if t == w.M_CALLBACK_REQ:
            cid = r.i32()
            method = r.s()
            args = [r.val() for _ in range(r.u32())]
            print("  <- callback %s%s" % (method, tuple(args)))
            try:
                w.send_callback_result(sock, cid, dispatch(method, args))
            except CallbackError as e:
                w.send_callback_error(sock, cid, str(e))
            continue
        if t == w.M_EXEC_RESP:
            ok = r.u8()
            return (True, r.val()) if ok else (False, r.s())
        raise w.WireError("unexpected msg type %d" % t)


def test_happy_path(host, port):
    print("== py-client: happy path")
    sock = w.connect(host, port)
    w.register_object_type(sock, "File", ["GetSize", "ReadBytes"])
    w.register_func(sock, "GetFile")
    w.register_func(sock, "GetTimeNow")
    print("  registered type 'File', functions GetFile/GetTimeNow")

    script = """
function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    return { size: f.GetSize(), time: t };
}
"""
    w.exec_script(sock, script, "main", ["test.txt"])
    ok, value = run_with_dispatch(sock)
    print("  exec result: %s" % (value,))
    assert ok, "exec failed: %s" % value
    assert value == {"size": 1024, "time": 1737000000}, value
    sock.close()
    print("  PASS")


def test_server_stats(host, port):
    print("== py-client: server stats")
    sock = w.connect(host, port)
    threads, busy, queued, clients = w.request_stats(sock)
    print("  threads=%d busy=%d queued=%d clients=%d" % (threads, busy, queued, clients))
    assert threads >= 1, "threads must be positive"
    assert clients >= 1, "our own connection must be counted"
    sock.close()
    print("  PASS")


def test_callback_error(host, port):
    print("== py-client: callback error path")
    sock = w.connect(host, port)
    w.register_func(sock, "Fail")
    w.exec_script(sock, "function main() { return Fail(); }", "main", [])
    t, r = w.recv_msg(sock)
    assert t == w.M_CALLBACK_REQ, "expected callback, got %d" % t
    cid = r.i32()
    method = r.s()
    assert method == "Fail", method
    print("  <- callback %s" % method)
    w.send_callback_error(sock, cid, "boom")
    ok, err = w.recv_exec_result(sock)
    print("  exec error: %s" % err.splitlines()[0])
    assert not ok, "expected failure, got %r" % (err,)
    assert "boom" in err, err
    sock.close()
    print("  PASS")




def test_cpp_gen_client(port):
    print("== cpp11-client: scenarios")
    ret = subprocess.call([CPP_GEN_BIN, "127.0.0.1", str(port)], cwd=ROOT)
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

    ensure_generated()
    ensure_server()
    ensure_cpp_client()

    port = _free_port()
    proc = start_server(port)
    try:
        test_happy_path("127.0.0.1", port)
        test_server_stats("127.0.0.1", port)
        test_callback_error("127.0.0.1", port)
        test_cpp_gen_client(port)
        test_java_client(port)
    finally:
        stop_server(proc)
    print("all smoke tests passed")


if __name__ == "__main__":
    main()
