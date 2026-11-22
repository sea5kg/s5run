// AUTO-GENERATED. DO NOT EDIT.
// TODO: not compiled yet (no Go toolchain available at generation time).
package s5runcli

import (
	"fmt"
	"net"
	"regexp"
	"time"
)

const clientVersion = "1.0.0"

type throwingHandler func(cbID int32, r *reader)

type pendingType struct {
	name    string
	methods []string
}

type pendingObject struct {
	name    string
	thisID  int
	methods []string
}

// Context is a cacheable execution context: registrations, servers, the script.
type Context struct {
	handlers        map[string]throwingHandler
	functions       map[string]interface{}
	instances       map[string]map[int]S5RunObject
	servers         []*RemoteServerInfo
	pendingTypes    []pendingType
	pendingObjects  []pendingObject
	pendingFunctions []string
	script          string
	nextID          int
	conn            net.Conn
}

// NewContext creates an empty context.
func NewContext() *Context {
	ctx := &Context{
		handlers:  map[string]throwingHandler{},
		functions: map[string]interface{}{},
		instances: map[string]map[int]S5RunObject{},
	}
	for k, v := range map[string]throwingHandler{
		"GetSize": ctx.callbackmFileGetSize,
		"ReadBytes": ctx.callbackmFileReadBytes,
		"GetName": ctx.callbackmFileGetName,
		"GetBufferSize": ctx.callbackmFileGetBufferSize,
		"SetBufferSize": ctx.callbackmFileSetBufferSize,
		"GetList": ctx.callbackmFileSystemGetList,
		"log": ctx.callbackmconsoleLog,
		"warning": ctx.callbackmconsoleWarning,
		"error": ctx.callbackmconsoleError,
	} {
		ctx.handlers[k] = v
	}
	return ctx
}

func (ctx *Context) SetScript(script string) *Context {
	ctx.script = script
	return ctx
}

func (ctx *Context) AddServer(server *RemoteServerInfo) *Context {
	ctx.servers = append(ctx.servers, server)
	return ctx
}

func (ctx *Context) AddClass(sample S5RunObject) *Context {
	name := sample.S5RunClassName()
	if ctx.conn != nil {
		ctx.sendRegisterType(name, sample.S5RunMethodNames())
	} else {
		ctx.pendingTypes = append(ctx.pendingTypes, pendingType{name, sample.S5RunMethodNames()})
	}
	return ctx
}

func (ctx *Context) AddObject(obj S5RunObject) *Context {
	cls := obj.S5RunClassName()
	ctx.nextID++
	if ctx.instances[cls] == nil {
		ctx.instances[cls] = map[int]S5RunObject{}
	}
	ctx.instances[cls][ctx.nextID] = obj
	if ctx.conn != nil {
		ctx.sendRegisterObject(cls, ctx.nextID, obj.S5RunMethodNames())
	} else {
		ctx.pendingObjects = append(ctx.pendingObjects, pendingObject{cls, ctx.nextID, obj.S5RunMethodNames()})
	}
	return ctx
}

func (ctx *Context) AddFunction(fn interface{}) *Context {
	name := s5runFunctionName(fn)
	ctx.functions[name] = fn
	switch name {
		case "GetFile":
			ctx.handlers["GetFile"] = ctx.callbackGetFile
		case "GetFileSize":
			ctx.handlers["GetFileSize"] = ctx.callbackGetFileSize
		case "GetTimeNow":
			ctx.handlers["GetTimeNow"] = ctx.callbackGetTimeNow
		case "HasFile":
			ctx.handlers["HasFile"] = ctx.callbackHasFile
		case "GetFileList":
			ctx.handlers["GetFileList"] = ctx.callbackGetFileList
		case "GetOpenedFiles":
			ctx.handlers["GetOpenedFiles"] = ctx.callbackGetOpenedFiles
		case "CloseFiles":
			ctx.handlers["CloseFiles"] = ctx.callbackCloseFiles
		case "GetPi":
			ctx.handlers["GetPi"] = ctx.callbackGetPi
		case "GetServerPort":
			ctx.handlers["GetServerPort"] = ctx.callbackGetServerPort
	}
	if ctx.conn != nil {
		ctx.sendRegisterFunction(name)
	} else {
		ctx.pendingFunctions = append(ctx.pendingFunctions, name)
	}
	return ctx
}


