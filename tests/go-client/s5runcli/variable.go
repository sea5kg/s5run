// AUTO-GENERATED. DO NOT EDIT.
// TODO: not compiled yet (no Go toolchain available at generation time).
package s5runcli

import (
	"bytes"
	"encoding/json"
	"fmt"
	"sort"
	"strings"
)

// S5RunVariable is any script argument/result value.
type S5RunVariable interface{ isValue() }

type Null struct{}
type Undefined struct{}
type Bool bool
type Int int64
type Float float64
type Str string
type Arr []S5RunVariable
type Obj map[string]S5RunVariable

// Ref references a remote object.
type Ref struct {
	ThisID int
	Type   string
}

func (Null) isValue()  {}
func (Undefined) isValue() {}
func (Bool) isValue()  {}
func (Int) isValue()   {}
func (Float) isValue() {}
func (Str) isValue()   {}
func (Arr) isValue()   {}
func (Obj) isValue()   {}
func (Ref) isValue()   {}

// ToStringS5Run returns the structured multi-line representation,
// handy for tests and debugging, e.g.
//
//	K_INT 1024
//	K_STRING "text\nline"
//	K_OBJECT
//	BEGIN
//	  size K_INT 1024
//	END
//
// Object fields are sorted by name.
func ToStringS5Run(v S5RunVariable) string {
	var b strings.Builder
	appendDebug(&b, v, 0)
	return strings.TrimSuffix(b.String(), "\n")
}

// ToStringJSON returns a JSON representation (encoding/json).
// A reference is encoded as {"__s5run_ref__":{"this_id":N,"type":"..."}}.
func ToStringJSON(v S5RunVariable) string {
	var buf bytes.Buffer
	enc := json.NewEncoder(&buf)
	enc.SetEscapeHTML(false)
	if err := enc.Encode(toJSONValue(v)); err != nil {
		return ""
	}
	return strings.TrimSuffix(buf.String(), "\n")
}

// appendDebug writes the structured multi-line form; the first line is
// emitted without an indent prefix (the caller positions it),
// continuation lines carry the given indent.
func appendDebug(b *strings.Builder, v S5RunVariable, indent int) {
	pad := strings.Repeat(" ", indent)
	padIn := strings.Repeat(" ", indent+2)
	switch t := v.(type) {
	case Undefined:
		b.WriteString("K_UNDEFINED\n")
	case Null:
		b.WriteString("K_NULL\n")
	case Bool:
		if t {
			b.WriteString("K_BOOL true\n")
		} else {
			b.WriteString("K_BOOL false\n")
		}
	case Int:
		fmt.Fprintf(b, "K_INT %d\n", int64(t))
	case Float:
		fmt.Fprintf(b, "K_DOUBLE %v\n", float64(t))
	case Str:
		b.WriteString("K_STRING ")
		appendJSONString(b, string(t))
		b.WriteString("\n")
	case Arr:
		b.WriteString("K_ARRAY\n")
		b.WriteString(pad + "BEGIN\n")
		for _, e := range t {
			b.WriteString(padIn)
			appendDebug(b, e, indent+2)
		}
		b.WriteString(pad + "END\n")
	case Obj:
		// sorted by name
		keys := make([]string, 0, len(t))
		for k := range t {
			keys = append(keys, k)
		}
		sort.Strings(keys)
		b.WriteString("K_OBJECT\n")
		b.WriteString(pad + "BEGIN\n")
		for _, k := range keys {
			b.WriteString(padIn + k + " ")
			appendDebug(b, t[k], indent+2)
		}
		b.WriteString(pad + "END\n")
	case Ref:
		fmt.Fprintf(b, "K_REF %d ", t.ThisID)
		appendJSONString(b, t.Type)
		b.WriteString("\n")
	}
}

// appendJSONString writes a JSON-escaped quoted string.
func appendJSONString(b *strings.Builder, s string) {
	b.WriteByte('"')
	for i := 0; i < len(s); i++ {
		c := s[i]
		switch c {
		case '"':
			b.WriteString("\\\"")
		case '\\':
			b.WriteString("\\\\")
		case '\n':
			b.WriteString("\\n")
		case '\r':
			b.WriteString("\\r")
		case '\t':
			b.WriteString("\\t")
		default:
			if c < 0x20 {
				fmt.Fprintf(b, "\\u%04x", c)
			} else {
				b.WriteByte(c)
			}
		}
	}
	b.WriteByte('"')
}

// toJSONValue converts into plain Go values for encoding/json.
func toJSONValue(v S5RunVariable) interface{} {
	switch t := v.(type) {
	case Null, Undefined:
		return nil
	case Bool:
		return bool(t)
	case Int:
		return int64(t)
	case Float:
		return float64(t)
	case Str:
		return string(t)
	case Arr:
		out := make([]interface{}, 0, len(t))
		for _, e := range t {
			out = append(out, toJSONValue(e))
		}
		return out
	case Obj:
		out := make(map[string]interface{}, len(t))
		for k, e := range t {
			out[k] = toJSONValue(e)
		}
		return out
	case Ref:
		return map[string]interface{}{
			"__s5run_ref__": map[string]interface{}{
				"this_id": t.ThisID,
				"type":    t.Type,
			},
		}
	}
	return nil
}

func encodeValue(w *writer, v S5RunVariable) {
	switch t := v.(type) {
	case Null, Undefined:
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
		writeArrayOf(w, t, func(e S5RunVariable) { encodeValue(w, e) })
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

func decodeValue(r *reader) S5RunVariable {
	tag := r.u8()
	switch tag {
	case tagUndefined:
		return Undefined{}
	case tagBool:
		return Bool(r.u8() != 0)
	case tagString:
		return Str(r.rawStr())
	case tagFloat32, tagFloat64:
		r.pos--
		return Float(readDouble(r))
	case tagArray:
		r.pos--
		return Arr(readArrayOf(r, func() S5RunVariable { return decodeValue(r) }))
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

func valueToString(v S5RunVariable) string {
	switch t := v.(type) {
	case Null:
		return "null"
	case Undefined:
		return "undefined"
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
