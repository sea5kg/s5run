/**********************************************************************************
 *
 *            ░░░░░░░
 *           ░██▓▓▓▓▒
 *  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 *  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
 *   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 *  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
 *   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
 *
 * MIT License
 *
 * Copyright (c) 2026 Evgenii Sopov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Original repository: https://github.com/sea5kg/s5run
 *
 ***********************************************************************************/

#include "s5run_generators.h"
#include "s5run_schema.h"

#include <sys/stat.h>

#include <fstream>
#include <string>
#include <vector>

namespace s5run {
namespace generators {

// -----------------------------------------------------------------------
// Type helpers
// -----------------------------------------------------------------------

static bool is_scalar_type(const std::string &t) {
  return t == "bool" || t == "int" || t == "uint" || t == "float" ||
         t == "double" || t == "string" || t == "byte";
}

static bool is_void(const std::string &t) {
  return t == "void";
}

static bool is_array_type(const std::string &t) {
  return t.size() > 7 && t.substr(0, 6) == "array<" && t[t.size() - 1] == '>';
}

static std::string array_inner(const std::string &t) {
  return t.substr(6, t.size() - 7);
}

static bool is_ref_type(const std::string &t) {
  return !is_scalar_type(t) && !is_void(t) && !is_array_type(t);
}

static bool has_convert(const std::vector<schema_argument> &args) {
  return args.size() == 1 && !args[0].convert_from.empty();
}

// Exported Go name: capitalize (GetSize stays, log -> Log).
static std::string go_name(const std::string &name) {
  if (name.empty())
    return name;
  std::string out = name;
  if (out[0] >= 'a' && out[0] <= 'z')
    out[0] = static_cast<char>(out[0] - 'a' + 'A');
  return out;
}

static std::string go_scalar_type(const std::string &t) {
  if (t == "bool") return "bool";
  if (t == "int") return "int64";
  if (t == "uint") return "uint64";
  if (t == "float" || t == "double") return "float64";
  if (t == "string") return "string";
  if (t == "byte") return "byte";
  return t;
}

static std::string go_type(const std::string &t) {
  if (is_array_type(t))
    return "[]" + go_type(array_inner(t));
  return go_scalar_type(t);
}

static std::string go_reader(const std::string &t) {
  if (t == "bool") return "readBool(r)";
  if (t == "int" || t == "uint") return "readInt(r)";
  if (t == "float" || t == "double") return "readDouble(r)";
  if (t == "string") return "readStrValue(r)";
  if (t == "byte") return "readByte(r)";
  return t;
}

static std::string go_writer_expr(const std::string &t, const std::string &expr) {
  if (t == "bool") return "writeBool(w, " + expr + ")";
  if (t == "int") return "writeInt(w, " + expr + ")";
  if (t == "uint") return "writeInt(w, int64(" + expr + "))";
  if (t == "float" || t == "double") return "writeDouble(w, " + expr + ")";
  if (t == "string") return "writeStrValue(w, " + expr + ")";
  if (t == "byte") return "writeByte(w, " + expr + ")";
  return expr;
}

static std::string read_stmt(const std::string &t, const std::string &var) {
  if (is_array_type(t)) {
    std::string inner = array_inner(t);
    return var + " := readArrayOf(r, func() " + go_type(inner) + " { return "
           + go_reader(inner) + " })";
  }
  return var + " := " + go_reader(t);
}

static std::string write_stmt(const std::string &t, const std::string &expr) {
  if (is_array_type(t)) {
    std::string inner = array_inner(t);
    return "writeArrayOf(w, " + expr + ", func(e " + go_type(inner) + ") { "
           + go_writer_expr(inner, "e") + " })";
  }
  return go_writer_expr(t, expr);
}

// -----------------------------------------------------------------------
// Static template blocks (wire.go, value.go)
// -----------------------------------------------------------------------

static const char *WIRE_TEMPLATE = R"GO(import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"math"
	"net"
)

// Wire tags.
const (
	tagUndefined uint8 = 0x00
	tagBool      uint8 = 0x01
	tagInt8      uint8 = 0x02
	tagInt16     uint8 = 0x03
	tagInt32     uint8 = 0x04
	tagInt64     uint8 = 0x05
	tagUint8     uint8 = 0x06
	tagUint16    uint8 = 0x07
	tagUint32    uint8 = 0x08
	tagUint64    uint8 = 0x09
	tagFloat32   uint8 = 0x0A
	tagFloat64   uint8 = 0x0B
	tagString    uint8 = 0x0C
	tagArray     uint8 = 0x0D
	tagObject    uint8 = 0x0E
	tagRef       uint8 = 0x0F
)

// Message types.
const (
	msgExecReq       uint8 = 0x01
	msgExecResp      uint8 = 0x02
	msgCallbackReq   uint8 = 0x03
	msgCallbackResp  uint8 = 0x04
	msgRegisterFunc  uint8 = 0x05
	msgRegisterObj   uint8 = 0x06
	msgRegisterType  uint8 = 0x07
	msgRegisterResp  uint8 = 0x08
	msgStatsReq      uint8 = 0x09
	msgStatsResp     uint8 = 0x0A
	msgServerHello   uint8 = 0x0F
)

// readHello consumes the server greeting "s5run <version> <protocol>".
func writeBool(w *writer, v bool) {
	w.u8(tagBool)
	if v {
		w.u8(1)
	} else {
		w.u8(0)
	}
}

func writeDouble(w *writer, v float64) {
	w.u8(tagFloat64)
	w.f64(v)
}

func writeStrValue(w *writer, v string) {
	w.u8(tagString)
	w.rawStr(v)
}

func writeByte(w *writer, v byte) {
	w.u8(tagUint8)
	w.u8(v)
}

func readHello(conn net.Conn) error {
	msgType, body, err := readFrame(conn)
	if err != nil || msgType != msgServerHello {
		return errors.New("s5run: expected server hello")
	}
	text := string(body)
	if len(text) < 6 || text[:6] != "s5run " {
		return errors.New("s5run: bad server hello: " + text)
	}
	for i := len(text) - 1; i > 5; i-- {
		if text[i] == ' ' {
			if parseInt(text[i+1:]) != 2 {
				return errors.New("s5run: server protocol version is not supported (client supports 2)")
			}
			return nil
		}
	}
	return errors.New("s5run: bad server hello: " + text)
}

type writer struct{ buf bytes.Buffer }

func (w *writer) u8(v uint8)   { w.buf.WriteByte(v) }
func (w *writer) i32(v int32)  { var b [4]byte; binary.LittleEndian.PutUint32(b[:], uint32(v)); w.buf.Write(b[:]) }
func (w *writer) u32(v uint32) { var b [4]byte; binary.LittleEndian.PutUint32(b[:], v); w.buf.Write(b[:]) }
func (w *writer) i64(v int64)  { var b [8]byte; binary.LittleEndian.PutUint64(b[:], uint64(v)); w.buf.Write(b[:]) }
func (w *writer) f64(v float64) {
	var b [8]byte
	binary.LittleEndian.PutUint64(b[:], math.Float64bits(v))
	w.buf.Write(b[:])
}
func (w *writer) rawStr(s string) {
	raw := []byte(s)
	switch {
	case len(raw) <= 0xFF:
		w.u8(0x00)
		w.u8(uint8(len(raw)))
	case len(raw) <= 0xFFFF:
		w.u8(0x01)
		var b [2]byte
		binary.LittleEndian.PutUint16(b[:], uint16(len(raw)))
		w.buf.Write(b[:])
	default:
		w.u8(0x02)
		w.u32(uint32(len(raw)))
	}
	w.buf.Write(raw)
}
func (w *writer) strValue(s string) { w.u8(tagString); w.rawStr(s) }
func (w *writer) boolValue(v bool)  { w.u8(tagBool); if v { w.u8(1) } else { w.u8(0) } }
func (w *writer) byteValue(v byte)  { w.u8(tagUint8); w.u8(v) }
func (w *writer) doubleValue(v float64) { w.u8(tagFloat64); w.f64(v) }
func (w *writer) refValue(id int32, cls string) { w.u8(tagRef); w.i32(id); w.rawStr(cls) }

func writeInt(w *writer, v int64) {
	switch {
	case v >= -128 && v <= 127:
		w.u8(tagInt8)
		w.u8(uint8(v))
	case v >= -32768 && v <= 32767:
		w.u8(tagInt16)
		var b [2]byte
		binary.LittleEndian.PutUint16(b[:], uint16(v))
		w.buf.Write(b[:])
	case v >= -2147483648 && v <= 2147483647:
		w.u8(tagInt32)
		w.i32(int32(v))
	default:
		w.u8(tagInt64)
		w.i64(v)
	}
}

type reader struct {
	data []byte
	pos  int
}

func (r *reader) u8() uint8 {
	v := r.data[r.pos]
	r.pos++
	return v
}

func (r *reader) u32() uint32 {
	v := binary.LittleEndian.Uint32(r.data[r.pos:])
	r.pos += 4
	return v
}

func (r *reader) i32() int32  { return int32(r.u32()) }
func (r *reader) i64() int64  { v := binary.LittleEndian.Uint64(r.data[r.pos:]); r.pos += 8; return int64(v) }
func (r *reader) f64() float64 {
	v := binary.LittleEndian.Uint64(r.data[r.pos:])
	r.pos += 8
	return math.Float64frombits(v)
}

func (r *reader) rawStr() string {
	lenSize := r.u8()
	var n int
	switch lenSize {
	case 0x00:
		n = int(r.u8())
	case 0x01:
		n = int(binary.LittleEndian.Uint16(r.data[r.pos:]))
		r.pos += 2
	case 0x02:
		n = int(r.u32())
	default:
		panic("wire: bad string length size")
	}
	s := string(r.data[r.pos : r.pos+n])
	r.pos += n
	return s
}

func readInt(r *reader) int64 {
	tag := r.u8()
	switch tag {
	case tagInt8:
		return int64(int8(r.u8()))
	case tagInt16:
		v := int64(int16(binary.LittleEndian.Uint16(r.data[r.pos:])))
		r.pos += 2
		return v
	case tagInt32, tagUint32:
		return int64(r.i32())
	case tagInt64, tagUint64:
		return r.i64()
	case tagUint8:
		return int64(r.u8())
	case tagUint16:
		return int64(binary.LittleEndian.Uint16(r.data[r.pos:]))
	}
	panic("wire: expected an integer")
}

func readBool(r *reader) bool {
	if r.u8() != tagBool {
		panic("wire: expected a bool")
	}
	return r.u8() != 0
}

func readDouble(r *reader) float64 {
	tag := r.u8()
	if tag == tagFloat32 {
		v := binary.LittleEndian.Uint32(r.data[r.pos:])
		r.pos += 4
		return float64(math.Float32frombits(v))
	}
	if tag == tagFloat64 {
		return r.f64()
	}
	panic("wire: expected a double")
}

func readStrValue(r *reader) string {
	if r.u8() != tagString {
		panic("wire: expected a string")
	}
	return r.rawStr()
}

func readByte(r *reader) byte {
	if r.u8() != tagUint8 {
		panic("wire: expected a byte")
	}
	return r.u8()
}

func readRef(r *reader) (int, string) {
	if r.u8() != tagRef {
		panic("wire: expected an object reference")
	}
	id := r.i32()
	cls := r.rawStr()
	return int(id), cls
}

func readArrayOf[T any](r *reader, elem func() T) []T {
	if r.u8() != tagArray {
		panic("wire: expected an array")
	}
	n := r.u32()
	out := make([]T, 0, n)
	for i := uint32(0); i < n; i++ {
		out = append(out, elem())
	}
	return out
}

func writeArrayOf[T any](w *writer, items []T, elem func(T)) {
	w.u8(tagArray)
	w.u32(uint32(len(items)))
	for _, e := range items {
		elem(e)
	}
}

// ---- framing ----

func writeFrame(conn net.Conn, msgType uint8, body []byte) error {
	var head [5]byte
	binary.BigEndian.PutUint32(head[:4], uint32(len(body)+1))
	head[4] = msgType
	if _, err := conn.Write(head[:]); err != nil {
		return err
	}
	if len(body) > 0 {
		if _, err := conn.Write(body); err != nil {
			return err
		}
	}
	return nil
}

func readFrame(conn net.Conn) (uint8, []byte, error) {
	var head [5]byte
	if _, err := io.ReadFull(conn, head[:]); err != nil {
		return 0, nil, err
	}
	length := binary.BigEndian.Uint32(head[:4])
	if length == 0 || length > 64*1024*1024 {
		return 0, nil, errors.New("s5run: bad frame length")
	}
	body := make([]byte, length-1)
	if _, err := io.ReadFull(conn, body); err != nil {
		return 0, nil, err
	}
	return head[4], body, nil
}

var _ = fmt.Sprintf // keep fmt for debugging
)GO";

