// AUTO-GENERATED. DO NOT EDIT.
package example;

import java.io.*;
import java.nio.*;
import java.nio.charset.StandardCharsets;

public final class Wire {
    private Wire() {}
    public static void putU8(ByteArrayOutputStream b, byte v) { b.write(v); }
    public static void putBool(ByteArrayOutputStream b, boolean v) { b.write(v ? 1 : 0); }
    public static void putU32(ByteArrayOutputStream b, long v) {
        for (int i = 0; i < 4; ++i) b.write((int)((v >> (8*i)) & 0xFF));
    }
    public static void putU64(ByteArrayOutputStream b, long v) {
        for (int i = 0; i < 8; ++i) b.write((int)((v >> (8*i)) & 0xFF));
    }
    public static void putF64(ByteArrayOutputStream b, double v) {
        putU64(b, Double.doubleToRawLongBits(v));
    }
    public static void putStr(ByteArrayOutputStream b, String s) {
        byte[] raw = s.getBytes(StandardCharsets.UTF_8);
        putU32(b, raw.length); b.writeBytes(raw);
    }
    public static byte    getU8(ByteBuffer b)   { return b.get(); }
    public static boolean getBool(ByteBuffer b) { return b.get() != 0; }
    public static long    getU32(ByteBuffer b) {
        long v = 0; for (int i = 0; i < 4; ++i) v |= (long)(b.get() & 0xFF) << (8*i);
        return v;
    }
    public static long    getU64(ByteBuffer b) {
        long v = 0; for (int i = 0; i < 8; ++i) v |= (long)(b.get() & 0xFF) << (8*i);
        return v;
    }
    public static double  getF64(ByteBuffer b) {
        return Double.longBitsToDouble(getU64(b));
    }
    public static String  getStr(ByteBuffer b) {
        long n = getU32(b); byte[] raw = new byte[(int)n]; b.get(raw);
        return new String(raw, StandardCharsets.UTF_8);
    }
}