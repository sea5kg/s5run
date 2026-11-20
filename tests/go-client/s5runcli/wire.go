// AUTO-GENERATED. DO NOT EDIT.
// TODO: not compiled yet (no Go toolchain available at generation time).
package s5runcli

import (
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
		return int64(int16(binary.LittleEndian.Uint16(r.data[r.pos:])))
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