static const char *VALUE_TEMPLATE = R"GO(import "fmt"

// Value is any script argument/result value.
type Value interface{ isValue() }

type Null struct{}
type Bool bool
type Int int64
type Float float64
type Str string
type Arr []Value
type Obj map[string]Value

// Ref references a remote object.
type Ref struct {
	ThisID int
	Type   string
}

func (Null) isValue()  {}
func (Bool) isValue()  {}
func (Int) isValue()   {}
func (Float) isValue() {}
func (Str) isValue()   {}
func (Arr) isValue()   {}
func (Obj) isValue()   {}
func (Ref) isValue()   {}

func encodeValue(w *writer, v Value) {
	switch t := v.(type) {
	case Null:
		w.u8(tagUndefined)
	case Bool:
		w.boolValue(bool(t))
	case Int:
		writeInt(w, int64(t))
	case Float:
		w.doubleValue(float64(t))
	case Str:
		w.strValue(string(t))
	case Arr:
		writeArrayOf(w, t, func(e Value) { encodeValue(w, e) })
	case Obj:
		w.u8(tagObject)
		w.u32(uint32(len(t)))
		for k, e := range t {
			w.strValue(k)
			encodeValue(w, e)
		}
	case Ref:
		w.refValue(int32(t.ThisID), t.Type)
	default:
		panic("cannot encode value")
	}
}

