// AUTO-GENERATED. DO NOT EDIT.
// TODO: not compiled yet (no Go toolchain available at generation time).
package s5runcli

import "fmt"

// Variable is any script argument/result value.
type Variable interface{ isValue() }

type Null struct{}
type Bool bool
type Int int64
type Float float64
type Str string
type Arr []Variable
type Obj map[string]Variable

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

func encodeValue(w *writer, v Variable) {
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
		writeArrayOf(w, t, func(e Variable) { encodeValue(w, e) })
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

func decodeValue(r *reader) Variable {
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
		return Arr(readArrayOf(r, func() Variable { return decodeValue(r) }))
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

func valueToString(v Variable) string {
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
