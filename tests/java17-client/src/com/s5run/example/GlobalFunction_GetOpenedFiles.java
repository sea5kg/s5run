// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface GlobalFunction_GetOpenedFiles extends S5RunFunction {
    @Override
    default String s5runFunctionName() { return "GetOpenedFiles"; }

    List<File> execute();
}
