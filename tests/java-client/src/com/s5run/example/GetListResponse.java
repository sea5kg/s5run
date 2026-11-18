// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetListResponse {
    public List<String> value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU32(b, this.value.size());
        for (var _e : this.value) {
            Wire.putStr(b, _e);
        }
        return b.toByteArray();
    }
    public static GetListResponse decode(byte[] data) {
        GetListResponse r = new GetListResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        {
            long _n = Wire.getU32(b);
            r.value = new ArrayList<>();
            for (long _i = 0; _i < _n; ++_i) {
                String _e;
                _e = Wire.getStr(b);
                r.value.add(_e);
            }
        }
        return r;
    }
}