// AUTO-GENERATED. DO NOT EDIT.
// TODO: not compiled yet (no Go toolchain available at generation time).
package s5runprovider

// S5RunArgMetadata describes a function/method argument.
type S5RunArgMetadata struct {
	Name string
	Type string
}

// S5RunMethodMetadata describes a method signature.
type S5RunMethodMetadata struct {
	Name       string
	ReturnType string
	Args       []S5RunArgMetadata
}

// S5RunClassMetadata describes a class: methods and properties.
type S5RunClassMetadata struct {
	Name       string
	Methods    []S5RunMethodMetadata
	Properties []S5RunProperty
}

// S5RunFunctionMetadata describes a global function.
type S5RunFunctionMetadata struct {
	Name       string
	ReturnType string
	Args       []S5RunArgMetadata
}


// S5RunProperty describes a JS accessor property: reading obj.<Name>
// calls the Getter method, writing calls the Setter.
type S5RunProperty struct {
	Name   string
	Getter string
	Setter string
}

// S5RunObject is the root of all generated classes.
type S5RunObject interface {
	S5RunClassName() string
	S5RunMethodNames() []string
	S5RunPropertyDescriptors() []S5RunProperty
}

// S5RunMetaObject is implemented by the generated base types
// (returns nil for custom types without a generated base).
type S5RunMetaObject interface {
	S5RunMetadata() *S5RunClassMetadata
}

// S5RunMetaFunction is implemented by the generated function bases.
type S5RunMetaFunction interface {
	S5RunMetadata() *S5RunFunctionMetadata
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
	Value      S5RunVariable
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


// db is the schema class 'db' - implement it and register.
type db interface {
	S5RunObject
	Insert(collection string, doc S5RunVariable) string
	Find(collection string, filter S5RunVariable) []S5RunVariable
	Update(collection string, id string, doc S5RunVariable) bool
	Delete(collection string, id string) bool
}

type DbBase struct{}

func (DbBase) S5RunClassName() string { return "db" }
func (DbBase) S5RunMetadata() *S5RunClassMetadata { return DbMetadata }
func (DbBase) S5RunPropertyDescriptors() []S5RunProperty {
	return []S5RunProperty{}
}
func (DbBase) S5RunMethodNames() []string {
	return []string{"insert", "find", "update", "delete"}
}
var DbMetadata = &S5RunClassMetadata{
	Name: "db",
	Methods: []S5RunMethodMetadata{
		{Name: "insert", ReturnType: "string", Args: []S5RunArgMetadata{{Name: "collection", Type: "string"},{Name: "doc", Type: "var"},}},
		{Name: "find", ReturnType: "array<var>", Args: []S5RunArgMetadata{{Name: "collection", Type: "string"},{Name: "filter", Type: "var"},}},
		{Name: "update", ReturnType: "bool", Args: []S5RunArgMetadata{{Name: "collection", Type: "string"},{Name: "id", Type: "string"},{Name: "doc", Type: "var"},}},
		{Name: "delete", ReturnType: "bool", Args: []S5RunArgMetadata{{Name: "collection", Type: "string"},{Name: "id", Type: "string"},}},
	},
	Properties: []S5RunProperty{},
}
