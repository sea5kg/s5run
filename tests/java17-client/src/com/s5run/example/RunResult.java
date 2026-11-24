// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

public record RunResult(
        boolean hasError,
        String error,
        int errorLine,
        long durationMs,
        S5RunVariable value) {

    public static RunResult ok(S5RunVariable value, long durationMs) {
        return new RunResult(false, "", 0, durationMs, value);
    }

    public static RunResult fail(String error, long durationMs) {
        String msg = error == null ? "unknown error" : error;
        return new RunResult(true, msg, parseErrorLine(msg), durationMs, new S5RunVariable.Null());
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
