// AUTO-GENERATED. DO NOT EDIT.
package example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetFileRequest {
    public String filepath;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putStr(b, this.filepath);
        return b.toByteArray();
    }
    public static GetFileRequest decode(byte[] data) {
        GetFileRequest r = new GetFileRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.filepath = Wire.getStr(b);
        return r;
    }
}