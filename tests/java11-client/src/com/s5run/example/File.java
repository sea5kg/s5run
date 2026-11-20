// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface File extends S5RunObject {
    @Override
    default String s5runClassName() { return "File"; }

    @Override
    default List<String> s5runMethodNames() { return List.of("GetSize", "ReadBytes"); }

    long GetSize();
    List<Byte> ReadBytes(long readBytes);
}