func decodeValue(r *reader) Value {
	tag := r.u8()
	switch tag {
	case tagUndefined:
		return Null{}
	case tagBool:
		return Bool(r.u8() != 0)
	case tagString:
		return Str(r.rawStr())
	case tagFloat32, tagFloat64:
		r.pos--
		return Float(readDouble(r))
	case tagArray:
		r.pos--
		return Arr(readArrayOf(r, func() Value { return decodeValue(r) }))
	case tagObject:
		out := Obj{}
		for i := r.u32(); i > 0; i-- {
			k := readStrValue(r)
			out[k] = decodeValue(r)
		}
		return out
	case tagRef:
		r.pos--
		id, cls := readRef(r)
		return Ref{ThisID: id, Type: cls}
	default:
		r.pos--
		return Int(readInt(r))
	}
}

func valueToString(v Value) string {
	switch t := v.(type) {
	case Null:
		return "null"
	case Bool:
		if t {
			return "true"
		}
		return "false"
	case Int:
		return fmt.Sprintf("%d", int64(t))
	case Float:
		return fmt.Sprintf("%v", float64(t))
	case Str:
		return string(t)
	}
	panic("wire: expected a scalar argument")
}
)GO";

// -----------------------------------------------------------------------
// types.go
// -----------------------------------------------------------------------

