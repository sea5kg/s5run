import com.s5run.example.*;

import java.util.List;

public class Main {

    static final class MyFile implements File {
        private final long size;
        private long bufferSize = 4096;

        MyFile(long size) {
            this.size = size;
        }

        @Override
        public long GetSize() {
            return size;
        }

        @Override
        public List<Byte> ReadBytes(long readBytes) {
            return List.of();
        }

        @Override
        public String GetName() {
            return "example.txt";
        }

        @Override
        public long GetBufferSize() {
            return bufferSize;
        }

        @Override
        public void SetBufferSize(long bufferSize) {
            this.bufferSize = bufferSize;
        }
    }

    static final class MyGetFile implements GlobalFunction_GetFile {
        static String lastPath = "";

        @Override
        public File execute(String filepath) {
            lastPath = filepath;
            return new MyFile(1024);
        }
    }

    static final class MyGetFileSize implements GlobalFunction_GetFileSize {
        @Override
        public long execute(File file) {
            return file.GetSize();
        }
    }

    static final class MyGetTimeNow implements GlobalFunction_GetTimeNow {
        @Override
        public long execute() {
            return 1737000000L;
        }
    }

    static final class MyHasFile implements GlobalFunction_HasFile {
        @Override
        public boolean execute(String filepath) {
            throw new RuntimeException("boom-hasfile");
        }
    }

    static final class MyGetOpenedFiles implements GlobalFunction_GetOpenedFiles {
        @Override
        public List<File> execute() {
            return List.of(new MyFile(100), new MyFile(200));
        }
    }

    static final class MyCloseFiles implements GlobalFunction_CloseFiles {
        static List<Long> closedSizes = new java.util.ArrayList<>();

        @Override
        public void execute(List<File> files) {
            for (File f : files) {
                closedSizes.add(f.GetSize());
            }
        }
    }

    static final class MyGetPi implements GlobalFunction_GetPi {
        @Override
        public double execute() {
            return 3.14;
        }
    }

    static final class MyGetServerPort implements GlobalFunction_GetServerPort {
        @Override
        public long execute() {
            return 8080;
        }
    }

    static final class MyConsole implements console {
        static final List<String> logs = new java.util.ArrayList<>();

        @Override
        public void log(List<String> args) {
            logs.add(String.join(" ", args));
        }

        @Override
        public void warning(List<String> args) {
        }

        @Override
        public void error(List<String> args) {
        }
    }

    static final class MyFileSystem implements FileSystem {
        static String lastPath = "";

        @Override
        public List<String> GetList(String filepath) {
            lastPath = filepath;
            return List.of("a.txt", "b.txt", "c.txt");
        }
    }

    static final String K_SCRIPT = loadBaseScript();

    private static String loadBaseScript() {
        String[] paths = {"tests/__test_data/client_scenario0/script.js", "../__test_data/client_scenario0/script.js"};
        for (String p : paths) {
            try {
                return new String(java.nio.file.Files.readAllBytes(java.nio.file.Paths.get(p)),
                        java.nio.charset.StandardCharsets.UTF_8);
            } catch (Exception e) {
                // try the next path
            }
        }
        throw new RuntimeException("cannot find tests/__test_data/client_scenario0/script.js");
    }

    static String address;

    public static void main(String[] argv) {
        String host = argv.length > 0 ? argv[0] : "127.0.0.1";
        int port = argv.length > 1 ? Integer.parseInt(argv[1]) : 7777;
        address = "s5run://" + host + ":" + port;

        if (!S5RunContext.clientVersion().equals("1.0.0")) {
            throw new RuntimeException("unexpected client version: " + S5RunContext.clientVersion());
        }

        happyPath();
        primitivesWithTwoServers();
        errorPath();

        System.out.println("java17-client: all smoke tests passed");
    }

