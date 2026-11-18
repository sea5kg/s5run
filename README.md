# s5run

Concept:

Remote script execution on another server (JavaScript/Lua) in an isolated environment. Using a special context sharing protocol.

For now, it's implemented via sockets, but I think it could also be done via websocket for the web version of the protocol.

Regarding context, I'm thinking of using something like add_remote_context('Storage', "ip:port") for the server.


## s5run-client.json

For example json:

```json
{
  "options": {
    "cpp11": {
      "output-dir": "tests/cpp11-client/s5run_cli_example/",
      "namespace": "s5run_cli_example"
    },
    "java": {
      "output-dir": "tests/java-client/src/",
      "package": "com.s5run.example"
    },
    "py": {
      "output-dir": "tests/py-client/",
      "module": "libs5run_cli_example"
    }
  },
  "version": 1,
  "functions": [
    {
      "name": "GetFile",
      "return": "File",
      "arguments": [
        {
          "type": "string",
          "name": "filepath"
        }
      ]
    },
    {
      "name": "GetTimeNow",
      "return": "int",
      "arguments": []
    },
    {
      "name": "HasFile",
      "return": "bool",
      "arguments": [
        {
          "type": "string",
          "name": "filepath"
        }
      ]
    },
    {
      "name": "GetFileList",
      "return": "array<string>",
      "arguments": [
        {
          "type": "string",
          "name": "filepath"
        }
      ]
    }
  ],
  "classes": [
    {
      "name": "File",
      "methods": [
        {
          "name": "GetSize",
          "return": "int",
          "arguments": []
        },
        {
          "name": "ReadBytes",
          "return": "array<byte>",
          "arguments": [
            {
              "type": "int",
              "name": "readBytes"
            }
          ]
        }
      ]
    },
    {
      "name": "FileSystem",
      "methods": [
        {
          "name": "GetList",
          "return": "array<string>",
          "arguments": [
            {
              "type": "string",
              "name": "filepath"
            }
          ]
        }
      ]
    },
    {
      "name": "console",
      "methods": [
        {
          "name": "log",
          "arguments": [
            {
              "type": "array<string>",
              "name": "args"
            }
          ]
        },
        {
          "name": "warning",
          "arguments": [
            {
              "type": "array<string>",
              "name": "args"
            }
          ]
        },
        {
          "name": "error",
          "arguments": [
            {
              "type": "array<string>",
              "name": "args"
            }
          ]
        }
      ]
    }
  ]
}
```

It's schema share classes, global functions and global objects.

For regenerate client libraries:

```sh
$ python3 -u generate_client_code.py
```

### How add your implementation

Looks by example in `tests/cpp11-client/`

- Script generated base class and interfaces will be putted here: `tests/cpp11-client/s5run-cli`

```cpp
// TODO
```

How to run:

```
auto cli = s5run_cli_example::make_client();
cli->registry_class<MyFile>();
cli->registry_global_function<MyGetFile>();
cli->registry_global_function<MyHasFile>();
cli->registry_global_function<MyGetTimeNow>();
cli->registry_global_object(new MyConsole());
cli->registry_global_object(new MyFileSystem());
cli->set_server_address("s5run://localhost:7777");
auto result = cli->run(script, "main", {s5run_cli_example::value("test.txt")});

```

## Build

```sh
$ ./build_simple.sh
```

## Example run

```sh
$ ./s5run-server 7777 8
script_service listening on :7777, threads=8
client 127.0.0.1:53421 connected
```
