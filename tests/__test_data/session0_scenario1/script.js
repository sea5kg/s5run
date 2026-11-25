function main(path) {
    var f = GetFile(path);
    console.log("second scenario", path);
    return f.GetSize();
}