    static void happyPath() {
        System.out.println("== java17-client: happy path");
        try (S5RunContext ctx = new S5RunContext()
                .addClass(new MyFile(1024))
                .addFunction(new MyGetFile())
                .addFunction(new MyGetFileSize())
                .addFunction(new MyGetTimeNow())
                .addFunction(new MyGetOpenedFiles())
                .addFunction(new MyCloseFiles())
                .addFunction(new MyGetPi())
                .addFunction(new MyGetServerPort())
                .addObject(new MyFileSystem())
                .addObject(new MyConsole())
                .addServer(new S5RunRemoteServerInfo().address(address).apiKey("some api key runner"))
                .setScript(K_SCRIPT)) {

        S5RunResult result = S5RunContext.runScript(ctx, "main", List.of(new S5RunVariable.Str("test.txt")));
        if (result.hasError()) {
            throw new RuntimeException("unexpected error: " + result.error());
        }
        System.out.println("  duration: " + result.durationMs() + " ms");

        var obj = ((S5RunVariable.Obj) result.value()).v();
        check(((S5RunVariable.Int) obj.get("size")).v() == 1024, "size");
        check(((S5RunVariable.Int) obj.get("sizeByRef")).v() == 1024, "sizeByRef");
        check(((S5RunVariable.Int) obj.get("openedCount")).v() == 2, "openedCount");
        check(((S5RunVariable.Int) obj.get("openedSum")).v() == 300, "openedSum");
        check(((S5RunVariable.Int) obj.get("fileListCount")).v() == 3, "fileListCount");
        check(((S5RunVariable.Int) obj.get("time")).v() == 1737000000L, "time");
        check(MyGetFile.lastPath.equals("test.txt"), "GetFile path");
        check(MyFileSystem.lastPath.equals("test.txt"), "FileSystem.GetList path");
        check(MyConsole.logs.get(0).equals("hello test.txt 1 2"), "console.log");
        check(MyConsole.logs.size() == 4, "console.log count");
        check(MyConsole.logs.get(1).equals("name: example.txt"), "property Name getter");
        check(MyConsole.logs.get(2).equals("bufsize: 4096"), "property BufferSize getter");
        check(MyConsole.logs.get(3).equals("bufsize-after: 2048"), "property BufferSize setter");
        check(MyCloseFiles.closedSizes.equals(List.of(100L, 200L)), "CloseFiles sizes");
        System.out.println("  PASS");
        }
    }

    static void primitivesWithTwoServers() {
        System.out.println("== java17-client: primitives (two servers)");
        try (S5RunContext ctx = new S5RunContext()
                .addFunction(new MyGetPi())
                .addFunction(new MyGetServerPort())
                .addServer(new S5RunRemoteServerInfo().address(address))
                .addServer(new S5RunRemoteServerInfo().address(address)) // same: exercises load balancing
                .setScript("function main() { return { pi: GetPi(), port: GetServerPort() }; }")) {

        S5RunResult result = S5RunContext.runScript(ctx, "main", List.of());
        if (result.hasError()) {
            throw new RuntimeException("unexpected error: " + result.error());
        }
        var obj = ((S5RunVariable.Obj) result.value()).v();
        double pi = ((S5RunVariable.Dbl) obj.get("pi")).v();
        check(pi > 3.13 && pi < 3.15, "pi");
        check(((S5RunVariable.Int) obj.get("port")).v() == 8080, "port");
        System.out.println("  PASS");
        }
    }

    static void errorPath() {
        System.out.println("== java17-client: error path");
        try (S5RunContext ctx = new S5RunContext()
                .addFunction(new MyHasFile())
                .addServer(new S5RunRemoteServerInfo().address(address))
                .setScript("function main() {\n  return HasFile(\"x\");\n}")) {

        S5RunResult result = S5RunContext.runScript(ctx, "main", List.of());
        if (!result.hasError()) {
            throw new RuntimeException("expected 'boom-hasfile' error");
        }
        System.out.println("  error: " + result.error());
        System.out.println("  error line: " + result.errorLine());
        check(result.error().contains("boom-hasfile"), "boom-hasfile in error");
        check(result.errorLine() == 2, "error line");
        System.out.println("  PASS");
        }
    }

    static void check(boolean cond, String what) {
        if (!cond) {
            throw new RuntimeException("unexpected " + what);
        }
    }
}
