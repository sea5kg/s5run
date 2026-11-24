package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	"s5runcli/s5runcli"
)

var kScript = loadBaseScript()

func loadBaseScript() string {
	for _, p := range []string{"tests/base_cli_test_script.js", "../base_cli_test_script.js"} {
		if b, err := os.ReadFile(p); err == nil {
			return string(b)
		}
	}
	panic("cannot find tests/base_cli_test_script.js")
}

type MyFile struct {
	s5runcli.FileBase
	size int64
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
func (f *MyFile) GetBufferSize() int64  { return 4096 }
func (f *MyFile) SetBufferSize(int64)   {}

var lastGetFilePath string

type MyGetFile struct{}

func (MyGetFile) S5RunFunctionName() string { return "GetFile" }
func (MyGetFile) Execute(filepath string) s5runcli.File {
	lastGetFilePath = filepath
	return &MyFile{size: 1024}
}

type MyGetFileSize struct{}

func (MyGetFileSize) S5RunFunctionName() string { return "GetFileSize" }
func (MyGetFileSize) Execute(file s5runcli.File) int64 { return file.GetSize() }

type MyGetTimeNow struct{}

func (MyGetTimeNow) S5RunFunctionName() string { return "GetTimeNow" }
func (MyGetTimeNow) Execute() int64 { return 1737000000 }

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

func (MyConsole) Log(args []string)  { consoleLogs = append(consoleLogs, strings.Join(args, " ")) }
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

	// happy path
	fmt.Println("== go-client: happy path")
	{
		ctx := s5runcli.NewContext()
		ctx.AddClass(&MyFile{size: 1024}).
			AddFunction(MyGetFile{}).
			AddFunction(MyGetFileSize{}).
			AddFunction(MyGetTimeNow{}).
			AddFunction(MyGetOpenedFiles{}).
			AddFunction(MyCloseFiles{}).
			AddObject(MyFileSystem{}).
			AddObject(MyConsole{}).
			AddServer(s5runcli.NewRemoteServerInfo().Address(address)).
			SetScript(kScript)

		result := s5runcli.RunScript(ctx, "main", []s5runcli.Variable{s5runcli.Str("test.txt")})
		if result.HasError {
			fmt.Fprintf(os.Stderr, "!!! FAIL: %s\n", result.Error)
			os.Exit(1)
		}
		fmt.Printf("  duration: %dms\n", result.DurationMs)

		obj := result.Value.(s5runcli.Obj)
		check(obj["size"].(s5runcli.Int) == 1024, "size")
		check(obj["sizeByRef"].(s5runcli.Int) == 1024, "sizeByRef")
		check(obj["openedCount"].(s5runcli.Int) == 2, "openedCount")
		check(obj["openedSum"].(s5runcli.Int) == 300, "openedSum")
		check(obj["fileListCount"].(s5runcli.Int) == 3, "fileListCount")
		check(obj["time"].(s5runcli.Int) == 1737000000, "time")
		check(lastGetFilePath == "test.txt", "GetFile path")
		check(lastGetListPath == "test.txt", "FileSystem.GetList path")
		check(len(consoleLogs) == 1 && consoleLogs[0] == "hello test.txt 1 2", "console.log")
		check(len(closeFilesSizes) == 2 && closeFilesSizes[0] == 100 && closeFilesSizes[1] == 200,
			"CloseFiles sizes")
		fmt.Println("  PASS")
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
