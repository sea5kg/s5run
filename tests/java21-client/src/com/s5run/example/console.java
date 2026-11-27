// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface console extends S5RunObject {
    @Override
    default String s5runClassName() { return "console"; }

    @Override
    default List<S5RunProperty> s5runPropertyDescriptors() { return List.of(); }

    /** 'new console()' in scripts; override to enable the JS constructor. */
    default console s5runConstruct() { return null; }

    @Override
    default List<String> s5runMethodNames() { return List.of("log", "warning", "error"); }

    void log(List<String> args);
    void warning(List<String> args);
    void error(List<String> args);
}
