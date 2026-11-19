// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetServerPortRequest {

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        return b.toByteArray();
    }
    public static GetServerPortRequest decode(byte[] data) {
        GetServerPortRequest r = new GetServerPortRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        return r;
    }
}