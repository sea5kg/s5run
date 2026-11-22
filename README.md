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




## Command line

```sh
$ ./s5run help

           ░░░░░░░
          ░██▓▓▓▓▒
 ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
 ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
 ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░

s5run v0.2.0 - remote script execution server

Usage:
  ./s5run <command> [options]

Commands:
  server     Start the script server
  generate   Generate client or provider code
  run        Run a script against a server
  version    Print version
  help       Show this help

Run './s5run help <command>' for command details.
```

### s5run server

```sh
$ ./s5run help server
s5run server - start the script server

Usage:
  ./s5run server --listen [host:]port [--threads N] [--queue] [--db PATH] [--ttl SECONDS]

Options:
  --listen [host:]port  listen address (default 0.0.0.0, e.g. --listen 7777 or --listen 0.0.0.0:7777)
  --threads N           worker threads (default: hardware concurrency)
  --queue               enable job queue (results stored in sqlite)
  --db PATH             job queue database (default s5run-queue.sqlite3)
  --ttl SECONDS         how long to keep job results (default 3600)
```

Usage examples:
- `./s5run server --listen 0.0.0.0:7777` - Start the script server on 7777 port
- `./s5run server --listen 0.0.0.0:7777 --threads 8` - Start the script server on 7777 port and 8 threads
- `./s5run server --listen 7777 --queue --db /var/lib/s5run/queue.sqlite3 --ttl 7200` - Start the server with a persistent job queue

### s5run generate

```sh
$ ./s5run help generate
s5run generate - generate client or provider code

Usage:
  ./s5run generate client   --language LANG --output-dir DIR [--schema FILE]
  ./s5run generate provider --language LANG --output-dir DIR [--schema FILE]
  ./s5run generate --list

Options:
  --language LANG  target language (cpp98, cpp11)
  --output-dir DIR where to write generated files (default: options.<lang>.output-dir from schema)
  --schema FILE    schema file (default s5run-schema.json)
  --list           list available generators
```

Usage examples:
- `./s5run generate --list` - Show supported languages for generate client or provider code
- `./s5run generate client --language cpp98 --output-dir ./tests/cpp98-client/s5run_cli` - Generate client code based on cpp98
- `./s5run generate client --language cpp98 --schema s5run-schema.json` - Same, but output-dir is taken from `options.cpp98.output-dir` in the schema
- `./s5run generate provider --language cpp98 --output-dir ./tests/cpp98-client/s5run_cli` - Generate provider code based on cpp98 (*not implemented yet*)

### s5run run

```sh
$ ./s5run help run
s5run run - run a script against a server

Usage:
  ./s5run run <host:port> [--script FILE] [--provider SPEC] [--api-key KEY] [args...]
  cat script.js | ./s5run run <host:port> [args...]

Options:
  --script FILE     script file to execute (default: stdin)
  --provider SPEC   remote context provider: "host:port,name1,name2"
  --api-key KEY     api key for authentication (stub)
  args...           arguments passed to the script entry point
```

Usage examples:
- `./s5run run localhost:7777 --script test_script.js 21 21` - Run script file
- `cat script.js | ./s5run run localhost:7777 [args...]` - Run script from stdin
- The built-in `console` object (log/warning/error) is registered automatically and served client-side



## Generation client code library (context-provider)

Hardcoded file `s5run-schema.json`

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
      "namespace": "s5run_cli_example",
      "prefix-filename": "MyLib_"
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

Optional `prefix-filename` (cpp98/cpp11) prepends a prefix to the generated
file names, e.g. `MyLib_` -> `MyLib_s5run_cli_cpp98.h`.

For regenerate client libraries:

```sh
$ python3 -m pys5rungen all     # all configured languages
$ python3 -m pys5rungen java    # only Java
$ python3 -m pys5rungen py      # only Python
$ python3 -m pys5rungen init    # create a default example s5run-schema.json
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

### Rebuild docker images for environments

Just generate Dockerfile's:

```sh
$ ./pm.py rebuild-environment-images --build
```

Build image:
```sh
$ docker build --rm=true -t "sea5kg/s5run:v0.1.0" .
```

Easy test run:

```sh
$ docker run --rm -p 7777:7777 sea5kg/s5run:v0.1.0
```

## Example run server

```sh
$ ./s5run server --listen "0.0.0.0:7777" --threads 8
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
