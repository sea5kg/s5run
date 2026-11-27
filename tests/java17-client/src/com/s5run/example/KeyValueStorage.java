// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface KeyValueStorage extends S5RunObject {
    @Override
    default String s5runClassName() { return "KeyValueStorage"; }

    @Override
    default List<S5RunProperty> s5runPropertyDescriptors() { return List.of(); }

    /** 'new KeyValueStorage()' in scripts; override to enable the JS constructor. */
    default KeyValueStorage s5runConstruct() { return null; }

    @Override
    default List<String> s5runMethodNames() { return List.of("SetValueByKey", "GetValueByKey"); }

    void SetValueByKey(String name, S5RunVariable value);
    S5RunVariable GetValueByKey(String name);
}
