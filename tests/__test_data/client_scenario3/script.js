function main(key_name, key_value) {
    console.log(key_name + "=" + key_value);
    var t = "1";
    t += 1;
    console.log("t=" + t);
    key_value++;
    console.log("new value=", key_value);
    return { "key_name": key_name, "key_value": key_value};
}
