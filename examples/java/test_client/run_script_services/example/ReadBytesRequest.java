// AUTO-GENERATED. DO NOT EDIT.
package example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class ReadBytesRequest {
    public long this_id;
    public long readBytes;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU64(b, this_id);
        Wire.putU64(b, this.readBytes);
        return b.toByteArray();
    }
    public static ReadBytesRequest decode(byte[] data) {
        ReadBytesRequest r = new ReadBytesRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.this_id = Wire.getU64(b);
        r.readBytes = Wire.getU64(b);
        return r;
    }
}