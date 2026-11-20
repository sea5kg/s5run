// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

public interface GlobalFunction_GetFile extends S5RunFunction {
    @Override
    default String s5runFunctionName() { return "GetFile"; }

    File execute(String filepath);
}