static std::string generate_types(const schema &s) {
  std::string out = R"GO(
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
)GO";

  for (const auto &c : s.classes) {
    std::string methods;
    for (const auto &m : c.methods) {
      std::string ret = is_void(m.return_type) ? "" : " " + go_type(m.return_type);
      std::string args;
      for (const auto &a : m.arguments) {
        if (!args.empty())
          args += ", ";
        args += a.name + " " + go_type(a.type);
      }
      methods += "\t" + go_name(m.name) + "(" + args + ")" + ret + "\n";
    }
    std::string names;
    for (const auto &m : c.methods) {
      if (!names.empty())
        names += ", ";
      names += "\"" + m.name + "\"";
    }
    out += "\n\n// " + c.name + " is the schema class '" + c.name + "' - implement it and register.\n";
    out += "type " + c.name + " interface {\n";
    out += "\tS5RunObject\n";
    out += methods;
    out += "}\n\n";
    out += "type " + go_name(c.name) + "Base struct{}\n\n";
    out += "func (" + go_name(c.name) + "Base) S5RunClassName() string { return \"" + c.name + "\" }\n";
    out += "func (" + go_name(c.name) + "Base) S5RunMethodNames() []string {\n";
    out += "\treturn []string{" + names + "}\n";
    out += "}\n";
  }

  for (const auto &f : s.functions) {
    std::string ret = is_void(f.return_type) ? "" : " " + go_type(f.return_type);
    std::string args;
    for (const auto &a : f.arguments) {
      if (!args.empty())
        args += ", ";
      args += a.name + " " + go_type(a.type);
    }
    out += "\n\n// GlobalFunction" + f.name + " is the global function '" + f.name + "'.\n";
    out += "type GlobalFunction" + f.name + " interface {\n";
    out += "\tS5RunFunction\n";
    out += "\tExecute(" + args + ")" + ret + "\n";
    out += "}\n\n";
    out += "type globalFunction" + f.name + "Base struct{}\n\n";
    out += "func (globalFunction" + f.name + "Base) S5RunFunctionName() string { return \"" + f.name + "\" }\n";
  }
  return out;
}

