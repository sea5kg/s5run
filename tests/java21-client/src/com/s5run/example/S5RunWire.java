// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.*;

public final class S5RunWire {
    public static final int
        T_UNDEFINED = 0x00, T_BOOL = 0x01, T_INT8 = 0x02, T_INT16 = 0x03,
        T_INT32 = 0x04, T_INT64 = 0x05, T_UINT8 = 0x06, T_UINT16 = 0x07,
        T_UINT32 = 0x08, T_UINT64 = 0x09, T_FLOAT32 = 0x0A, T_FLOAT64 = 0x0B,
        T_STRING = 0x0C, T_ARRAY = 0x0D, T_OBJECT = 0x0E, T_REF = 0x0F;

    public static final int
        M_EXEC_REQ = 0x01, M_EXEC_RESP = 0x02, M_CALLBACK_REQ = 0x03,
        M_CALLBACK_RESP = 0x04, M_REGISTER_FUNC = 0x05, M_REGISTER_OBJ = 0x06,
        M_REGISTER_TYPE = 0x07, M_REGISTER_RESP = 0x08,
        M_STATS_REQ = 0x09, M_STATS_RESP = 0x0A, M_SERVER_HELLO = 0x0F;

    public static final class Writer {
        private final ByteArrayOutputStream buf = new ByteArrayOutputStream();

        public byte[] toByteArray() { return buf.toByteArray(); }

        public void u8(int v) { buf.write(v & 0xFF); }

        public void u32(long v) {
            for (int i = 0; i < 4; i++) buf.write((int) ((v >>> (8 * i)) & 0xFF));
        }

        public void u64(long v) {
            for (int i = 0; i < 8; i++) buf.write((int) ((v >>> (8 * i)) & 0xFF));
        }

        public void i32(int v) { u32(v & 0xFFFFFFFFL); }
        public void int64(long v) {
            if (v >= Byte.MIN_VALUE && v <= Byte.MAX_VALUE) { u8(T_INT8); u8((int) v); }
            else if (v >= Short.MIN_VALUE && v <= Short.MAX_VALUE) { u8(T_INT16); u16((int) v); }
            else if (v >= Integer.MIN_VALUE && v <= Integer.MAX_VALUE) { u8(T_INT32); u32(v); }
            else { u8(T_INT64); u64(v); }
        }

        public void u16(int v) {
            buf.write(v & 0xFF);
            buf.write((v >> 8) & 0xFF);
        }

        public void f64(double v) { u64(Double.doubleToLongBits(v)); }
        public void f32(float v) { u32(Float.floatToIntBits(v) & 0xFFFFFFFFL); }

        public void bool(boolean v) { u8(v ? 1 : 0); }

        public void strValue(String s) {
            u8(T_STRING);
            str(s);
        }

        public void boolValue(boolean v) {
            u8(T_BOOL);
            bool(v);
        }

        public void byteValue(int v) {
            u8(T_UINT8);
            u8(v);
        }

        public void f64Value(double v) {
            u8(T_FLOAT64);
            f64(v);
        }

        public void str(String s) {
            byte[] b = s.getBytes(StandardCharsets.UTF_8);
            if (b.length <= 0xFF) { u8(0x00); u8(b.length); }
            else if (b.length <= 0xFFFF) { u8(0x01); u16(b.length); }
            else { u8(0x02); u32(b.length); }
            buf.write(b, 0, b.length);
        }
    }

    public static final class Reader {
        private final ByteBuffer buf;

        public Reader(byte[] data) { buf = ByteBuffer.wrap(data); }

        public int u8() { return buf.get() & 0xFF; }
        public int u16() { return (buf.get() & 0xFF) | ((buf.get() & 0xFF) << 8); }
        public long u32() {
            long v = 0;
            for (int i = 0; i < 4; i++) v |= (buf.get() & 0xFFL) << (8 * i);
            return v;
        }
        public long u64() {
            long v = 0;
            for (int i = 0; i < 8; i++) v |= (buf.get() & 0xFFL) << (8 * i);
            return v;
        }
        public int i32() { return (int) u32(); }
        public long i64() { return u64(); }
        public int readI8() { return (byte) u8(); }
        public int readI16() { return (short) u16(); }
        public long readLongInt() {
            int tag = u8();
            return switch (tag) {
                case T_INT8 -> (byte) u8();
                case T_INT16 -> (short) u16();
                case T_INT32, T_UINT32 -> (long) i32();
                case T_INT64, T_UINT64 -> i64();
                case T_UINT8 -> u8();
                case T_UINT16 -> u16();
                default -> throw new RuntimeException("wire: expected an integer");
            };
        }
        public byte readByteNum() {
            if (u8() != T_UINT8) throw new RuntimeException("wire: expected a byte");
            return (byte) u8();
        }
        public boolean readBool() {
            if (u8() != T_BOOL) throw new RuntimeException("wire: expected a bool");
            return u8() != 0;
        }
        public boolean readBoolRaw() { return u8() != 0; }
        public double f32() { return Float.intBitsToFloat(i32()); }
        public double f64() { return Double.longBitsToDouble(u64()); }

