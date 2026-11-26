# s5run schema (`s5run-schema.json`)

The schema describes the shared script context: global functions, classes
(with methods and properties) and global objects. From this single file the
`s5run` binary generates client libraries for every supported language:

```sh
./s5run generate client --language cpp98   # cpp11, go, py, java11, java17, java21
./s5run generate provider --language cpp98 # provider-side library (C++ 98)
./s5run generate --list                    # available generators
```

## Top-level fields

| Field           | Type   | Required | Description                                       |
|-----------------|--------|----------|---------------------------------------------------|
| `client-version`| string | no       | Version of the generated library (`"1.0.0"`).     |
| `version`       | number | no       | Schema format version (currently `1`).            |
| `options`       | object | no       | Per-language generation options (see below).      |
| `functions`     | array  | no       | Global functions available to scripts.            |
| `classes`       | array  | no       | Classes (types) with methods and properties.      |

Unknown fields are rejected with a hint listing the allowed ones:

```
schema: unknown field 'module' in options.cpp98 (allowed: namespace, output-dir, prefix-filename)
```

## Options (per language)

| Section                       | Allowed fields                                |
|-------------------------------|-----------------------------------------------|
| `options.cpp98`, `options.cpp11` | `namespace`, `output-dir`, `prefix-filename` |
| `options.go`                  | `output-dir`, `package`                        |
| `options.py`                  | `output-dir`, `module`                         |
| `options.java`, `options.java11`, `options.java17`, `options.java21` | `output-dir`, `package` |

- `output-dir` — where generated files are written. Optional in the CLI:
  `--output-dir` overrides it; if neither is set, generation fails with a hint.
- `namespace` (C++) / `package` (Go, Java) / `module` (Python) — the name of
  the generated library namespace/package/module.
- `prefix-filename` (C++ only) — optional prefix prepended to the generated
  file names, e.g. `"MyLib_"` -> `MyLib_s5run_cli_cpp98.h` (also used in the
  include guard).

## Types

Type strings are used in `return`, argument `type` and property `type`.

| Type            | Meaning                                   | C++ 98/11      | Go         | Python   | Java         |
|-----------------|-------------------------------------------|----------------|------------|----------|--------------|
| `bool`          | boolean                                   | `bool`         | `bool`     | `bool`   | `boolean`    |
| `int`           | 64-bit signed integer                     | `i64`/`int64_t`| `int64`    | `int`    | `long`       |
| `uint`          | 64-bit unsigned integer                   | same as `int`  | `int64`    | `int`    | `long`       |
| `float`, `double` | double-precision floating point         | `double`       | `float64`  | `float`  | `double`     |
| `string`        | UTF-8 string                              | `std::string`  | `string`   | `str`    | `String`     |
| `byte`          | single byte                               | `u8`/`uint8_t` | `byte`     | `int`    | `byte`       |
| `var`           | dynamic value (any JS value: number, string, bool, array, object) | `s5run_variable` | `S5RunVariable` | `Any` | `S5RunVariable` |
| `void`          | no return value (**return types only**)  | `void`         | (omitted)  | (omitted)| `void`       |
| `array<T>`      | array of `T` (T: any type above or a class) | `std::vector<T>` | `[]T`   | `list[T]`| `List<T>`    |
| `ClassName`     | reference to a declared class             | class pointer/shared_ptr | class | class | interface |

Rules (validated after parsing, with detailed errors):

- a class reference must be declared in `classes` of the same schema;
- `void` is only allowed as a return type (not in arguments or array elements);
- class names must not shadow builtin types (`string`, `int`, `var`, ...);
- duplicate names are rejected: classes, functions, methods, properties.

## Functions

```json
{
  "name": "GetFile",
  "return": "File",
  "arguments": [
    { "type": "string", "name": "filepath" }
  ]
}
```

- `name` (required) — the function name as called from scripts.
- `return` (optional, default `"void"`) — the return type.
- `arguments` (optional array) — typed positional arguments; each item has
  `name` and `type` (both required).

## Classes

```json
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
        { "type": "int", "name": "readBytes" }
      ]
    }
  ],
  "properties": [
    {
      "name": "BufferSize",
      "type": "int",
      "getter": "GetBufferSize",
      "setter": "SetBufferSize"
    }
  ]
}
```

- `name` (required) — the class/type name.
- `methods` (optional array) — same format as functions. A method is callable
  from scripts as `obj.MethodName(...)`.
- `properties` (optional array) — JS accessor properties: readable/writable as
  `obj.Name` from scripts. Fields:

| Field   | Required | Description                                    |
|---------|----------|------------------------------------------------|
| `name`  | yes      | Property name as used in scripts (`obj.Name`).  |
| `type`  | yes      | Property type (any non-void type).              |
| `getter`| no       | Method name called on reads (read-only if omitted). |
| `setter`| no       | Method name called on writes (write-only if omitted). |

Property validation:

- `getter`/`setter` must reference **declared methods** of the same class:
  `class File property 'P': getter 'Nope' is not a declared method`;
- type consistency: the getter return type must equal the property type; the
  setter must take exactly one argument of the property type:
  `getter 'GetP' returns 'int', expected 'string' (property type)`;
- property names must be unique within the class.

## Convert-args methods (variadic console-style)

A single object instead of the `arguments` array declares a method/function
that accepts **any number of arguments**, converted to strings (like
`console.log(...)`):

```json
{
  "name": "log",
  "arguments": {
    "convert-from": "...",
    "type": "array<string>",
    "name": "args"
  }
}
```

Rules: `convert-from` must be `"..."`, `type` must be `array<string>`, and
all three fields are required. The generated client receives `args` as a
list of strings.

## Global objects

A class becomes a **global object** when an instance of it is registered
(`add_object` / `AddObject` / `add_object` depending on the language) — the
server installs it as a global script variable. Typical example: a `console`
class with `log`/`warning`/`error` methods registered as the `console` object.

## Full example

```json
{
  "client-version": "1.0.0",
  "version": 1,
  "options": {
    "cpp11": {
      "output-dir": "tests/cpp11-client/s5run_cli_example/",
      "namespace": "s5run_cli_example"
    },
    "py": {
      "output-dir": "tests/py-client/",
      "module": "libs5run_cli_example"
    },
    "go": {
      "output-dir": "tests/go-client/s5runcli/",
      "package": "s5runcli"
    },
    "java21": {
      "output-dir": "tests/java21-client/src/",
      "package": "com.s5run.example"
    }
  },
  "functions": [
    { "name": "GetFile", "return": "File",
      "arguments": [{ "type": "string", "name": "filepath" }] },
    { "name": "GetTimeNow", "return": "int", "arguments": [] },
    { "name": "Ping", "return": "void", "arguments": [] }
  ],
  "classes": [
    {
      "name": "File",
      "methods": [
        { "name": "GetSize", "return": "int", "arguments": [] },
        { "name": "ReadBytes", "return": "array<byte>",
          "arguments": [{ "type": "int", "name": "readBytes" }] }
      ],
      "properties": [
        { "name": "Name", "type": "string", "getter": "GetName" }
      ]
    },
    {
      "name": "console",
      "methods": [
        { "name": "log",
          "arguments": { "convert-from": "...", "type": "array<string>", "name": "args" } }
      ]
    }
  ]
}
```
