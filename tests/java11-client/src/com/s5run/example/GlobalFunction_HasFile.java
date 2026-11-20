// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

public interface GlobalFunction_HasFile extends S5RunFunction {
    @Override
    default String s5runFunctionName() { return "HasFile"; }

    boolean execute(String filepath);
}
