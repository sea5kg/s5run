package main

import (
	"fmt"
	"os"
	"sort"
	"strconv"
	"strings"

	"s5runcli/s5runcli"
)

// ---- data-driven scenarios (like the cpp/java clients) ----

type scenario struct {
	name                  string
	scriptJs              string
	outputConsoleExpected string
	outputReturnExpected  string // "" = no return check
	inputArguments        []string
}

func readOrDefault(path, def string) string {
	b, err := os.ReadFile(path)
	if err != nil {
		return def
	}
	return string(b)
}

func splitLines(text string) []string {
	var out []string
	for _, line := range strings.Split(text, "\n") {
		line = strings.TrimSuffix(line, "\r")
		if line != "" {
			out = append(out, line)
		}
	}
	return out
}

func loadScenario(base string) scenario {
	name := base
	if i := strings.LastIndex(base, "/"); i >= 0 {
		name = base[i+1:]
	}
	return scenario{
		name:                  name,
		scriptJs:              readOrDefault(base+"/script.js", ""),
		outputConsoleExpected: readOrDefault(base+"/output_console_expected.txt", ""),
		outputReturnExpected:  readOrDefault(base+"/output_return_expected.txt", ""),
		inputArguments:        splitLines(readOrDefault(base+"/input_arguments.txt", "")),
	}
}

// listScenarios scans __test_data for client_scenarioX directories.
func listScenarios() []scenario {
	var out []scenario
	for _, root := range []string{"tests/__test_data", "../__test_data"} {
		entries, err := os.ReadDir(root)
		if err != nil {
			continue
		}
		var names []string
		for _, e := range entries {
			n := e.Name()
			if e.IsDir() && strings.HasPrefix(n, "client_") && strings.Contains(n, "_scenario") {
				names = append(names, n)
			}
		}
		sort.Strings(names)
		for _, n := range names {
			out = append(out, loadScenario(root+"/"+n))
		}
		if len(out) > 0 {
			return out // the first root that has scenarios
		}
	}
	panic("no client_scenarioX found in __test_data")
}

// parseInputArgument: int -> bool -> string.
func parseInputArgument(raw string) s5runcli.S5RunVariable {
	if v, err := strconv.ParseInt(raw, 10, 64); err == nil {
		return s5runcli.Int(v)
	}
	if raw == "true" {
		return s5runcli.Bool(true)
	}
	if raw == "false" {
		return s5runcli.Bool(false)
	}
	return s5runcli.Str(raw)
}

// checkScenarioReturn compares the script return value with
// output_return_expected.txt (both use ToStringS5Run()).
func checkScenarioReturn(name string, got s5runcli.S5RunVariable, expectedRaw string) {
	expected := splitLines(expectedRaw)
	gotLines := splitLines(s5runcli.ToStringS5Run(got))
	if strings.Join(gotLines, "\n") == strings.Join(expected, "\n") {
		return
	}
	var details strings.Builder
	n := len(gotLines)
	if len(expected) > n {
		n = len(expected)
	}
	for i := 0; i < n; i++ {
		gotLine := "<missing>"
		if i < len(gotLines) {
			gotLine = gotLines[i]
		}
		expLine := "<missing>"
		if i < len(expected) {
			expLine = expected[i]
		}
		if gotLine != expLine {
			fmt.Fprintf(&details, "  line %d: got %q expected %q\n", i+1, gotLine, expLine)
		}
	}
	fmt.Fprintf(os.Stderr, "!!! FAIL: %s: return value mismatch:\n%s\n", name, details.String())
	os.Exit(1)
}

type MyFile struct {
	s5runcli.FileBase
	size       int64
	bufferSize int64
}

func newMyFile(size int64) *MyFile {
	return &MyFile{size: size, bufferSize: 4096}
}

func (f *MyFile) GetSize() int64 { return f.size }
func (f *MyFile) ReadBytes(readBytes int64) []byte {
	out := make([]byte, readBytes)
	for i := range out {
		out[i] = byte('a' + i%26)
	}
	return out
}
func (f *MyFile) GetName() string       { return "example.txt" }
func (f *MyFile) GetBufferSize() int64  { return f.bufferSize }
func (f *MyFile) SetBufferSize(v int64) { f.bufferSize = v }

var lastGetFilePath string

type MyGetFile struct{}

func (MyGetFile) S5RunFunctionName() string { return "GetFile" }
func (MyGetFile) Execute(filepath string) s5runcli.File {
	lastGetFilePath = filepath
	return newMyFile(1024)
}

type MyGetFileSize struct{}

func (MyGetFileSize) S5RunFunctionName() string        { return "GetFileSize" }
func (MyGetFileSize) Execute(file s5runcli.File) int64 { return file.GetSize() }

type MyGetTimeNow struct{}

func (MyGetTimeNow) S5RunFunctionName() string { return "GetTimeNow" }
func (MyGetTimeNow) Execute() int64            { return 1737000000 }

type MyHasFile struct{}

func (MyHasFile) S5RunFunctionName() string { return "HasFile" }
func (MyHasFile) Execute(filepath string) bool {
	panic("boom-hasfile")
}

var closeFilesSizes []int64

type MyCloseFiles struct{}

func (MyCloseFiles) S5RunFunctionName() string { return "CloseFiles" }
func (MyCloseFiles) Execute(files []s5runcli.File) {
	closeFilesSizes = nil
	for _, f := range files {
		closeFilesSizes = append(closeFilesSizes, f.GetSize())
	}
}

