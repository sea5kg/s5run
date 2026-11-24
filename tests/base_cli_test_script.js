function main(path) {
    var f = GetFile(path);
    var t = GetTimeNow();
    var b = f.ReadBytes(3);
    console.log("hello", path, 1, 2);
    var sizeByRef = GetFileSize(f);
    var opened = GetOpenedFiles();
    var openedSum = 0;
    for (var i = 0; i < opened.length; ++i)
        openedSum += opened[i].GetSize();
    CloseFiles(opened);
    var fileList = FileSystem.GetList(path);
    return {
        size: f.GetSize(),
        sizeByRef: sizeByRef,
        openedCount: opened.length,
        openedSum: openedSum,
        fileListCount: fileList.length,
        time: t,
        bytes: b.length
    };
}
