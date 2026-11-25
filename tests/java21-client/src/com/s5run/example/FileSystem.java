// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface FileSystem extends S5RunObject {
    @Override
    default String s5runClassName() { return "FileSystem"; }

    @Override
    default List<S5RunProperty> s5runPropertyDescriptors() { return List.of(); }

    @Override
    default List<String> s5runMethodNames() { return List.of("GetList"); }

    List<String> GetList(String filepath);
}
