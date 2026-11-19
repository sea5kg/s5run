// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetFileSizeRequest {
    public File file;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU64(b, this.file != null ? this.file.getThisId() : 0);
        return b.toByteArray();
    }
    public static GetFileSizeRequest decode(byte[] data) {
        GetFileSizeRequest r = new GetFileSizeRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.file = null; // ref in array: not supported yet
        Wire.getU64(b);
        return r;
    }
}