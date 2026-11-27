// AUTO-GENERATED. DO NOT EDIT.
// TODO: not compiled yet (no Go toolchain available at generation time).
package s5runprovider

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
	props   []S5RunProperty
}

type pendingObject struct {
	name    string
	thisID  int
	methods []string
	props   []S5RunProperty
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
	metaClasses   map[string]*S5RunClassMetadata
	metaFunctions map[string]*S5RunFunctionMetadata
	metaObjects   map[string]metaObjectEntry
}

// NewContext creates an empty context.
func NewContext() *Context {
	ctx := &Context{
		handlers:  map[string]throwingHandler{},
		functions: map[string]interface{}{},
		instances: map[string]map[int]S5RunObject{},
		metaClasses:   map[string]*S5RunClassMetadata{},
		metaFunctions: map[string]*S5RunFunctionMetadata{},
		metaObjects:   map[string]metaObjectEntry{},
	}
	for k, v := range map[string]throwingHandler{
		"insert": ctx.callbackmdbInsert,
		"find": ctx.callbackmdbFind,
		"update": ctx.callbackmdbUpdate,
		"delete": ctx.callbackmdbDelete,
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
	if m, ok := sample.(S5RunMetaObject); ok && m.S5RunMetadata() != nil {
		ctx.metaClasses[name] = m.S5RunMetadata()
	}
	if ctx.conn != nil {
		ctx.sendRegisterType(name, sample.S5RunMethodNames(), sample.S5RunPropertyDescriptors())
	} else {
		ctx.pendingTypes = append(ctx.pendingTypes, pendingType{name, sample.S5RunMethodNames(), sample.S5RunPropertyDescriptors()})
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
	if m, ok := obj.(S5RunMetaObject); ok && m.S5RunMetadata() != nil {
		ctx.metaObjects[m.S5RunMetadata().Name] = metaObjectEntry{m.S5RunMetadata(), ctx.nextID}
	}
	if ctx.conn != nil {
		ctx.sendRegisterObject(cls, ctx.nextID, obj.S5RunMethodNames(), obj.S5RunPropertyDescriptors())
	} else {
		ctx.pendingObjects = append(ctx.pendingObjects, pendingObject{cls, ctx.nextID, obj.S5RunMethodNames(), obj.S5RunPropertyDescriptors()})
	}
	return ctx
}

func (ctx *Context) AddFunction(fn interface{}) *Context {
	name := s5runFunctionName(fn)
	ctx.functions[name] = fn
	if m, ok := fn.(S5RunMetaFunction); ok && m.S5RunMetadata() != nil {
		ctx.metaFunctions[m.S5RunMetadata().Name] = m.S5RunMetadata()
	}

	switch name {

	}
	if ctx.conn != nil {
		ctx.sendRegisterFunction(name)
	} else {
		ctx.pendingFunctions = append(ctx.pendingFunctions, name)
	}
	return ctx
}


func (ctx *Context) callbackmdbInsert(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 3 {
			panic("db.insert: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("db", int(thisID)).(db)
		collection := readStrValue(r)
		doc := decodeValue(r)
		res := obj.Insert(collection, doc)
		writeStrValue(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmdbFind(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 3 {
			panic("db.find: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("db", int(thisID)).(db)
		collection := readStrValue(r)
		filter := decodeValue(r)
		res := obj.Find(collection, filter)
		writeArrayOf(w, res, func(e S5RunVariable) { encodeValue(w, e) })
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmdbUpdate(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 4 {
			panic("db.update: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("db", int(thisID)).(db)
		collection := readStrValue(r)
		id := readStrValue(r)
		doc := decodeValue(r)
		res := obj.Update(collection, id, doc)
		writeBool(w, res)
	_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
}


func (ctx *Context) callbackmdbDelete(cbID int32, r *reader) {
	w := &writer{}
	w.i32(cbID)
	w.u8(1)
		if r.u32() != 3 {
			panic("db.delete: wrong argument count")
		}
		thisID := readInt(r)
		obj := ctx.instanceByID("db", int(thisID)).(db)
		collection := readStrValue(r)
		id := readStrValue(r)
		res := obj.Delete(collection, id)
		writeBool(w, res)
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
		if err := ctx.sendRegisterType(t.name, t.methods, t.props); err != nil {
			return err
		}
	}
	ctx.pendingTypes = nil
	for _, o := range ctx.pendingObjects {
		if err := ctx.sendRegisterObject(o.name, o.thisID, o.methods, o.props); err != nil {
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

func putPropertyDescriptors(w *writer, props []S5RunProperty) {
	w.u32(uint32(len(props)))
	for _, p := range props {
		w.rawStr(p.Name)
		w.rawStr(p.Getter)
		w.rawStr(p.Setter)
	}
}

func (ctx *Context) sendRegisterType(name string, methods []string, props []S5RunProperty) error {
	w := &writer{}
	w.rawStr(name)
	w.u32(uint32(len(methods)))
	for _, m := range methods {
		w.rawStr(m)
	}
	putPropertyDescriptors(w, props)
	if err := writeFrame(ctx.conn, msgRegisterType, w.buf.Bytes()); err != nil {
		return err
	}
	return ctx.expectRegisterResp("register_type(" + name + ")")
}

func (ctx *Context) sendRegisterObject(name string, thisID int, methods []string, props []S5RunProperty) error {
	w := &writer{}
	w.rawStr(name)
	w.i32(int32(thisID))
	w.rawStr(name)
	w.u32(uint32(len(methods)))
	for _, m := range methods {
		w.rawStr(m)
	}
	putPropertyDescriptors(w, props)
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
func RunScript(ctx *Context, entry string, args []S5RunVariable) *RunResult {
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


// ---- provider mode ----

type metaObjectEntry struct {
	meta   *S5RunClassMetadata
	thisID int
}

func (ctx *Context) writeMetaFunction(w *writer, m *S5RunFunctionMetadata) {
	w.u8(1) // function
	w.rawStr(m.Name)
	w.rawStr(m.ReturnType)
	w.u32(uint32(len(m.Args)))
	for _, a := range m.Args {
		w.rawStr(a.Name)
		w.rawStr(a.Type)
	}
}

func (ctx *Context) writeMetaClass(w *writer, kind uint8, m *S5RunClassMetadata, thisID int) {
	w.u8(kind)
	w.rawStr(m.Name)
	w.u32(uint32(len(m.Methods)))
	for _, mm := range m.Methods {
		w.rawStr(mm.Name)
		w.rawStr(mm.ReturnType)
		w.u32(uint32(len(mm.Args)))
		for _, a := range mm.Args {
			w.rawStr(a.Name)
			w.rawStr(a.Type)
		}
	}
	w.u32(uint32(len(m.Properties)))
	for _, p := range m.Properties {
		w.rawStr(p.Name)
		w.rawStr(p.Getter)
		w.rawStr(p.Setter)
	}
	if kind == 3 {
		w.i32(int32(thisID))
	}
}

// handleMetaReq answers M_PROVIDER_META_REQ: metadata by name
// ("" = list everything registered).
func (ctx *Context) handleMetaReq(body []byte) {
	r := &reader{data: body}
	name := r.rawStr()
	w := &writer{}
	if name == "" {
		count := len(ctx.metaFunctions) + len(ctx.metaObjects) + len(ctx.metaClasses)
		w.u8(uint8(count))
		for _, m := range ctx.metaFunctions {
			ctx.writeMetaFunction(w, m)
		}
		for _, mo := range ctx.metaObjects {
			ctx.writeMetaClass(w, 3, mo.meta, mo.thisID)
		}
		for _, m := range ctx.metaClasses {
			ctx.writeMetaClass(w, 2, m, 0)
		}
	} else if m, ok := ctx.metaFunctions[name]; ok {
		w.u8(1)
		ctx.writeMetaFunction(w, m)
	} else if mo, ok := ctx.metaObjects[name]; ok {
		w.u8(1)
		ctx.writeMetaClass(w, 3, mo.meta, mo.thisID)
	} else if m, ok := ctx.metaClasses[name]; ok {
		w.u8(1)
		ctx.writeMetaClass(w, 2, m, 0)
	} else {
		w.u8(0) // not found
	}
	_ = writeFrame(ctx.conn, msgProviderMetaResp, w.buf.Bytes())
}

// Serve starts the provider server on host:port, waits for one client
// and serves metadata requests + API calls until the client disconnects.
func Serve(ctx *Context, host string, port int) error {
	ln, err := net.Listen("tcp", fmt.Sprintf("%s:%d", host, port))
	if err != nil {
		return err
	}
	conn, err := ln.Accept()
	ln.Close()
	if err != nil {
		return err
	}
	defer conn.Close()
	if tcp, ok := conn.(*net.TCPConn); ok {
		_ = tcp.SetNoDelay(true)
	}
	ctx.conn = conn

	// hello: "s5run <version> <protocol>"
	hello := "s5run " + ClientVersion() + " 2"
	if err := writeFrame(conn, msgServerHello, []byte(hello)); err != nil {
		return err
	}

	for {
		msgType, body, err := readFrame(conn)
		if err != nil {
			return nil // client disconnected
		}
		switch msgType {
		case msgProviderMetaReq:
			ctx.handleMetaReq(body)
		case msgCallbackReq:
			func() {
				defer func() {
					if e := recover(); e != nil {
						w := &writer{}
						w.i32(0)
						w.u8(0)
						w.rawStr(fmt.Sprintf("%v", e))
						_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())
					}
				}()
				ctx.dispatchCallback(body)
			}()
		default:
			return nil // unexpected message
		}
	}
}
