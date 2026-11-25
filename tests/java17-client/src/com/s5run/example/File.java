// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface File extends S5RunObject {
    @Override
    default String s5runClassName() { return "File"; }

    @Override
    default List<S5RunProperty> s5runPropertyDescriptors() { return List.of(new S5RunProperty("Name", "GetName", ""), new S5RunProperty("BufferSize", "GetBufferSize", "SetBufferSize")); }

    @Override
    default List<String> s5runMethodNames() { return List.of("GetSize", "ReadBytes", "GetName", "GetBufferSize", "SetBufferSize"); }

    long GetSize();
    List<Byte> ReadBytes(long readBytes);
    String GetName();
    long GetBufferSize();
    void SetBufferSize(long bufferSize);
}
