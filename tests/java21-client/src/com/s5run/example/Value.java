// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public sealed interface Value {
    record Null() implements Value {}
    record Bool(boolean v) implements Value {}
    record Int(long v) implements Value {}
    record Dbl(double v) implements Value {}
    record Str(String v) implements Value {}
    record Arr(List<Value> v) implements Value {}
    record Obj(Map<String, Value> v) implements Value {}
    record Ref(int thisId, String type) implements Value {}
}
