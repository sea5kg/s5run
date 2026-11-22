// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

import java.io.*;
import java.net.Socket;
import java.util.*;

public final class Context implements AutoCloseable {
    @FunctionalInterface
    interface ThrowingHandler {
        void handle(int cbId, Wire.Reader r) throws Exception;
    }

    private final Map<String, ThrowingHandler> handlers = new HashMap<>();
    private final Map<String, S5RunFunction> functions = new HashMap<>();
    private final Map<String, Map<Integer, S5RunObject>> instances = new HashMap<>();
    private final Map<String, List<String>> knownClasses = new HashMap<>();
    private final List<RemoteServerInfo> servers = new ArrayList<>();
    private final List<PendingType> pendingTypes = new ArrayList<>();
    private final List<PendingObject> pendingObjects = new ArrayList<>();
    private final List<String> pendingFunctions = new ArrayList<>();
    private String script = "";
    private int nextThisId = 0;
    private Socket socket;
    private InputStream in;
    private OutputStream out;

    private static final class PendingType {
        final String name;
        final List<String> methods;
        PendingType(String name, List<String> methods) { this.name = name; this.methods = methods; }
    }

    private static final class PendingObject {
        final String name;
        final int thisId;
        final List<String> methods;
        PendingObject(String name, int thisId, List<String> methods) { this.name = name; this.thisId = thisId; this.methods = methods; }
    }

    public Context() {
                handlers.put("GetSize", (cbId, r) -> cbm_File_GetSize(cbId, r));
                handlers.put("ReadBytes", (cbId, r) -> cbm_File_ReadBytes(cbId, r));
                handlers.put("GetName", (cbId, r) -> cbm_File_GetName(cbId, r));
                handlers.put("GetBufferSize", (cbId, r) -> cbm_File_GetBufferSize(cbId, r));
                handlers.put("SetBufferSize", (cbId, r) -> cbm_File_SetBufferSize(cbId, r));
                handlers.put("GetList", (cbId, r) -> cbm_FileSystem_GetList(cbId, r));
                handlers.put("log", (cbId, r) -> cbm_console_log(cbId, r));
                handlers.put("warning", (cbId, r) -> cbm_console_warning(cbId, r));
                handlers.put("error", (cbId, r) -> cbm_console_error(cbId, r));
    }

    public static String clientVersion() {
        return ClientVersion.VERSION;
    }

    public Context setScript(String script) {
        this.script = script;
        return this;
    }

    public Context addServer(RemoteServerInfo server) {
        Objects.requireNonNull(server, "server");
        servers.add(server);
        return this;
    }

    public Context addClass(S5RunObject sample) {
        return addClass(sample.s5runClassName(), sample.s5runMethodNames());
    }

