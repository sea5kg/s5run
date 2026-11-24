// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public sealed interface S5RunVariable {
    record Null() implements S5RunVariable {}
    record Bool(boolean v) implements S5RunVariable {}
    record Int(long v) implements S5RunVariable {}
    record Dbl(double v) implements S5RunVariable {}
    record Str(String v) implements S5RunVariable {}
    record Arr(List<S5RunVariable> v) implements S5RunVariable {}
    record Obj(Map<String, S5RunVariable> v) implements S5RunVariable {}
    record Ref(int thisId, String type) implements S5RunVariable {}
}