// -----------------------------------------------------------------------
// Callback generation (functions and class methods)
// -----------------------------------------------------------------------

// Emits body lines: decode arguments, call, encode result.
static void emit_decode_arg(std::vector<std::string> &body, const schema_argument &a) {
  const std::string &t = a.type;
  if (is_ref_type(t)) {
    body.push_back("\tid_" + a.name + ", _ := readRef(r)");
    body.push_back("\t" + a.name + " := ctx.instanceByID(\"" + t + "\", id_" + a.name + ").(" + t + ")");
  } else if (is_array_type(t) && is_ref_type(array_inner(t))) {
    std::string cls = array_inner(t);
    body.push_back("\t" + a.name + " := readArrayOf(r, func() " + cls + " {");
    body.push_back("\t\tid, _ := readRef(r)");
    body.push_back("\t\treturn ctx.instanceByID(\"" + cls + "\", id).(" + cls + ")");
    body.push_back("\t})");
  } else {
    body.push_back("\t" + read_stmt(t, a.name));
  }
}

static void emit_encode_result(std::vector<std::string> &body, const std::string &t) {
  if (is_ref_type(t)) {
    body.push_back("\tid := ctx.registerInstance(res)");
    body.push_back("\tw.refValue(int32(id), \"" + t + "\")");
  } else if (is_array_type(t) && is_ref_type(array_inner(t))) {
    std::string cls = array_inner(t);
    body.push_back("\tids := make([]int, 0, len(res))");
    body.push_back("\tfor _, o := range res {");
    body.push_back("\t\tids = append(ids, ctx.registerInstance(o))");
    body.push_back("\t}");
    body.push_back("\twriteArrayOf(w, ids, func(id int) { w.refValue(int32(id), \"" + cls + "\") })");
  } else {
    body.push_back("\t" + write_stmt(t, "res"));
  }
}