    private Context addClass(String name, List<String> methods) {
        if (socket != null) {
            try {
                sendRegisterType(name, methods);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } else {
            pendingTypes.add(new PendingType(name, methods));
        }
        return this;
    }

    public Context addObject(S5RunObject obj) {
        String cls = obj.s5runClassName();
        int id = ++nextThisId;
        instances.computeIfAbsent(cls, k -> new HashMap<>()).put(id, obj);
        if (socket != null) {
            try {
                sendRegisterObject(cls, id, obj.s5runMethodNames());
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } else {
            pendingObjects.add(new PendingObject(cls, id, obj.s5runMethodNames()));
        }
        return this;
    }

    public Context addFunction(S5RunFunction fn) {
        functions.put(fn.s5runFunctionName(), fn);
        String name = fn.s5runFunctionName();
        handlers.put(name, (cbId, r) -> dispatchByName(name, cbId, r));
        if (socket != null) {
            try {
                sendRegisterFunction(fn.s5runFunctionName());
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } else {
            pendingFunctions.add(fn.s5runFunctionName());
        }
        return this;
    }

    // ------------------------------------------------------------------
    // public entry point
    // ------------------------------------------------------------------

    public static RunResult runScript(Context ctx, String entry, List<Value> args) {
        long started = System.currentTimeMillis();
        try {
            ctx.connect();
            ctx.flushPending();
            Wire.Writer w = new Wire.Writer();
            w.str(ctx.script);
            w.str(entry);
            w.u32(args.size());
            for (Value a : args) Wire.encodeValue(w, a);
            ctx.sendMessage(Wire.M_EXEC_REQ, w.toByteArray());
            while (true) {
                Frame f = ctx.readFrame();
                if (f.msgType == Wire.M_CALLBACK_REQ) {
                    ctx.dispatchCallback(f.body);
                    continue;
                }
                if (f.msgType == Wire.M_EXEC_RESP) {
                    Wire.Reader r = new Wire.Reader(f.body);
                    if (r.u8() == 0) {
                        throw new RuntimeException(r.str());
                    }
                    return RunResult.ok(Wire.decodeValue(r), System.currentTimeMillis() - started);
                }
                throw new RuntimeException("s5run: unexpected message type");
            }
        } catch (Exception e) {
            return RunResult.fail(e.getMessage(), System.currentTimeMillis() - started);
        }
    }

    // ------------------------------------------------------------------
    // callbacks
    // ------------------------------------------------------------------


    private void cb_GetFile(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("GetFile: wrong argument count");
                    String filepath = r.strValue();
        var res = asGetFile().execute(filepath);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            int objId = registerInstance(res);
            w.u8(Wire.T_REF);
            w.i32(objId);
            w.str("File");
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_GetFileSize(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("GetFileSize: wrong argument count");
                    int refId_file = readRefId(r);
            File file = instanceById("File", refId_file);
        var res = asGetFileSize().execute(file);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.int64(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_GetTimeNow(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 0) throw new RuntimeException("GetTimeNow: wrong argument count");
        
        var res = asGetTimeNow().execute();
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.int64(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_HasFile(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("HasFile: wrong argument count");
                    String filepath = r.strValue();
        var res = asHasFile().execute(filepath);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.boolValue(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_GetFileList(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("GetFileList: wrong argument count");
                    String filepath = r.strValue();
        var res = asGetFileList().execute(filepath);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            Wire.writeListOfStr(w, res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_GetOpenedFiles(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 0) throw new RuntimeException("GetOpenedFiles: wrong argument count");
        
        var res = asGetOpenedFiles().execute();
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.u8(Wire.T_ARRAY);
            w.u32(res.size());
            for (File item : res) {
                int id = registerInstance(item);
                w.u8(Wire.T_REF);
                w.i32(id);
                w.str("File");
            }
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_CloseFiles(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("CloseFiles: wrong argument count");
                    if (r.u8() != Wire.T_ARRAY) throw new RuntimeException("wire: expected array");
            int cnt_files = (int) r.u32();
            List<File> files = new ArrayList<>(cnt_files);
            for (int i = 0; i < cnt_files; i++) {
                int refId = readRefId(r);
                File obj = instanceById("File", refId);
                files.add(obj);
            }
        asCloseFiles().execute(files);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.u8(Wire.T_UNDEFINED);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_GetPi(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 0) throw new RuntimeException("GetPi: wrong argument count");
        
        var res = asGetPi().execute();
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.f64Value(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cb_GetServerPort(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 0) throw new RuntimeException("GetServerPort: wrong argument count");
        
        var res = asGetServerPort().execute();
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.int64(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }


    private void cbm_File_GetSize(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("File.GetSize: wrong argument count");
        int thisId = (int) r.readLongInt();
        File obj = instanceById("File", thisId);

        var res = obj.GetSize();
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.int64(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_File_ReadBytes(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 2) throw new RuntimeException("File.ReadBytes: wrong argument count");
        int thisId = (int) r.readLongInt();
        File obj = instanceById("File", thisId);
            long readBytes = r.readLongInt();
        var res = obj.ReadBytes(readBytes);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            Wire.writeListOfByte(w, res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_File_GetName(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("File.GetName: wrong argument count");
        int thisId = (int) r.readLongInt();
        File obj = instanceById("File", thisId);

        var res = obj.GetName();
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.strValue(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_File_GetBufferSize(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 1) throw new RuntimeException("File.GetBufferSize: wrong argument count");
        int thisId = (int) r.readLongInt();
        File obj = instanceById("File", thisId);

        var res = obj.GetBufferSize();
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.int64(res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_File_SetBufferSize(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 2) throw new RuntimeException("File.SetBufferSize: wrong argument count");
        int thisId = (int) r.readLongInt();
        File obj = instanceById("File", thisId);
            long bufferSize = r.readLongInt();
        obj.SetBufferSize(bufferSize);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.u8(Wire.T_UNDEFINED);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_FileSystem_GetList(int cbId, Wire.Reader r) throws Exception {
                    if (r.u32() != 2) throw new RuntimeException("FileSystem.GetList: wrong argument count");
        int thisId = (int) r.readLongInt();
        FileSystem obj = instanceById("FileSystem", thisId);
            String filepath = r.strValue();
        var res = obj.GetList(filepath);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            Wire.writeListOfStr(w, res);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_console_log(int cbId, Wire.Reader r) throws Exception {
                    long n = r.u32();
            n -= 1;
        int thisId = (int) r.readLongInt();
        console obj = instanceById("console", thisId);
            long n_ignored = 0;
            List<String> args = new ArrayList<>((int) n);
            for (long i = 0; i < n; i++)
                args.add(valueToString(Wire.decodeValue(r)));
        obj.log(args);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.u8(Wire.T_UNDEFINED);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_console_warning(int cbId, Wire.Reader r) throws Exception {
                    long n = r.u32();
            n -= 1;
        int thisId = (int) r.readLongInt();
        console obj = instanceById("console", thisId);
            long n_ignored = 0;
            List<String> args = new ArrayList<>((int) n);
            for (long i = 0; i < n; i++)
                args.add(valueToString(Wire.decodeValue(r)));
        obj.warning(args);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.u8(Wire.T_UNDEFINED);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private void cbm_console_error(int cbId, Wire.Reader r) throws Exception {
                    long n = r.u32();
            n -= 1;
        int thisId = (int) r.readLongInt();
        console obj = instanceById("console", thisId);
            long n_ignored = 0;
            List<String> args = new ArrayList<>((int) n);
            for (long i = 0; i < n; i++)
                args.add(valueToString(Wire.decodeValue(r)));
        obj.error(args);
        Wire.Writer w = new Wire.Writer();
        w.i32(cbId);
        w.u8(1);
            w.u8(Wire.T_UNDEFINED);
        sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
    }

    private GlobalFunction_GetFile asGetFile() {
        return (GlobalFunction_GetFile) functions.get("GetFile");
    }
    private GlobalFunction_GetFileSize asGetFileSize() {
        return (GlobalFunction_GetFileSize) functions.get("GetFileSize");
    }
    private GlobalFunction_GetTimeNow asGetTimeNow() {
        return (GlobalFunction_GetTimeNow) functions.get("GetTimeNow");
    }
    private GlobalFunction_HasFile asHasFile() {
        return (GlobalFunction_HasFile) functions.get("HasFile");
    }
    private GlobalFunction_GetFileList asGetFileList() {
        return (GlobalFunction_GetFileList) functions.get("GetFileList");
    }
    private GlobalFunction_GetOpenedFiles asGetOpenedFiles() {
        return (GlobalFunction_GetOpenedFiles) functions.get("GetOpenedFiles");
    }
    private GlobalFunction_CloseFiles asCloseFiles() {
        return (GlobalFunction_CloseFiles) functions.get("CloseFiles");
    }
    private GlobalFunction_GetPi asGetPi() {
        return (GlobalFunction_GetPi) functions.get("GetPi");
    }
    private GlobalFunction_GetServerPort asGetServerPort() {
        return (GlobalFunction_GetServerPort) functions.get("GetServerPort");
    }

    private void dispatchByName(String name, int cbId, Wire.Reader r) throws Exception {
        switch (name) {
            case "GetFile": cb_GetFile(cbId, r); return;
            case "GetFileSize": cb_GetFileSize(cbId, r); return;
            case "GetTimeNow": cb_GetTimeNow(cbId, r); return;
            case "HasFile": cb_HasFile(cbId, r); return;
            case "GetFileList": cb_GetFileList(cbId, r); return;
            case "GetOpenedFiles": cb_GetOpenedFiles(cbId, r); return;
            case "CloseFiles": cb_CloseFiles(cbId, r); return;
            case "GetPi": cb_GetPi(cbId, r); return;
            case "GetServerPort": cb_GetServerPort(cbId, r); return;
            default: throw new RuntimeException("unknown function: " + name);
        }
    }

    private void dispatchCallback(byte[] body) throws Exception {
        Wire.Reader r = new Wire.Reader(body);
        int cbId = r.i32();
        String method = r.str();
        ThrowingHandler handler = handlers.get(method);
        if (handler == null) {
            Wire.Writer w = new Wire.Writer();
            w.i32(cbId);
            w.u8(0);
            w.str("unknown method: " + method);
            sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
            return;
        }
        try {
            handler.handle(cbId, r);
        } catch (Exception e) {
            Wire.Writer w = new Wire.Writer();
            w.i32(cbId);
            w.u8(0);
            w.str(String.valueOf(e.getMessage()));
            sendMessage(Wire.M_CALLBACK_RESP, w.toByteArray());
        }
    }

    @SuppressWarnings("unchecked")
    private <T extends S5RunObject> T instanceById(String cls, int thisId) {
        T obj = (T) instances.getOrDefault(cls, Map.of()).get(thisId);
        if (obj == null) {
            throw new RuntimeException(cls + ": unknown this_id " + thisId);
        }
        return obj;
    }

    private int registerInstance(S5RunObject obj) {
        int id = ++nextThisId;
        instances.computeIfAbsent(obj.s5runClassName(), k -> new HashMap<>()).put(id, obj);
        return id;
    }

    private int readRefId(Wire.Reader r) {
        if (r.u8() != Wire.T_REF) throw new RuntimeException("wire: expected an object reference");
        int id = r.i32();
        r.str(); // skip the type name
        return id;
    }

    private static String valueToString(Value v) {
        return Wire.valueToString(v);
    }

    // ------------------------------------------------------------------
    // connection
    // ------------------------------------------------------------------

    private void connect() throws Exception {
        if (socket != null) return;
        if (servers.isEmpty()) {
            throw new RuntimeException("s5run: no server added (addServer)");
        }
        int best = 0;
        if (servers.size() > 1) {
            long bestLoad = -1;
            for (int i = 0; i < servers.size(); i++) {
                long load = probeServerLoad(servers.get(i));
                if (load < 0) continue;
                if (bestLoad < 0 || load < bestLoad) {
                    bestLoad = load;
                    best = i;
                }
            }
            if (bestLoad < 0) throw new RuntimeException("s5run: all servers are unreachable");
        }
        RemoteServerInfo srv = servers.get(best);
        socket = new Socket(srv.getHost(), srv.getPort());
        socket.setTcpNoDelay(true);
        socket.setSoTimeout(10000);
        in = socket.getInputStream();
        out = socket.getOutputStream();
        readHello(in, out);
    }

    private static void readHello(InputStream in, OutputStream out) throws Exception {
        byte[] head = in.readNBytes(5);
        if (head.length < 5) throw new RuntimeException("s5run: expected server hello");
        int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
        int msgType = head[4] & 0xFF;
        byte[] body = in.readNBytes(len - 1);
        if (msgType != Wire.M_SERVER_HELLO || body.length < len - 1)
            throw new RuntimeException("s5run: expected server hello");
        String text = new String(body, java.nio.charset.StandardCharsets.UTF_8);
        String[] parts = text.split(" ");
        if (parts.length != 3 || !parts[0].equals("s5run"))
            throw new RuntimeException("s5run: bad server hello: " + text);
        int protocol = Integer.parseInt(parts[2]);
        if (protocol != 2)
            throw new RuntimeException("s5run: server protocol version " + protocol
                    + " is not supported (client supports 2)");
    }

    private static long probeServerLoad(RemoteServerInfo server) {
        try (Socket probe = new Socket()) {
            probe.connect(new java.net.InetSocketAddress(server.getHost(), server.getPort()), 2000);
            probe.setTcpNoDelay(true);
            probe.setSoTimeout(2000);
            OutputStream po = probe.getOutputStream();
            InputStream pi = probe.getInputStream();
            readHello(pi, po);
            writeFrame(po, Wire.M_STATS_REQ, new byte[0]);
            byte[] head = pi.readNBytes(5);
            if (head.length < 5) return -1;
            int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
            int msgType = head[4] & 0xFF;
            byte[] body = pi.readNBytes(len - 1);
            if (msgType != Wire.M_STATS_RESP || body.length < 16) return -1;
            Wire.Reader r = new Wire.Reader(body);
            r.u32(); // threads
            long busy = r.u32();
            long queued = r.u32();
            return busy + queued;
        } catch (Exception e) {
            return -1;
        }
    }

    private void flushPending() throws Exception {
        for (PendingType t : new ArrayList<>(pendingTypes)) {
            sendRegisterType(t.name, t.methods);
        }
        pendingTypes.clear();
        for (PendingObject o : new ArrayList<>(pendingObjects)) {
            sendRegisterObject(o.name, o.thisId, o.methods);
        }
        pendingObjects.clear();
        for (String name : new ArrayList<>(pendingFunctions)) {
            sendRegisterFunction(name);
        }
        pendingFunctions.clear();
    }

    private void sendRegisterType(String name, List<String> methods) throws Exception {
        Wire.Writer w = new Wire.Writer();
        w.str(name);
        w.u32(methods.size());
        for (String m : methods) w.str(m);
        sendMessage(Wire.M_REGISTER_TYPE, w.toByteArray());
        expectRegisterResp("register_type(" + name + ")");
    }

    private void sendRegisterObject(String name, int thisId, List<String> methods) throws Exception {
        Wire.Writer w = new Wire.Writer();
        w.str(name);
        w.i32(thisId);
        w.str(name);
        w.u32(methods.size());
        for (String m : methods) w.str(m);
        sendMessage(Wire.M_REGISTER_OBJ, w.toByteArray());
        expectRegisterResp("register_object(" + name + ")");
    }

    private void sendRegisterFunction(String name) throws Exception {
        Wire.Writer w = new Wire.Writer();
        w.str(name);
        sendMessage(Wire.M_REGISTER_FUNC, w.toByteArray());
        expectRegisterResp("register_function(" + name + ")");
    }

    private void expectRegisterResp(String what) throws Exception {
        Frame f = readFrame();
        if (f.msgType != Wire.M_REGISTER_RESP) {
            throw new RuntimeException("s5run: " + what + " failed");
        }
        Wire.Reader r = new Wire.Reader(f.body);
        if (r.u8() == 0) {
            throw new RuntimeException("s5run: " + what + " error: " + r.str());
        }
    }

    /** Closes the connection; the context can be reused afterwards. */
    @Override
    public void close() {
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException ignored) {
            }
            socket = null;
            in = null;
            out = null;
        }
    }

    // ------------------------------------------------------------------
    // framing
    // ------------------------------------------------------------------

    private static final class Frame {
        final int msgType;
        final byte[] body;
        Frame(int msgType, byte[] body) { this.msgType = msgType; this.body = body; }
    }

    private void sendMessage(int msgType, byte[] body) throws Exception {
        writeFrame(out, msgType, body);
    }

    private static void writeFrame(OutputStream os, int msgType, byte[] body) throws Exception {
        int len = 1 + body.length;
        os.write((len >> 24) & 0xFF);
        os.write((len >> 16) & 0xFF);
        os.write((len >> 8) & 0xFF);
        os.write(len & 0xFF);
        os.write(msgType);
        os.write(body);
        os.flush();
    }

    private Frame readFrame() throws Exception {
        byte[] head = in.readNBytes(5);
        if (head.length < 5) throw new RuntimeException("s5run: connection closed");
        int len = ((head[0] & 0xFF) << 24) | ((head[1] & 0xFF) << 16) | ((head[2] & 0xFF) << 8) | (head[3] & 0xFF);
        if (len <= 0 || len > 64 * 1024 * 1024) throw new RuntimeException("s5run: bad frame length");
        int msgType = head[4] & 0xFF;
        byte[] body = in.readNBytes(len - 1);
        if (body.length < len - 1) throw new RuntimeException("s5run: connection closed");
        return new Frame(msgType, body);
    }
}
