// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class logRequest {
    public long this_id;
    public List<String> args;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU64(b, this_id);
        Wire.putU32(b, this.args.size());
        for (var _e : this.args) {
            Wire.putStr(b, _e);
        }
        return b.toByteArray();
    }
    public static logRequest decode(byte[] data) {
        logRequest r = new logRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.this_id = Wire.getU64(b);
        {
            long _n = Wire.getU32(b);
            r.args = new ArrayList<>();
            for (long _i = 0; _i < _n; ++_i) {
                String _e;
                _e = Wire.getStr(b);
                r.args.add(_e);
            }
        }
        return r;
    }
}