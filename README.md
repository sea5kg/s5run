# sea5kg-run-scripts-server


Concept:

Remote script execution on another server (JavaScript/Lua) in an isolated environment. Using a special context sharing protocol.

For now, it's implemented via sockets, but I think it could also be done via websocket for the web version of the protocol.

Regarding context, I'm thinking of using something like add_remote_context('Storage', "ip:port") for the server.


## sea5kg-run-scripts-client.json

It's schema share classes and functions.

For regenerate client libraries:

```
python3 -u gen_clients_code.py
```

## Build

```
$ ./build_simple.sh
```

## Example run

```
$ ./sea5kg-run-scripts-server 7777 8
script_service listening on :7777, threads=8
client 127.0.0.1:53421 connected

$ ./test-client
add -> {"ok":true,"value":5}
hello -> {"ok":true,"value":{"len":5,"msg":"hello, world"}}
fact -> {"ok":true,"value":3628800}
```