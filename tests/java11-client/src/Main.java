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
            java.util.List<Byte> out = new java.util.ArrayList<>();
            for (long i = 0; i < readBytes; ++i) {
                out.add((byte) ('a' + (int) (i % 26)));
            }
            return out;
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

    // ---- data-driven scenarios (like the cpp clients) ----

    static final class Scenario {
        String name;
        String scriptJs;
        String outputConsoleExpected;
        String outputReturnExpected; // "" = no return check
        java.util.List<String> inputArguments = new java.util.ArrayList<>();
    }

    static String readFileOrDefault(String path, String def) {
        try {
            return new String(java.nio.file.Files.readAllBytes(java.nio.file.Paths.get(path)),
                    java.nio.charset.StandardCharsets.UTF_8);
        } catch (Exception e) {
            return def;
        }
    }

    static java.util.List<String> splitLines(String text) {
        java.util.List<String> out = new java.util.ArrayList<>();
        for (String line : text.split("\n")) {
            if (line.endsWith("\r")) {
                line = line.substring(0, line.length() - 1);
            }
            if (!line.isEmpty()) {
                out.add(line);
            }
        }
        return out;
    }

    static Scenario loadScenario(String base) {
        Scenario s = new Scenario();
        s.name = base.substring(base.lastIndexOf('/') + 1);
        s.scriptJs = readFileOrDefault(base + "/script.js", null);
        if (s.scriptJs == null) {
            throw new RuntimeException("cannot find " + base + "/script.js");
        }
        s.outputConsoleExpected = readFileOrDefault(base + "/output_console_expected.txt", "");
        s.outputReturnExpected = readFileOrDefault(base + "/output_return_expected.txt", "");
        s.inputArguments = splitLines(readFileOrDefault(base + "/input_arguments.txt", ""));
        return s;
    }

    // Scan __test_data for client_scenarioX directories.
    static java.util.List<Scenario> listScenarios() {
        String[] roots = {"tests/__test_data", "../__test_data"};
        java.util.List<Scenario> out = new java.util.ArrayList<>();
        for (String root : roots) {
            java.io.File dir = new java.io.File(root);
            java.io.File[] sub = dir.listFiles();
            if (sub == null) {
                continue;
            }
            java.util.List<String> names = new java.util.ArrayList<>();
            for (java.io.File f : sub) {
                String n = f.getName();
                if (f.isDirectory() && n.startsWith("client_") && n.contains("_scenario")) {
                    names.add(n);
                }
            }
            java.util.Collections.sort(names);
            for (String n : names) {
                out.add(loadScenario(root + "/" + n));
            }
            if (!out.isEmpty()) {
                return out; // the first root that has scenarios
            }
        }
        throw new RuntimeException("no client_scenarioX found in __test_data");
    }

    // Parse a raw argument into a typed value: int -> bool -> string.
    static S5RunVariable parseInputArgument(String raw) {
        try {
            return new S5RunVariable.Int(Long.parseLong(raw));
        } catch (NumberFormatException e) {
            // not an int
        }
        if (raw.equals("true")) {
            return new S5RunVariable.Bool(true);
        }
        if (raw.equals("false")) {
            return new S5RunVariable.Bool(false);
        }
        return new S5RunVariable.Str(raw);
    }

    // Compare the script return value with output_return_expected.txt:
    // both sides use S5RunVariable.toStringS5Run() format.
    static void checkScenarioReturn(String name, S5RunVariable got, String expectedRaw) {
        java.util.List<String> expected = splitLines(expectedRaw);
        java.util.List<String> gotLines = splitLines(got.toStringS5Run());
        if (gotLines.equals(expected)) {
            return;
        }
        StringBuilder details = new StringBuilder();
        int n = Math.max(gotLines.size(), expected.size());
        for (int i = 0; i < n; i++) {
            String gotLine = i < gotLines.size() ? gotLines.get(i) : "<missing>";
            String expLine = i < expected.size() ? expected.get(i) : "<missing>";
            if (!gotLine.equals(expLine)) {
                details.append("  line ").append(i + 1).append(": got \"").append(gotLine)
                       .append("\" expected \"").append(expLine).append("\"\n");
            }
        }
        throw new RuntimeException(name + ": return value mismatch:\n" + details);
    }

    static String address;

    public static void main(String[] argv) {
        String host = argv.length > 0 ? argv[0] : "127.0.0.1";
        int port = argv.length > 1 ? Integer.parseInt(argv[1]) : 7777;
        address = "s5run://" + host + ":" + port;

        if (!S5RunContext.clientVersion().equals("1.0.0")) {
            throw new RuntimeException("unexpected client version: " + S5RunContext.clientVersion());
        }

        scenarios();
        primitivesWithTwoServers();
        errorPath();

        System.out.println("java11-client: all smoke tests passed");
    }

    static void scenarios() {
        java.util.List<Scenario> tests = listScenarios();
        for (Scenario test : tests) {
            System.out.println("== java11-client: " + test.name);
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
                    .setScript(test.scriptJs)) {

            MyConsole.logs.clear();
            java.util.List<S5RunVariable> args = new java.util.ArrayList<>();
            for (String raw : test.inputArguments) {
                args.add(parseInputArgument(raw));
            }
            S5RunResult result = S5RunContext.runScript(ctx, "main", args);
            if (result.hasError()) {
                System.out.println("  SCRIPT ERROR: " + result.error());
                System.out.flush();
                throw new RuntimeException(test.name + ": script error: " + result.error());
            }
            System.out.println("  duration: " + result.durationMs() + " ms");

            // return value: compare with output_return_expected.txt
            if (!test.outputReturnExpected.isEmpty()) {
                checkScenarioReturn(test.name, result.value(), test.outputReturnExpected);
                System.out.println("  return value: " + result.value().toStringS5Run().replace("\n", " | "));
                System.out.println("  return json:  " + result.value().toStringJson());
                System.out.println("  return value matches output_return_expected.txt");
            } else {
                System.out.println("  return value: no output_return_expected.txt, skipped");
            }

            // console output: compare with output_console_expected.txt (line per log)
            java.util.List<String> expectedConsole = splitLines(test.outputConsoleExpected);
            if (!MyConsole.logs.equals(expectedConsole)) {
                StringBuilder details = new StringBuilder();
                int n = Math.max(MyConsole.logs.size(), expectedConsole.size());
                for (int i = 0; i < n; i++) {
                    String gotLine = i < MyConsole.logs.size() ? MyConsole.logs.get(i) : "<missing>";
                    String expLine = i < expectedConsole.size() ? expectedConsole.get(i) : "<missing>";
                    if (!gotLine.equals(expLine)) {
                        details.append("  line ").append(i + 1).append(": got \"").append(gotLine)
                               .append("\" expected \"").append(expLine).append("\"\n");
                    }
                }
                throw new RuntimeException(test.name + ": console output mismatch:\n" + details);
            }
            System.out.println("  console output matches " + MyConsole.logs.size() + " lines");

            // side-effect checks (callback state, to be moved into data files)
            if (test.name.equals("client_scenario0")) {
                check(MyGetFile.lastPath.equals("test.txt"), "GetFile path");
                check(MyFileSystem.lastPath.equals("test.txt"), "FileSystem.GetList path");
                check(MyCloseFiles.closedSizes.equals(List.of(100L, 200L)), "CloseFiles sizes");
            }
            System.out.println("  PASS");
            }
        }
    }

    static void primitivesWithTwoServers() {
        System.out.println("== java11-client: primitives (two servers)");
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
        System.out.println("== java11-client: error path");
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
