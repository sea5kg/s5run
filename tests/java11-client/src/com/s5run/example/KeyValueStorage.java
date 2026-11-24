// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface KeyValueStorage extends S5RunObject {
    @Override
    default String s5runClassName() { return "KeyValueStorage"; }

    @Override
    default List<String> s5runMethodNames() { return List.of("SetValueByKey", "GetValueByKey"); }

    void SetValueByKey(String name, S5RunVariable value);
    void GetValueByKey(String name);
}