static std::string join_indented(const std::vector<std::string> &body) {
  std::string out;
  for (const auto &b : body) {
    if (!out.empty())
      out += "\n";
    out += "\t" + b;
  }
  return out;
}

static std::string generate_function_cb(const schema_function &f) {
  std::vector<std::string> body;
  if (has_convert(f.arguments)) {
    const schema_argument &a = f.arguments[0];
    body.push_back("\tn := r.u32()");
    body.push_back("\tvar " + a.name + " []string");
    body.push_back("\tfor i := uint32(0); i < n; i++ {");
    body.push_back("\t\t" + a.name + " = append(" + a.name + ", valueToString(decodeValue(r)))");
    body.push_back("\t}");
  } else {
    body.push_back("\tif r.u32() != " + std::to_string(f.arguments.size()) + " {");
    body.push_back("\t\tpanic(\"" + f.name + ": wrong argument count\")");
    body.push_back("\t}");
    for (const auto &a : f.arguments)
      emit_decode_arg(body, a);
  }

  std::string call;
  for (const auto &a : f.arguments) {
    if (!call.empty())
      call += ", ";
    call += a.name;
  }
  if (is_void(f.return_type)) {
    body.push_back("\tfn.Execute(" + call + ")");
    body.push_back("\tw.u8(tagUndefined)");
  } else {
    body.push_back("\tres := fn.Execute(" + call + ")");
    emit_encode_result(body, f.return_type);
  }

  return "\nfunc (ctx *Context) callback" + f.name + "(cbID int32, r *reader) {\n"
         "\tfn := ctx.functions[\"" + f.name + "\"].(GlobalFunction" + f.name + ")\n"
         "\tw := &writer{}\n"
         "\tw.i32(cbID)\n"
         "\tw.u8(1)\n"
         + join_indented(body) + "\n"
         "\t_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())\n"
         "}\n";
}

static std::string generate_method_cb(const schema_class &c, const schema_method &m) {
  std::vector<std::string> body;
  if (has_convert(m.arguments)) {
    const schema_argument &a0 = m.arguments[0];
    body.push_back("\tn := r.u32()");
    body.push_back("\tif n < 1 {");
    body.push_back("\t\tpanic(\"" + c.name + "." + m.name + ": expected this_id\")");
    body.push_back("\t}");
    body.push_back("\tthisID := readInt(r)");
    body.push_back("\tobj := ctx.instanceByID(\"" + c.name + "\", int(thisID)).(" + c.name + ")");
    body.push_back("\tvar " + a0.name + " []string");
    body.push_back("\tfor i := uint32(0); i < n-1; i++ {");
    body.push_back("\t\t" + a0.name + " = append(" + a0.name + ", valueToString(decodeValue(r)))");
    body.push_back("\t}");
  } else {
    body.push_back("\tif r.u32() != " + std::to_string(1 + m.arguments.size()) + " {");
    body.push_back("\t\tpanic(\"" + c.name + "." + m.name + ": wrong argument count\")");
    body.push_back("\t}");
    body.push_back("\tthisID := readInt(r)");
    body.push_back("\tobj := ctx.instanceByID(\"" + c.name + "\", int(thisID)).(" + c.name + ")");
    for (const auto &a : m.arguments)
      emit_decode_arg(body, a);
  }

  std::string call;
  for (const auto &a : m.arguments) {
    if (!call.empty())
      call += ", ";
    call += a.name;
  }
  if (is_void(m.return_type)) {
    body.push_back("\tobj." + go_name(m.name) + "(" + call + ")");
    body.push_back("\tw.u8(tagUndefined)");
  } else {
    body.push_back("\tres := obj." + go_name(m.name) + "(" + call + ")");
    emit_encode_result(body, m.return_type);
  }

  return "\nfunc (ctx *Context) callbackm" + c.name + go_name(m.name) + "(cbID int32, r *reader) {\n"
         "\tw := &writer{}\n"
         "\tw.i32(cbID)\n"
         "\tw.u8(1)\n"
         + join_indented(body) + "\n"
         "\t_ = writeFrame(ctx.conn, msgCallbackResp, w.buf.Bytes())\n"
         "}\n";
}

