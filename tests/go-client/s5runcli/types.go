// AUTO-GENERATED. DO NOT EDIT.
// TODO: not compiled yet (no Go toolchain available at generation time).
package s5runcli

// S5RunObject is the root of all generated classes.
type S5RunObject interface {
	S5RunClassName() string
	S5RunMethodNames() []string
}

// S5RunFunction is the root of all generated global functions.
type S5RunFunction interface {
	S5RunFunctionName() string
}

// RemoteServerInfo holds settings of a remote s5run server (fluent).
type RemoteServerInfo struct {
	address string
	apiKey  string
	host    string
	port    int
}

func NewRemoteServerInfo() *RemoteServerInfo { return &RemoteServerInfo{} }

func (s *RemoteServerInfo) Address(address string) *RemoteServerInfo {
	const prefix = "s5run://"
	if len(address) <= len(prefix) || address[:len(prefix)] != prefix {
		panic("expected format s5run://host:port")
	}
	rest := address[len(prefix):]
	for i := len(rest) - 1; i > 0; i-- {
		if rest[i] == ':' {
			s.host = rest[:i]
			s.port = parseInt(rest[i+1:])
			s.address = address
			return s
		}
	}
	panic("expected format s5run://host:port")
}

func (s *RemoteServerInfo) ApiKey(apiKey string) *RemoteServerInfo {
	s.apiKey = apiKey
	return s
}

func (s *RemoteServerInfo) Host() string { return s.host }
func (s *RemoteServerInfo) Port() int    { return s.port }

// RunResult is the outcome of a script run.
type RunResult struct {
	HasError   bool
	Error      string
	ErrorLine  int
	DurationMs int64
	Value      Value
}

// ClientVersion returns the generated client version.
func ClientVersion() string { return clientVersion }

// s5runFunctionName returns the function name of any generated function
// implementation (via the S5RunFunction interface).
func s5runFunctionName(fn interface{}) string {
	return fn.(S5RunFunction).S5RunFunctionName()
}

func parseInt(s string) int {
	v := 0
	for _, c := range s {
		if c < '0' || c > '9' {
			panic("expected digits")
		}
		v = v*10 + int(c-'0')
	}
	return v
}


// File is the schema class 'File' - implement it and register.
type File interface {
	S5RunObject
	GetSize() int64
	ReadBytes(readBytes int64) []byte
	GetName() string
	GetBufferSize() int64
	SetBufferSize(bufferSize int64)
}

type FileBase struct{}

func (FileBase) S5RunClassName() string { return "File" }
func (FileBase) S5RunMethodNames() []string {
	return []string{"GetSize", "ReadBytes", "GetName", "GetBufferSize", "SetBufferSize"}
}


// FileSystem is the schema class 'FileSystem' - implement it and register.
type FileSystem interface {
	S5RunObject
	GetList(filepath string) []string
}

type FileSystemBase struct{}

func (FileSystemBase) S5RunClassName() string { return "FileSystem" }
func (FileSystemBase) S5RunMethodNames() []string {
	return []string{"GetList"}
}


// console is the schema class 'console' - implement it and register.
type console interface {
	S5RunObject
	Log(args []string)
	Warning(args []string)
	Error(args []string)
}

type ConsoleBase struct{}

func (ConsoleBase) S5RunClassName() string { return "console" }
func (ConsoleBase) S5RunMethodNames() []string {
	return []string{"log", "warning", "error"}
}


// KeyValueStorage is the schema class 'KeyValueStorage' - implement it and register.
type KeyValueStorage interface {
	S5RunObject
	SetValueByKey(name string, value Value)
	GetValueByKey(name string)
}

type KeyValueStorageBase struct{}

func (KeyValueStorageBase) S5RunClassName() string { return "KeyValueStorage" }
func (KeyValueStorageBase) S5RunMethodNames() []string {
	return []string{"SetValueByKey", "GetValueByKey"}
}


// GlobalFunctionGetFile is the global function 'GetFile'.
type GlobalFunctionGetFile interface {
	S5RunFunction
	Execute(filepath string) File
}

type globalFunctionGetFileBase struct{}

func (globalFunctionGetFileBase) S5RunFunctionName() string { return "GetFile" }


// GlobalFunctionGetFileSize is the global function 'GetFileSize'.
type GlobalFunctionGetFileSize interface {
	S5RunFunction
	Execute(file File) int64
}

type globalFunctionGetFileSizeBase struct{}

func (globalFunctionGetFileSizeBase) S5RunFunctionName() string { return "GetFileSize" }


// GlobalFunctionGetTimeNow is the global function 'GetTimeNow'.
type GlobalFunctionGetTimeNow interface {
	S5RunFunction
	Execute() int64
}

type globalFunctionGetTimeNowBase struct{}

func (globalFunctionGetTimeNowBase) S5RunFunctionName() string { return "GetTimeNow" }


// GlobalFunctionHasFile is the global function 'HasFile'.
type GlobalFunctionHasFile interface {
	S5RunFunction
	Execute(filepath string) bool
}

type globalFunctionHasFileBase struct{}

func (globalFunctionHasFileBase) S5RunFunctionName() string { return "HasFile" }


// GlobalFunctionGetFileList is the global function 'GetFileList'.
type GlobalFunctionGetFileList interface {
	S5RunFunction
	Execute(filepath string) []string
}

type globalFunctionGetFileListBase struct{}

func (globalFunctionGetFileListBase) S5RunFunctionName() string { return "GetFileList" }


// GlobalFunctionGetOpenedFiles is the global function 'GetOpenedFiles'.
type GlobalFunctionGetOpenedFiles interface {
	S5RunFunction
	Execute() []File
}

type globalFunctionGetOpenedFilesBase struct{}

func (globalFunctionGetOpenedFilesBase) S5RunFunctionName() string { return "GetOpenedFiles" }


// GlobalFunctionCloseFiles is the global function 'CloseFiles'.
type GlobalFunctionCloseFiles interface {
	S5RunFunction
	Execute(files []File)
}

type globalFunctionCloseFilesBase struct{}

func (globalFunctionCloseFilesBase) S5RunFunctionName() string { return "CloseFiles" }


// GlobalFunctionGetPi is the global function 'GetPi'.
type GlobalFunctionGetPi interface {
	S5RunFunction
	Execute() float64
}

type globalFunctionGetPiBase struct{}

func (globalFunctionGetPiBase) S5RunFunctionName() string { return "GetPi" }


// GlobalFunctionGetServerPort is the global function 'GetServerPort'.
type GlobalFunctionGetServerPort interface {
	S5RunFunction
	Execute() uint64
}

type globalFunctionGetServerPortBase struct{}

func (globalFunctionGetServerPortBase) S5RunFunctionName() string { return "GetServerPort" }
