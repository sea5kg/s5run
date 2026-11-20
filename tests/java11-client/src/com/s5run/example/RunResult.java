// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

public final class RunResult {
    private final boolean hasError;
    private final String error;
    private final int errorLine;
    private final long durationMs;
    private final Value value;

    public RunResult(boolean hasError, String error, int errorLine, long durationMs, Value value) {
        this.hasError = hasError;
        this.error = error;
        this.errorLine = errorLine;
        this.durationMs = durationMs;
        this.value = value;
    }

    public boolean hasError() { return hasError; }
    public String error() { return error; }
    public int errorLine() { return errorLine; }
    public long durationMs() { return durationMs; }
    public Value value() { return value; }

    public static RunResult ok(Value value, long durationMs) {
        return new RunResult(false, "", 0, durationMs, value);
    }

    public static RunResult fail(String error, long durationMs) {
        String msg = error == null ? "unknown error" : error;
        return new RunResult(true, msg, parseErrorLine(msg), durationMs, Value.Null.INSTANCE);
    }

    static int parseErrorLine(String error) {
        if (error == null) return 0;
        int pos = 0;
        while (pos < error.length()) {
            int colon = error.indexOf(':', pos);
            if (colon < 0) return 0;
            int p1 = colon + 1;
            if (p1 >= error.length() || !Character.isDigit(error.charAt(p1))) {
                pos = p1;
                continue;
            }
            int p2 = p1;
            while (p2 < error.length() && Character.isDigit(error.charAt(p2))) p2++;
            if (p2 + 1 < error.length() && error.charAt(p2) == ':' && Character.isDigit(error.charAt(p2 + 1))) {
                return Integer.parseInt(error.substring(p1, p2));
            }
            pos = p2;
        }
        return 0;
    }
}
