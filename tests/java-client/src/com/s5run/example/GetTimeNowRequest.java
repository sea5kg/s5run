// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetTimeNowRequest {

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        return b.toByteArray();
    }
    public static GetTimeNowRequest decode(byte[] data) {
        GetTimeNowRequest r = new GetTimeNowRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        return r;
    }
}