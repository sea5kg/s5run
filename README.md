# s5run

[![s5run Version](https://img.shields.io/badge/s5run-v0.2.0-yellow.svg)](https://github.com/sea5kg/s5run) [![s5run ProgrammingLanguage](https://img.shields.io/badge/ProgrammingLanguage-c++-yellow.svg)](https://github.com/sea5kg/s5run) [![Docker Pulls](https://img.shields.io/docker/pulls/sea5kg/s5run.svg)](https://hub.docker.com/r/sea5kg/s5run/) [![Github Stars](https://img.shields.io/github/stars/sea5kg/s5run.svg?label=github%20%E2%98%85)](https://github.com/sea5kg/s5run/) [![Github Stars](https://img.shields.io/github/contributors/sea5kg/s5run.svg)](https://github.com/sea5kg/s5run/) [![Github Forks](https://img.shields.io/github/forks/sea5kg/s5run.svg?label=github%20forks)](https://github.com/sea5kg/s5run/)


Look's like: *Reversible RPC (bidirectional streaming/callback bridge) over a single connection.*


Remote script execution on another server (JavaScript). Using a special context sharing protocol.

Supports protocols:

- s5run (socket, binary stream)
- Maybe in future: websockets
- Maybe in future: http (rest)

Not implemented yet:

- `add_context_provider`


## Generation client code library (context-provider)

Hardcoded file `s5run-client.json`

Supported:

- cpp98 - generate client library code for C++ 98 standard
- cpp11 - generate client library code for C++ 11 standard
- java11 - generate client library for java11
- java17 - generate client library for java17
- java21 - generate client library for java21
- py - generate client library for python

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
  "client-version": "1.0.0",
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
          "arguments": {
            "convert-from": "...",
            "type": "array<string>",
            "name": "args"
          }
        },
        {
          "name": "warning",
          "arguments": {
            "convert-from": "...",
            "type": "array<string>",
            "name": "args"
          }
        },
        {
          "name": "error",
          "arguments": {
            "convert-from": "...",
            "type": "array<string>",
            "name": "args"
          }
        }
      ]
    }
  ]
}
```

It's schema share classes, global functions and global objects.

For regenerate client libraries:

```sh
$ python3 -m pys5rungen all     # all configured languages
$ python3 -m pys5rungen cpp11   # only C++11
$ python3 -m pys5rungen java    # only Java
$ python3 -m pys5rungen py      # only Python
$ python3 -m pys5rungen init    # create a default example s5run-client.json
$ python3 -m pys5rungen help    # show help
```

### How add your implementation

Looks by example in `tests/cpp11-client/`

- Script generated base class and interfaces will be putted here: `tests/cpp11-client/s5run-cli`

```cpp
// TODO
```

How to run:

```
auto ctx = s5run_cli_example::make_context();
ctx->add_context_provider("s5run://localhost:7775", "some_api_key", {"Storage", "Cache"});
ctx->add_class(std::make_shared<MyFile>());
ctx->add_function(std::make_shared<MyGetFile>());
ctx->add_function(std::make_shared<MyHasFile>());
ctx->add_function(std::make_shared<MyGetTimeNow>());
ctx->add_object(std::make_shared<MyConsole>());
ctx->add_object(std::make_shared<MyFileSystem>());
std::shared_ptr<s5run_cli_example::remote_server_info> server0 = std::make_shared<s5run_cli_example::remote_server_info>();
server0->set_address("s5run://localhost:7777");
server0->set_api_key("some api key runner");
ctx->add_server(server0);
ctx->set_script(script);
auto result = s5run_cli_example::run_script(ctx, "main", {s5run_cli_example::value("test.txt")});

```

## Build

```sh
$ ./build_simple.sh
```

## Build image

```sh

# build env if need
$ docker build --rm=true -t "sea5kg/s5run:build-environment-2026-11-20" -f Dockerfile.build-environment .
$ docker build --rm=true -t "sea5kg/s5run:release-environment-2026-11-20" -f Dockerfile.release-environment .

# build image
$ docker build --rm=true -t "sea5kg/s5run:v0.1.0" .
```

Easy test run:

```sh
$ docker run --rm -p 7777:7777 sea5kg/s5run:v0.1.0
```

## Example run server

```sh
$ ./s5run --listen "0.0.0.0:7777" --threads 8
s5run listening on 0.0.0.0:7777, threads=8
client 127.0.0.1:53421 connected
```

## Possible integration models

**`BackendModel`**

As one of the services behind the backend without an exposed port

```
[ ............ ]
[ s5run-server ] <--> [ s5run-cli, backend ] <--> [ client ]
[ ............ ]
```

**`FrontendModel`**

s5run publishes its port, and the client connects to it to execute its script.
However, the context of the execution objects (Database, Storage) is provided by your Backends.

```
[ ............ ] <--> [ s5run-cli, client ]
[ s5run-server ]
[ ............ ] <--> [ s5run-provider, backend ]
```

## Authors and credits

- **Evgenii Sopov (sea5kg)** — author, maintainer — [github.com/sea5kg](https://github.com/sea5kg)
- **zai (AI assistant, GLM)** — pair programming: binary protocol implementation, client code generator, smoke tests

### Third-party

- [QuickJS (quickjs-ng)](https://github.com/quickjs-ng/quickjs) — JavaScript engine
- [Lua](https://www.lua.org/) — scripting engine (integration in progress)
- [SQLITE](https://www.sqlite.org/download.html) (v3.53.1) - SQLite is in the Public Domain
- [HowardHinnant/date](https://github.com/HowardHinnant/date) (v3.0.4) - Parser for datetime
- [nlohmann/json](https://github.com/nlohmann/json) (v3.12.0) - Reader/Writer for json format
