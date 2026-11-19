// AUTO-GENERATED. DO NOT EDIT.
package com.s5run.example;

import java.util.*;

public interface IRunScriptGlobalFunctions {
    File GetFile(String filepath);
    long GetFileSize(File file);
    long GetTimeNow();
    boolean HasFile(String filepath);
    List<String> GetFileList(String filepath);
    List<File> GetOpenedFiles();
    void CloseFiles(List<File> files);
    double GetPi();
    long GetServerPort();
}