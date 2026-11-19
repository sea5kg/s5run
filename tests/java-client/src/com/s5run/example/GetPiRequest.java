// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;
import java.io.*;
import java.nio.*;

public class GetPiRequest {

    public byte[] encode() {
        ByteArrayOutputStream b = new ByteArrayOutputStream();
        return b.toByteArray();
    }
    public static GetPiRequest decode(byte[] data) {
        GetPiRequest r = new GetPiRequest();
        ByteBuffer b = ByteBuffer.wrap(data);
        return r;
    }
}