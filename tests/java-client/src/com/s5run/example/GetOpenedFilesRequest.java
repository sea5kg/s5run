// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetOpenedFilesRequest {

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        return b.toByteArray();
    }
    public static GetOpenedFilesRequest decode(byte[] data) {
        GetOpenedFilesRequest r = new GetOpenedFilesRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        return r;
    }
}