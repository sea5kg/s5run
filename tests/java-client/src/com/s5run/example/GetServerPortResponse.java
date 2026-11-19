// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetServerPortResponse {
    public long value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU64(b, this.value);
        return b.toByteArray();
    }
    public static GetServerPortResponse decode(byte[] data) {
        GetServerPortResponse r = new GetServerPortResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.value = Wire.getU64(b);
        return r;
    }
}