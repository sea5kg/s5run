// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class HasFileResponse {
    public boolean value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putBool(b, this.value);
        return b.toByteArray();
    }
    public static HasFileResponse decode(byte[] data) {
        HasFileResponse r = new HasFileResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.value = Wire.getBool(b);
        return r;
    }
}