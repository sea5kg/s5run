// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class HasFileRequest {
    public String filepath;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putStr(b, this.filepath);
        return b.toByteArray();
    }
    public static HasFileRequest decode(byte[] data) {
        HasFileRequest r = new HasFileRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.filepath = Wire.getStr(b);
        return r;
    }
}