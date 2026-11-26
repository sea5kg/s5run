// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public sealed interface S5RunVariable {
    record Null() implements S5RunVariable {}
    record Undefined() implements S5RunVariable {}
    record Bool(boolean v) implements S5RunVariable {}
    record Int(long v) implements S5RunVariable {}
    record Dbl(double v) implements S5RunVariable {}
    record Str(String v) implements S5RunVariable {}
    record Arr(List<S5RunVariable> v) implements S5RunVariable {}
    record Obj(Map<String, S5RunVariable> v) implements S5RunVariable {}
    record Ref(int thisId, String type) implements S5RunVariable {}

    /** Structured multi-line representation, handy for tests and debugging, e.g.
     * K_INT 1024 / K_OBJECT BEGIN..END blocks. Object fields are sorted by name. */
    default String toStringS5Run() {
        StringBuilder sb = new StringBuilder();
        appendS5Run(sb, 0);
        String out = sb.toString();
        if (out.endsWith("\n")) {
            out = out.substring(0, out.length() - 1);
        }
        return out;
    }

    /** JSON representation (hand-rolled, no external libraries). */
    default String toStringJson() {
        StringBuilder sb = new StringBuilder();
        appendJson(sb);
        return sb.toString();
    }

    private void appendS5Run(StringBuilder sb, int indent) {
        String pad = " ".repeat(indent);
        String padIn = " ".repeat(indent + 2);
        if (this instanceof Undefined) {
            sb.append("K_UNDEFINED\n");
        } else if (this instanceof Null) {
            sb.append("K_NULL\n");
        } else if (this instanceof Bool) {
            sb.append(((Bool) this).v() ? "K_BOOL true\n" : "K_BOOL false\n");
        } else if (this instanceof Int) {
            sb.append("K_INT ").append(((Int) this).v()).append("\n");
        } else if (this instanceof Dbl) {
            sb.append("K_DOUBLE ").append(((Dbl) this).v()).append("\n");
        } else if (this instanceof Str) {
            sb.append("K_STRING ");
            appendJsonString(sb, ((Str) this).v());
            sb.append("\n");
        } else if (this instanceof Arr) {
            sb.append("K_ARRAY\n");
            sb.append(pad).append("BEGIN\n");
            for (S5RunVariable e : ((Arr) this).v()) {
                sb.append(padIn);
                e.appendS5Run(sb, indent + 2);
            }
            sb.append(pad).append("END\n");
        } else if (this instanceof Obj) {
            // TreeMap/LinkedHashMap iteration; std maps in tests are sorted
            sb.append("K_OBJECT\n");
            sb.append(pad).append("BEGIN\n");
            for (Map.Entry<String, S5RunVariable> en : ((Obj) this).v().entrySet()) {
                sb.append(padIn).append(en.getKey()).append(" ");
                en.getValue().appendS5Run(sb, indent + 2);
            }
            sb.append(pad).append("END\n");
        } else if (this instanceof Ref) {
            Ref rf = (Ref) this;
            sb.append("K_REF ").append(rf.thisId()).append(" ");
            appendJsonString(sb, rf.type());
            sb.append("\n");
        }
    }

    private static void appendJsonString(StringBuilder sb, String s) {
        sb.append('"');
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '"': sb.append("\\\""); break;
                case '\\': sb.append("\\\\"); break;
                case '\n': sb.append("\\n"); break;
                case '\r': sb.append("\\r"); break;
                case '\t': sb.append("\\t"); break;
                default:
                    if (c < 0x20) {
                        sb.append(String.format("\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
            }
        }
        sb.append('"');
    }

    private void appendJson(StringBuilder sb) {
        if (this instanceof Null) {
            sb.append("null");
        } else if (this instanceof Undefined) {
            sb.append("null");
        } else if (this instanceof Bool) {
            sb.append(((Bool) this).v());
        } else if (this instanceof Int) {
            sb.append(((Int) this).v());
        } else if (this instanceof Dbl) {
            sb.append(((Dbl) this).v());
        } else if (this instanceof Str) {
            appendJsonString(sb, ((Str) this).v());
        } else if (this instanceof Arr) {
            sb.append("[");
            boolean first = true;
            for (S5RunVariable e : ((Arr) this).v()) {
                if (!first) {
                    sb.append(", ");
                }
                first = false;
                e.appendJson(sb);
            }
            sb.append("]");
        } else if (this instanceof Obj) {
            sb.append("{");
            boolean first = true;
            for (Map.Entry<String, S5RunVariable> en : ((Obj) this).v().entrySet()) {
                if (!first) {
                    sb.append(", ");
                }
                first = false;
                appendJsonString(sb, en.getKey());
                sb.append(": ");
                en.getValue().appendJson(sb);
            }
            sb.append("}");
        } else if (this instanceof Ref) {
            Ref rf = (Ref) this;
            sb.append("{\"__s5run_ref__\":{\"this_id\":").append(rf.thisId()).append(",\"type\":");
            appendJsonString(sb, rf.type());
            sb.append("}}");
        }
    }
}
