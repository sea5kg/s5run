// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

public interface GlobalFunction_GetTimeNow extends S5RunFunction {
    @Override
    default String s5runFunctionName() { return "GetTimeNow"; }

    long execute();
}
