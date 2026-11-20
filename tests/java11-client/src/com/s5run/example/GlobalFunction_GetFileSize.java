// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

public interface GlobalFunction_GetFileSize extends S5RunFunction {
    @Override
    default String s5runFunctionName() { return "GetFileSize"; }

    long execute(File file);
}
