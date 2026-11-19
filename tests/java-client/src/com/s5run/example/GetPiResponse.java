// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetPiResponse {
    public double value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putF64(b, this.value);
        return b.toByteArray();
    }
    public static GetPiResponse decode(byte[] data) {
        GetPiResponse r = new GetPiResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.value = Wire.getF64(b);
        return r;
    }
}