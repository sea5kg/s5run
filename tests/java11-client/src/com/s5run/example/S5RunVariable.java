// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface S5RunVariable {
    final class Null implements S5RunVariable {
        public static final Null INSTANCE = new Null();
        private Null() {}
    }
    final class Bool implements S5RunVariable {
        final boolean val;
        public Bool(boolean v) { this.val = v; }
        public boolean v() { return val; }
    }
    final class Int implements S5RunVariable {
        final long val;
        public Int(long v) { this.val = v; }
        public long v() { return val; }
    }
    final class Dbl implements S5RunVariable {
        final double val;
        public Dbl(double v) { this.val = v; }
        public double v() { return val; }
    }
    final class Str implements S5RunVariable {
        final String val;
        public Str(String v) { this.val = v; }
        public String v() { return val; }
    }
    final class Arr implements S5RunVariable {
        final List<S5RunVariable> val;
        public Arr(List<S5RunVariable> v) { this.val = v; }
        public List<S5RunVariable> v() { return val; }
    }
    final class Obj implements S5RunVariable {
        final Map<String, S5RunVariable> val;
        public Obj(Map<String, S5RunVariable> v) { this.val = v; }
        public Map<String, S5RunVariable> v() { return val; }
    }
    final class Ref implements S5RunVariable {
        final int thisId;
        final String type;
        public Ref(int thisId, String type) { this.thisId = thisId; this.type = type; }
        public int thisId() { return thisId; }
        public String type() { return type; }
    }
}
