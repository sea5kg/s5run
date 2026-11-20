// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface Value {
    final class Null implements Value {
        public static final Null INSTANCE = new Null();
        private Null() {}
    }
    final class Bool implements Value {
        final boolean val;
        public Bool(boolean v) { this.val = v; }
        public boolean v() { return val; }
    }
    final class Int implements Value {
        final long val;
        public Int(long v) { this.val = v; }
        public long v() { return val; }
    }
    final class Dbl implements Value {
        final double val;
        public Dbl(double v) { this.val = v; }
        public double v() { return val; }
    }
    final class Str implements Value {
        final String val;
        public Str(String v) { this.val = v; }
        public String v() { return val; }
    }
    final class Arr implements Value {
        final List<Value> val;
        public Arr(List<Value> v) { this.val = v; }
        public List<Value> v() { return val; }
    }
    final class Obj implements Value {
        final Map<String, Value> val;
        public Obj(Map<String, Value> v) { this.val = v; }
        public Map<String, Value> v() { return val; }
    }
    final class Ref implements Value {
        final int thisId;
        final String type;
        public Ref(int thisId, String type) { this.thisId = thisId; this.type = type; }
        public int thisId() { return thisId; }
        public String type() { return type; }
    }
}