func (ctx *Context) callbackGetFile(cbID int32, r *reader) {
	fn := ctx.functions["GetFile"].(GlobalFunctionGetFile)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("GetFile: wrong argument count")
		}
		filepath := readStrValue(r)
		res := fn.Execute(filepath)
		id := ctx.registerInstance(res)
		w.refValue(int32(id), "File")
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackGetFileSize(cbID int32, r *reader) {
	fn := ctx.functions["GetFileSize"].(GlobalFunctionGetFileSize)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("GetFileSize: wrong argument count")
		}
		id_file, _ := readRef(r)
		file := ctx.instanceByID("File", id_file).(File)
		res := fn.Execute(file)
		writeInt(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackGetTimeNow(cbID int32, r *reader) {
	fn := ctx.functions["GetTimeNow"].(GlobalFunctionGetTimeNow)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 0 {
			panic("GetTimeNow: wrong argument count")
		}
		res := fn.Execute()
		writeInt(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackHasFile(cbID int32, r *reader) {
	fn := ctx.functions["HasFile"].(GlobalFunctionHasFile)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("HasFile: wrong argument count")
		}
		filepath := readStrValue(r)
		res := fn.Execute(filepath)
		writeBool(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackGetFileList(cbID int32, r *reader) {
	fn := ctx.functions["GetFileList"].(GlobalFunctionGetFileList)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("GetFileList: wrong argument count")
		}
		filepath := readStrValue(r)
		res := fn.Execute(filepath)
		writeArrayOf(w, res, func(e string) { writeStrValue(w, e) })
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackGetOpenedFiles(cbID int32, r *reader) {
	fn := ctx.functions["GetOpenedFiles"].(GlobalFunctionGetOpenedFiles)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 0 {
			panic("GetOpenedFiles: wrong argument count")
		}
		res := fn.Execute()
		ids := make([]int, 0, len(res))
		for _, o := range res {
			ids = append(ids, ctx.registerInstance(o))
		}
		writeArrayOf(w, ids, func(id int) { w.refValue(int32(id), "File") })
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackCloseFiles(cbID int32, r *reader) {
	fn := ctx.functions["CloseFiles"].(GlobalFunctionCloseFiles)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("CloseFiles: wrong argument count")
		}
		files := readArrayOf(r, func() File {
			id, _ := readRef(r)
			return ctx.instanceByID("File", id).(File)
		})
		fn.Execute(files)
		w.u8(tagUndefined)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackGetPi(cbID int32, r *reader) {
	fn := ctx.functions["GetPi"].(GlobalFunctionGetPi)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 0 {
			panic("GetPi: wrong argument count")
		}
		res := fn.Execute()
		writeDouble(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackGetServerPort(cbID int32, r *reader) {
	fn := ctx.functions["GetServerPort"].(GlobalFunctionGetServerPort)
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 0 {
			panic("GetServerPort: wrong argument count")
		}
		res := fn.Execute()
		writeInt(w, int64(res))
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmFileGetSize(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("File.GetSize: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("File", int(thisID)).(File)
		res := obj.GetSize()
		writeInt(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmFileReadBytes(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 2 {
			panic("File.ReadBytes: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("File", int(thisID)).(File)
		readBytes := readInt(r)
		res := obj.ReadBytes(readBytes)
		writeArrayOf(w, res, func(e byte) { writeByte(w, e) })
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmFileGetName(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("File.GetName: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("File", int(thisID)).(File)
		res := obj.GetName()
		writeStrValue(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmFileGetBufferSize(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 1 {
			panic("File.GetBufferSize: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("File", int(thisID)).(File)
		res := obj.GetBufferSize()
		writeInt(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmFileSetBufferSize(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 2 {
			panic("File.SetBufferSize: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("File", int(thisID)).(File)
		bufferSize := readInt(r)
		obj.SetBufferSize(bufferSize)
		w.u8(tagUndefined)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmFileSystemGetList(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 2 {
			panic("FileSystem.GetList: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("FileSystem", int(thisID)).(FileSystem)
		filepath := readStrValue(r)
		res := obj.GetList(filepath)
		writeArrayOf(w, res, func(e string) { writeStrValue(w, e) })
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmconsoleLog(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		n := r.u32()
		if n < 1 {
			panic("console.log: expected this_id")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("console", int(thisID)).(console)
		var args []string
		for i := uint32(0); i < n-1; i++ {
			args = append(args, valueToString(decodeValue(r)))
		}
		obj.Log(args)
		w.u8(tagUndefined)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmconsoleWarning(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		n := r.u32()
		if n < 1 {
			panic("console.warning: expected this_id")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("console", int(thisID)).(console)
		var args []string
		for i := uint32(0); i < n-1; i++ {
			args = append(args, valueToString(decodeValue(r)))
		}
		obj.Warning(args)
		w.u8(tagUndefined)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmconsoleError(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		n := r.u32()
		if n < 1 {
			panic("console.error: expected this_id")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("console", int(thisID)).(console)
		var args []string
		for i := uint32(0); i < n-1; i++ {
			args = append(args, valueToString(decodeValue(r)))
		}
		obj.Error(args)
		w.u8(tagUndefined)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) dispatchCallback(body []byte) {
	r := &reader{data: body}
	cbID := r.i32()
	method := r.rawStr()
	handler, ok := ctx.handlers[method]
	if !ok {
		w := &writer{}
		w.i32(cbID)
		w.u8(0)
		w.rawStr("unknown method: " + method)
		_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
		return
	}
	func() {
		defer func() {
			if e := recover(); e != nil {
				w := &writer{}
				w.i32(cbID)
				w.u8(0)
				w.rawStr(fmt.Sprintf("%v", e))
				_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
			}
		}()
		handler(cbID, r)
	}()
}

func (ctx *Context) instanceByID(cls string, id int) S5RunObject {
	obj, ok := ctx.instances[cls][id]
	if !ok {
		panic(cls + ": unknown this_id")
	}
	return obj
}

func (ctx *Context) registerInstance(obj S5RunObject) int {
	ctx.nextID++
	cls := obj.S5RunClassName()
	if ctx.instances[cls] == nil {
		ctx.instances[cls] = map[int]S5RunObject{}
	}
	ctx.instances[cls][ctx.nextID] = obj
	return ctx.nextID
}

func (ctx *Context) connect() error {
	if ctx.conn != nil {
		return nil
	}
	if len(ctx.servers) == 0 {
		return fmt.Errorf("s5run: no server added (AddServer)")
	}
	best := 0
	if len(ctx.servers) > 1 {
		bestLoad := int64(-1)
		for i, srv := range ctx.servers {
			load, err := probeServerLoad(srv)
			if err != nil {
				continue
			}
			if bestLoad < 0 || load < bestLoad {
				bestLoad = load
				best = i
			}
		}
		if bestLoad < 0 {
			return fmt.Errorf("s5run: all servers are unreachable")
		}
	}
	conn, err := net.DialTimeout("tcp",
		fmt.Sprintf("%s:%d", ctx.servers[best].Host(), ctx.servers[best].Port()), 10*time.Second)
	if err != nil {
		return err
	}
	ctx.conn = conn
	return readHello(conn)
}

func probeServerLoad(server *RemoteServerInfo) (int64, error) {
	conn, err := net.DialTimeout("tcp", fmt.Sprintf("%s:%d", server.Host(), server.Port()), 2*time.Second)
	if err != nil {
		return -1, err
	}
	defer conn.Close()
	_ = conn.SetDeadline(time.Now().Add(2 * time.Second))
	if err := readHello(conn); err != nil {
		return -1, err
	}
	if err := writeFrame(conn, msgStatsReq, nil); err != nil {
		return -1, err
	}
	msgType, body, err := readFrame(conn)
	if err != nil || msgType != msgStatsResp || len(body) < 16 {
		return -1, fmt.Errorf("bad stats response")
	}
	r := &reader{data: body}
	r.u32() // threads
	busy := r.u32()
	queued := r.u32()
	return int64(busy + queued), nil
}

func (ctx *Context) flushPending() error {
	for _, t := range ctx.pendingTypes {
		if err := ctx.sendRegisterType(t.name, t.methods); err != nil {
			return err
		}
	}
	ctx.pendingTypes = nil
	for _, o := range ctx.pendingObjects {
		if err := ctx.sendRegisterObject(o.name, o.thisID, o.methods); err != nil {
			return err
		}
	}
	ctx.pendingObjects = nil
	for _, name := range ctx.pendingFunctions {
		if err := ctx.sendRegisterFunction(name); err != nil {
			return err
		}
	}
	ctx.pendingFunctions = nil
	return nil
}

func (ctx *Context) sendRegisterType(name string, methods []string) error {
	w := &writer{}
	w.rawStr(name)
	w.u32(uint32(len(methods)))
	for _, m := range methods {
		w.rawStr(m)
	}
	if err := writeFrame(ctx.conn, msgRegisterType, w.buf.Bytes()); err != nil {
		return err
	}
	return ctx.expectRegisterResp("register_type(" + name + ")")
}

func (ctx *Context) sendRegisterObject(name string, thisID int, methods []string) error {
	w := &writer{}
	w.rawStr(name)
	w.i32(int32(thisID))
	w.rawStr(name)
	w.u32(uint32(len(methods)))
	for _, m := range methods {
		w.rawStr(m)
	}
	if err := writeFrame(ctx.conn, msgRegisterObj, w.buf.Bytes()); err != nil {
		return err
	}
	return ctx.expectRegisterResp("register_object(" + name + ")")
}

func (ctx *Context) sendRegisterFunction(name string) error {
	w := &writer{}
	w.rawStr(name)
	if err := writeFrame(ctx.conn, msgRegisterFunc, w.buf.Bytes()); err != nil {
		return err
	}
	return ctx.expectRegisterResp("register_function(" + name + ")")
}

func (ctx *Context) expectRegisterResp(what string) error {
	msgType, body, err := readFrame(ctx.conn)
	if err != nil || msgType != msgRegisterResp {
		return fmt.Errorf("s5run: %s failed", what)
	}
	r := &reader{data: body}
	if r.u8() == 0 {
		return fmt.Errorf("s5run: %s error: %s", what, r.rawStr())
	}
	return nil
}

// Close closes the connection (the context can be reused afterwards).
func (ctx *Context) Close() {
	if ctx.conn != nil {
		_ = ctx.conn.Close()
		ctx.conn = nil
	}
}

var errorLineRe = regexp.MustCompile(`:(\d+):\d+`)

func parseErrorLine(errMsg string) int {
	m := errorLineRe.FindStringSubmatch(errMsg)
	if m == nil {
		return 0
	}
	return parseInt(m[1])
}

// RunScript executes the context script; errors land in RunResult.
func RunScript(ctx *Context, entry string, args []Value) *RunResult {
	started := time.Now()
	fail := func(err error) *RunResult {
		return &RunResult{HasError: true, Error: err.Error(), ErrorLine: parseErrorLine(err.Error())}
	}
	if err := ctx.connect(); err != nil {
		return fail(err)
	}
	if err := ctx.flushPending(); err != nil {
		return fail(err)
	}
	w := &writer{}
	w.rawStr(ctx.script)
	w.rawStr(entry)
	w.u32(uint32(len(args)))
	for _, a := range args {
		encodeValue(w, a)
	}
	if err := writeFrame(ctx.conn, msgExecReq, w.buf.Bytes()); err != nil {
		return fail(err)
	}
	for {
		msgType, body, err := readFrame(ctx.conn)
		if err != nil {
			return fail(err)
		}
		switch msgType {
		case msgCallbackReq:
			ctx.dispatchCallback(body)
		case msgExecResp:
			r := &reader{data: body}
			if r.u8() == 0 {
				return fail(fmt.Errorf("%s", r.rawStr()))
			}
			return &RunResult{
				DurationMs: time.Since(started).Milliseconds(),
				Value:      decodeValue(r),
			}
		default:
			return fail(fmt.Errorf("s5run: unexpected message type"))
		}
	}
}
