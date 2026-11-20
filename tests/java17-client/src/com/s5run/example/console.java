// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface console extends S5RunObject {
    @Override
    default String s5runClassName() { return "console"; }

    @Override
    default List<String> s5runMethodNames() { return List.of("log", "warning", "error"); }

    void log(List<String> args);
    void warning(List<String> args);
    void error(List<String> args);
}