type MyGetOpenedFiles struct{}

func (MyGetOpenedFiles) S5RunFunctionName() string { return "GetOpenedFiles" }
func (MyGetOpenedFiles) Execute() []s5runcli.File {
	return []s5runcli.File{&MyFile{size: 100}, &MyFile{size: 200}}
}

var consoleLogs []string

type MyConsole struct {
	s5runcli.ConsoleBase
}

func (MyConsole) Log(args []string)     { consoleLogs = append(consoleLogs, strings.Join(args, " ")) }
func (MyConsole) Warning(args []string) {}
func (MyConsole) Error(args []string)   {}

var lastGetListPath string

type MyFileSystem struct {
	s5runcli.FileSystemBase
}

func (MyFileSystem) GetList(filepath string) []string {
	lastGetListPath = filepath
	return []string{"a.txt", "b.txt", "c.txt"}
}

func check(cond bool, what string) {
	if !cond {
		fmt.Fprintf(os.Stderr, "!!! FAIL: unexpected %s\n", what)
		os.Exit(1)
	}
}

func main() {
	host := "127.0.0.1"
	port := 7777
	if len(os.Args) > 1 {
		host = os.Args[1]
	}
	if len(os.Args) > 2 {
		p, _ := strconv.Atoi(os.Args[2])
		port = p
	}
	address := fmt.Sprintf("s5run://%s:%d", host, port)

	// data-driven scenarios
	for _, test := range listScenarios() {
		fmt.Printf("== go-client: %s\n", test.name)
		func() {
			ctx := s5runcli.NewContext()
			ctx.AddClass(newMyFile(1024)).
				AddFunction(MyGetFile{}).
				AddFunction(MyGetFileSize{}).
				AddFunction(MyGetTimeNow{}).
				AddFunction(MyGetOpenedFiles{}).
				AddFunction(MyCloseFiles{}).
				AddObject(MyFileSystem{}).
				AddObject(MyConsole{}).
				AddServer(s5runcli.NewRemoteServerInfo().Address(address)).
				SetScript(test.scriptJs)

			consoleLogs = consoleLogs[:0]
			var args []s5runcli.S5RunVariable
			for _, raw := range test.inputArguments {
				args = append(args, parseInputArgument(raw))
			}
			result := s5runcli.RunScript(ctx, "main", args)
			if result.HasError {
				fmt.Fprintf(os.Stderr, "!!! FAIL: unexpected error: %s\n", result.Error)
				os.Exit(1)
			}
			fmt.Printf("  duration: %dms\n", result.DurationMs)

			// return value: compare with output_return_expected.txt
			if test.outputReturnExpected != "" {
				checkScenarioReturn(test.name, result.Value, test.outputReturnExpected)
				fmt.Printf("  return value: %s\n", strings.ReplaceAll(s5runcli.ToStringS5Run(result.Value), "\n", " | "))
				fmt.Printf("  return json:  %s\n", s5runcli.ToStringJSON(result.Value))
				fmt.Println("  return value matches output_return_expected.txt")
			} else {
				fmt.Println("  return value: no output_return_expected.txt, skipped")
			}

			// console output: compare with output_console_expected.txt (line per log)
			expectedConsole := splitLines(test.outputConsoleExpected)
			if strings.Join(consoleLogs, "\n") != strings.Join(expectedConsole, "\n") {
				var details strings.Builder
				n := len(consoleLogs)
				if len(expectedConsole) > n {
					n = len(expectedConsole)
				}
				for i := 0; i < n; i++ {
					gotLine := "<missing>"
					if i < len(consoleLogs) {
						gotLine = consoleLogs[i]
					}
					expLine := "<missing>"
					if i < len(expectedConsole) {
						expLine = expectedConsole[i]
					}
					if gotLine != expLine {
						fmt.Fprintf(&details, "  line %d: got %q expected %q\n", i+1, gotLine, expLine)
					}
				}
				fmt.Fprintf(os.Stderr, "!!! FAIL: %s: console output mismatch:\n%s\n",
					test.name, details.String())
				os.Exit(1)
			}
			fmt.Printf("  console output matches %d lines\n", len(consoleLogs))

			// side-effect checks (callback state, to be moved into data files)
			if test.name == "client_scenario0" {
				check(lastGetFilePath == "test.txt", "GetFile path")
				check(lastGetListPath == "test.txt", "FileSystem.GetList path")
				check(len(closeFilesSizes) == 2 && closeFilesSizes[0] == 100 && closeFilesSizes[1] == 200,
					"CloseFiles sizes")
			}
			fmt.Println("  PASS")
		}()
	}

	// error path
	fmt.Println("== go-client: error path")
	{
		ctx := s5runcli.NewContext()
		ctx.AddFunction(MyHasFile{}).
			AddServer(s5runcli.NewRemoteServerInfo().Address(address)).
			SetScript("function main() {\n  return HasFile(\"x\");\n}")

		result := s5runcli.RunScript(ctx, "main", nil)
		check(result.HasError, "expected error")
		fmt.Printf("  error: %s\n", strings.SplitN(result.Error, "\n", 2)[0])
		fmt.Printf("  error line: %d\n", result.ErrorLine)
		check(strings.Contains(result.Error, "boom-hasfile"), "boom-hasfile")
		check(result.ErrorLine == 2, "error line")
		fmt.Println("  PASS")
	}

	fmt.Println("go-client: all smoke tests passed")
}
