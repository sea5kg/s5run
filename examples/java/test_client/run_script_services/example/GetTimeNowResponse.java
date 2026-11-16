// AUTO-GENERATED. DO NOT EDIT.
package example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetTimeNowResponse {
    public long value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU64(b, this.value);
        return b.toByteArray();
    }
    public static GetTimeNowResponse decode(byte[] data) {
        GetTimeNowResponse r = new GetTimeNowResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        r.value = Wire.getU64(b);
        return r;
    }
}