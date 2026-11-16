# remote-run-scripts


```
$ ./sea5kg-run-scripts-server 7777 8
script_service listening on :7777, threads=8
client 127.0.0.1:53421 connected

$ ./test-client
add -> {"ok":true,"value":5}
hello -> {"ok":true,"value":{"len":5,"msg":"hello, world"}}
fact -> {"ok":true,"value":3628800}
```