// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class ReadBytesResponse {
    public List<Byte> value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU32(b, this.value.size());
        for (var _e : this.value) {
            Wire.putU8(b, _e);
        }
        return b.toByteArray();
    }
    public static ReadBytesResponse decode(byte[] data) {
        ReadBytesResponse r = new ReadBytesResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        {
            long _n = Wire.getU32(b);
            r.value = new ArrayList<>();
            for (long _i = 0; _i < _n; ++_i) {
                byte _e;
                _e = Wire.getU8(b);
                r.value.add(_e);
            }
        }
        return r;
    }
}