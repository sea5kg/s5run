// AUTO-GENERATED. DO NOT EDIT.
package example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetFileListResponse {
    public List<String> value;

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        Wire.putU32(b, this.value.size());
        for (var _e : this.value) {
            Wire.putStr(b, _e);
        }
        return b.toByteArray();
    }
    public static GetFileListResponse decode(byte[] data) {
        GetFileListResponse r = new GetFileListResponse();
        ByteBuffer b = ByteBuffer.wrap(data);
        {
            long _n = Wire.getU32(b);
            r.value = new ArrayList<>();
            for (long _i = 0; _i < _n; ++_i) {
                String _e;
                _e = Wire.getStr(b);
                r.value.add(_e);
            }
        }
        return r;
    }
}