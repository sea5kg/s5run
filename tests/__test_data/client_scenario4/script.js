function main(key_name, key_value) {
    console.log(key_name + "=" + key_value);
    let key_storage = new KeyValueStorage();
    key_storage.SetValueByKey("test", 1);
    console.log("1", key_storage.GetValueByKey("test"));
    return { "key_name": key_name, "key_value": key_value};
}