        public String strValue() {
            if (u8() != T_STRING) throw new RuntimeException("wire: expected a string");
            return str();
        }

        public String str() {
            int lenSize = u8();
            int n = switch (lenSize) {
                case 0x00 -> u8();
                case 0x01 -> u16();
                case 0x02 -> (int) u32();
                default -> throw new RuntimeException("wire: bad string length size");
            };
            byte[] b = new byte[n];
            buf.get(b);
            return new String(b, StandardCharsets.UTF_8);
        }
    }

    public static void encodeValue(Writer w, S5RunVariable v) {
        switch (v) {
            case S5RunVariable.Null n -> w.u8(T_UNDEFINED);
            case S5RunVariable.Undefined u -> w.u8(T_UNDEFINED);
            case S5RunVariable.Bool b -> { w.u8(T_BOOL); w.bool(b.v()); }
            case S5RunVariable.Int i -> w.int64(i.v());
            case S5RunVariable.Dbl d -> { w.u8(T_FLOAT64); w.f64(d.v()); }
            case S5RunVariable.Str s -> w.strValue(s.v());
            case S5RunVariable.Arr a -> {
                w.u8(T_ARRAY);
                w.u32(a.v().size());
                for (S5RunVariable e : a.v()) encodeValue(w, e);
            }
            case S5RunVariable.Obj o -> {
                w.u8(T_OBJECT);
                w.u32(o.v().size());
                for (var en : o.v().entrySet()) {
                    w.strValue(en.getKey());
                    encodeValue(w, en.getValue());
                }
            }
            case S5RunVariable.Ref rf -> {
                w.u8(T_REF);
                w.i32(rf.thisId());
                w.str(rf.type());
            }
        }
    }

    public static S5RunVariable decodeValue(Reader r) {
        int tag = r.u8();
        return switch (tag) {
            case T_UNDEFINED -> new S5RunVariable.Undefined();
            case T_BOOL -> new S5RunVariable.Bool(r.readBoolRaw());
            case T_INT8 -> new S5RunVariable.Int(r.readI8());
            case T_INT16 -> new S5RunVariable.Int(r.readI16());
            case T_INT32, T_UINT32 -> new S5RunVariable.Int(r.i32());
            case T_INT64, T_UINT64 -> new S5RunVariable.Int(r.i64());
            case T_UINT8 -> new S5RunVariable.Int(r.u8());
            case T_UINT16 -> new S5RunVariable.Int(r.u16());
            case T_FLOAT32 -> new S5RunVariable.Dbl(r.f32());
            case T_FLOAT64 -> new S5RunVariable.Dbl(r.f64());
            case T_STRING -> new S5RunVariable.Str(r.str());
            case T_ARRAY -> {
                int n = (int) r.u32();
                List<S5RunVariable> a = new ArrayList<>(n);
                for (int i = 0; i < n; i++) a.add(decodeValue(r));
                yield new S5RunVariable.Arr(a);
            }
            case T_OBJECT -> {
                int n = (int) r.u32();
                Map<String, S5RunVariable> o = new LinkedHashMap<>();
                for (int i = 0; i < n; i++) o.put(r.strValue(), decodeValue(r));
                yield new S5RunVariable.Obj(o);
            }
            case T_REF -> new S5RunVariable.Ref(r.i32(), r.str());
            default -> throw new RuntimeException("wire: unknown value tag " + tag);
        };
    }

    public static String valueToString(S5RunVariable v) {
        return switch (v) {
            case S5RunVariable.Undefined u -> "undefined";
            case S5RunVariable.Null n -> "null";
            case S5RunVariable.Bool b -> Boolean.toString(b.v());
            case S5RunVariable.Int i -> Long.toString(i.v());
            case S5RunVariable.Dbl d -> Double.toString(d.v());
            case S5RunVariable.Str s -> s.v();
            default -> throw new RuntimeException("wire: expected a scalar argument");
        };
    }

    public static List<Byte> readListOfByte(Reader r) {
        if (r.u8() != T_ARRAY) throw new RuntimeException("wire: expected array");
        int n = (int) r.u32();
        List<Byte> out = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            out.add(r.readByteNum());
        }
        return out;
    }

    public static void writeListOfByte(Writer w, List<Byte> v) {
        w.u8(T_ARRAY);
        w.u32(v.size());
        for (byte e : v) {
            w.byteValue((int) e);
        }
    }
    public static List<String> readListOfStr(Reader r) {
        if (r.u8() != T_ARRAY) throw new RuntimeException("wire: expected array");
        int n = (int) r.u32();
        List<String> out = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            out.add(r.strValue());
        }
        return out;
    }

    public static void writeListOfStr(Writer w, List<String> v) {
        w.u8(T_ARRAY);
        w.u32(v.size());
        for (String e : v) {
            w.strValue(e);
        }
    }
    private S5RunWire() {}
}
