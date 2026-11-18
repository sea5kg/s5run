// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetSizeRequest {
    public long this_id;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU64(b, this_id);
        return b.toByteArray();
    }
    public static GetSizeRequest decode(byte[] data) {
        GetSizeRequest r = new GetSizeRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.this_id = Wire.getU64(b);
        return r;
    }
}