// -----------------------------------------------------------------------
// context.go
// -----------------------------------------------------------------------

static std::string generate_context(const schema &s) {
  std::string fn_switch;
  for (const auto &f : s.functions) {
    if (!fn_switch.empty())
      fn_switch += "\n";
    fn_switch += "\t\tcase \"" + f.name + "\":";
    fn_switch += "\n\t\t\tctx.handlers[\"" + f.name + "\"] = ctx.callback" + f.name;
  }

  std::string method_handlers;
  for (const auto &c : s.classes) {
    for (const auto &m : c.methods) {
      if (!method_handlers.empty())
        method_handlers += "\n";
      method_handlers += "\t\t\"" + m.name + "\": ctx.callbackm" + c.name + go_name(m.name) + ",";
    }
  }

  std::vector<std::string> cb_blocks;
  for (const auto &f : s.functions)
    cb_blocks.push_back(generate_function_cb(f));
  for (const auto &c : s.classes)
    for (const auto &m : c.methods)
      cb_blocks.push_back(generate_method_cb(c, m));
  std::string cb_funcs;
  for (size_t i = 0; i < cb_blocks.size(); ++i) {
    if (i > 0)
      cb_funcs += "\n";
    cb_funcs += cb_blocks[i];
  }

  std::string out = R"GO(import (
	"fmt"
	"net"
	"regexp"
	"time"
)

const clientVersion = ")GO";
  out += s.client_version + "\"\n\n";

  out += R"GO(type throwingHandler func(cbID int32, r *reader)

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
)GO";
  out += method_handlers;
  out += R"GO(
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
)GO";
  out += fn_switch;
  out += R"GO(
	}
	if ctx.conn != nil {
		ctx.sendRegisterFunction(name)
	} else {
		ctx.pendingFunctions = append(ctx.pendingFunctions, name)
	}
	return ctx
}

)GO";
  out += cb_funcs;
  out += R"GO(

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
)GO";
  return out;
}

// -----------------------------------------------------------------------
// Generator
// -----------------------------------------------------------------------

class generator_golang : public IGenerator {
public:
  std::vector<std::string> generate(const schema &s,
                                     const std::string &output_dir) override;
};

static std::string trim(const std::string &s) {
  size_t b = s.find_first_not_of(" \t\r\n");
  if (b == std::string::npos)
    return "";
  size_t e = s.find_last_not_of(" \t\r\n");
  return s.substr(b, e - b + 1);
}

static void write_go_file(const std::string &pkg_dir, const std::string &pkg,
                          const std::string &name, const std::string &body,
                          std::vector<std::string> &written) {
  std::string path = pkg_dir + "/" + name + ".go";
  std::ofstream f(path.c_str());
  f << "// AUTO-GENERATED. DO NOT EDIT.\n";
  f << "// TODO: not compiled yet (no Go toolchain available at generation time).\n";
  f << "package " << pkg << "\n\n";
  f << trim(body) << "\n";
  f.close();
  written.push_back(path);
}

std::vector<std::string> generator_golang::generate(const schema &s,
                                                    const std::string &output_dir) {
  std::vector<std::string> written;
  std::string pkg = s.golang.package_.empty() ? "s5runcli" : s.golang.package_;
  ::mkdir(output_dir.c_str(), 0755);

  write_go_file(output_dir, pkg, "wire", WIRE_TEMPLATE, written);
  write_go_file(output_dir, pkg, "value", VALUE_TEMPLATE, written);
  write_go_file(output_dir, pkg, "types", generate_types(s), written);
  write_go_file(output_dir, pkg, "context", generate_context(s), written);

  return written;
}

std::shared_ptr<IGenerator> make_generator_golang() {
  return std::make_shared<generator_golang>();
}

} // namespace generators
} // namespace s5run
