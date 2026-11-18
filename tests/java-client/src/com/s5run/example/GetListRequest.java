// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetListRequest {
    public long this_id;
    public String filepath;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU64(b, this_id);
        Wire.putStr(b, this.filepath);
        return b.toByteArray();
    }
    public static GetListRequest decode(byte[] data) {
        GetListRequest r = new GetListRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.this_id = Wire.getU64(b);
        r.filepath = Wire.getStr(b);
        return r;
    }
}