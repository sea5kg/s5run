// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetOpenedFilesResponse {
    public List<File> value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU32(b, this.value.size());
        for (var _e : this.value) {
            Wire.putU64(b, _e != null ? _e.getThisId() : 0);
        }
        return b.toByteArray();
    }
    public static GetOpenedFilesResponse decode(byte[] data) {
        GetOpenedFilesResponse r = new GetOpenedFilesResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        {
            long _n = Wire.getU32(b);
            r.value = new ArrayList<>();
            for (long _i = 0; _i < _n; ++_i) {
                File _e;
                _e = null; // ref in array: not supported yet
                Wire.getU64(b);
                r.value.add(_e);
            }
        }
        return r;
    }
}