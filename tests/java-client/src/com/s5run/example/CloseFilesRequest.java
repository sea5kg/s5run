// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class CloseFilesRequest {
    public List<File> files;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU32(b, this.files.size());
        for (var _e : this.files) {
            Wire.putU64(b, _e != null ? _e.getThisId() : 0);
        }
        return b.toByteArray();
    }
    public static CloseFilesRequest decode(byte[] data) {
        CloseFilesRequest r = new CloseFilesRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        {
            long _n = Wire.getU32(b);
            r.files = new ArrayList<>();
            for (long _i = 0; _i < _n; ++_i) {
                File _e;
                _e = null; // ref in array: not supported yet
                Wire.getU64(b);
                r.files.add(_e);
            }
        }
        return r;
    }
}