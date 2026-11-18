// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetFileListRequest {
    public String filepath;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putStr(b, this.filepath);
        return b.toByteArray();
    }
    public static GetFileListRequest decode(byte[] data) {
        GetFileListRequest r = new GetFileListRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.filepath = Wire.getStr(b);
        return r;
    }
}