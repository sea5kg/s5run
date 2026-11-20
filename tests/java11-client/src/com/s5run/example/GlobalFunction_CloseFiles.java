// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface GlobalFunction_CloseFiles extends S5RunFunction {
    @Override
    default String s5runFunctionName() { return "CloseFiles"; }

    void execute(List<File> files);
}
