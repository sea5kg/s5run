# s5run binary protocol

Protocol version: **3** (announced in the server hello).

s5run speaks a compact binary protocol over a single TCP connection.
One side registers a script context (functions, classes, objects), the
other side executes scripts; calls flow in both directions over the same
socket. There are three roles:

| Role                | Description                                                    |
|---------------------|----------------------------------------------------------------|
| **server**          | Runs scripts (QuickJS engine). Accepts client connections.    |
| **client**          | Registers context implementations, runs scripts.              |
| **provider**        | A backend server: publishes context metadata, serves API calls.|

The `s5run run --provider host:port,names...` client establishes a
connection with the script server and tells it where the provider
is located and what context it provides.

## Framing

Every message is one frame:

```
+------------------+---------------------+----------------------+
| u32 frame length | u8 message type     | body (length - 1 B)  |
| (big-endian)     | (see the table)     |                      |
+------------------+---------------------+----------------------+
```

- `frame length` counts **the whole frame** (type byte + body), i.e.
  `length = 1 + body.size()`;
- valid range: `1 < length <= 64 MiB`;
- all multi-byte integers **inside** the body are **little-endian** (only
  the frame length is big-endian);
- the body may be empty (`length == 1`).

## Hello

The server (and the provider) sends one frame immediately after the
connection is accepted, **before** any other message:

```
type: M_SERVER_HELLO (0x0F)
body: "s5run <server-version> <protocol-version>"   (plain text, UTF-8)
```

Example: `s5run v0.3.0 2`. A client that speaks another protocol version
must close the connection.

## Primitive encodings

### Integer (`u8/u16/u32/i8/i16/i32/i64`)

Fixed-width, little-endian, two's complement for the signed variants.

### String (`str`)

Length-prefixed, UTF-8, with a variable-width length prefix:

```
u8  size-tag   0x00: u8  length (1..255 bytes)
               0x01: u16 length (little-endian)
               0x02: u32 length (little-endian)
... raw bytes
```

The most compact prefix that fits is used.

## Message types

| Type | Code | Direction | Purpose |
|------|------|-----------|---------|
| `M_EXEC_REQ` | 0x01 | client -> server | run a script |
| `M_EXEC_RESP` | 0x02 | server -> client | script result |
| `M_CALLBACK_REQ` | 0x03 | server -> client (or client -> provider) | invoke a context method |
| `M_CALLBACK_RESP` | 0x04 | client -> server (or provider -> client) | method result |
| `M_REGISTER_FUNC` | 0x05 | client -> server | register a global function |
| `M_REGISTER_OBJ` | 0x06 | client -> server | register a global object |
| `M_REGISTER_TYPE` | 0x07 | client -> server | register a class (type) |
| `M_REGISTER_RESP` | 0x08 | server -> client | registration result |
| `M_STATS_REQ` | 0x09 | client -> server | server load stats request |
| `M_STATS_RESP` | 0x0A | server -> client | server load stats |
| `M_JOB_SUBMIT` | 0x0B | client -> server | queue a job |
| `M_JOB_SUBMIT_RESP` | 0x0C | server -> client | job uuid |
| `M_JOB_RESULT_REQ` | 0x0D | client -> server | fetch a job result |
| `M_JOB_RESULT_RESP` | 0x0E | server -> client | job result/status |
| `M_SERVER_HELLO` | 0x0F | server -> client | greeting (see above) |
| `M_PROVIDER_META_REQ` | 0x10 | client -> provider | context metadata request |
| `M_PROVIDER_META_RESP` | 0x11 | provider -> client | context metadata |
| `M_PROVIDER_MOUNT_REQ` | 0x12 | client -> server | mount provider entries on the server |
| `M_PROVIDER_MOUNT_RESP` | 0x13 | server -> client | mount result |

## Values

A value is a tag byte followed by type-specific data:

| Tag | Name | Payload |
|-----|------|---------|
| 0x00 | `T_UNDEFINED` | (none) — undefined / null (single wire kind) |
| 0x01 | `T_BOOL` | `u8` (0 = false, 1 = true) |
| 0x02 | `T_INT8` | `i8` |
| 0x03 | `T_INT16` | `i16` |
| 0x04 | `T_INT32` | `i32` |
| 0x05 | `T_INT64` | `i64` |
| 0x06 | `T_UINT8` | `u8` |
| 0x07 | `T_UINT16` | `u16` |
| 0x08 | `T_UINT32` | `u32` |
| 0x09 | `T_UINT64` | `u64` |
| 0x0A | `T_FLOAT32` | `f32` (IEEE 754, little-endian) |
| 0x0B | `T_FLOAT64` | `f64` (IEEE 754, little-endian) |
| 0x0C | `T_STRING` | `str` |
| 0x0D | `T_ARRAY` | `u32 count`, then `count` values |
| 0x0E | `T_OBJECT` | `u32 count`, then `count` x (`str` key, value) |
| 0x0F | `T_REF` | `i32 this_id`, `str` type name |

Notes:

- **Integers are encoded in the most compact signed form** (INT8 -> INT16
  -> INT32 -> INT64). Decoders accept any of the integer tags (including
  the unsigned variants) for an integer value.
- **Undefined and null share one tag** (`T_UNDEFINED`): a script `null`
  becomes `undefined` after a round-trip. The distinction exists only
  inside an engine before encoding.
- `T_REF` is a reference to a registered object instance; the type name
  must be a registered class (see `M_REGISTER_TYPE`). Refs may appear in
  arguments, results and (nested) inside arrays.
- `T_OBJECT` key order is not guaranteed; consumers treat it as a map.

## Registration messages

Registrations can be sent before or after the connection is established;
pending registrations are flushed when the script runs. Each registration
is answered with `M_REGISTER_RESP`.

### M_REGISTER_FUNC

```
str name
```

### M_REGISTER_TYPE

Registers a class: scripts may then hold references to its instances
(returned by callbacks) and call their methods.

```
str  name
u32  method_count
str  method_name [method_count]
u32  prop_count
{ str name; str getter; str setter } [prop_count]
```

Properties are JS accessor properties (`obj.Name` read/write); getter /
setter reference registered methods (empty string = read-only /
write-only).

### M_REGISTER_OBJ

Registers a concrete object instance as a global script variable.

```
str  name          (global variable name)
i32 this_id        (caller-assigned instance id)
str  type          (class name; must be registered via M_REGISTER_TYPE
                    when method results return refs of this type)
u32  method_count
str  method_name [method_count]
u32  prop_count
{ str name; str getter; str setter } [prop_count]
```

`this_id` values are unique per connection; the callee assigns them.

### M_REGISTER_RESP

```
u8  ok (1 = success)
str error (empty when ok)
```

## Script execution

### M_EXEC_REQ

```
str   script   (JavaScript source)
str   entry    (function name, usually "main")
u32   arg_count
value args [arg_count]
```

### M_EXEC_RESP

```
u8 ok
if ok:  value                     (the entry return value)
else:   str error                 (engine error with <script>:LINE:COL positions)
```

## Callbacks

While a script runs, the server calls back into the client (registered
context) synchronously. Callback requests and responses use the same
frame pair between a client and the server **and** between a bridge
client and a provider.

### M_CALLBACK_REQ

```
i32   callback_id       (echoed in the response)
str   method            (registered method or function name)
u32   arg_count
value args [arg_count]
```

For an **object method** call, `args[0]` is the integer `this_id` of the
target instance, followed by the method arguments. For a global function
call, `args` are the function arguments. For a convert-args method
(console-style) `args[0]` is still `this_id` and the rest are arbitrary
values the callee converts to strings.

### M_CALLBACK_RESP

```
i32  callback_id
u8   ok
if ok:  value               (method return value; T_UNDEFINED for void)
else:   str error
```

Object references returned as values (`T_REF`) are resolved on the server
side using the registered classes; new instances returned by a callback
get registered implicitly by the callee.

## Server stats

### M_STATS_REQ

Empty body. Used by clients to pick the least loaded server
(load balancing): `load = busy + queued`.

### M_STATS_RESP

```
u32 threads    (worker thread count)
u32 busy       (busy workers)
u32 queued     (queued jobs)
u32 clients    (connected clients)
```

## Job queue (persistent, optional)

When the server is started with `--queue`, scripts can be submitted to a
persistent SQLite-backed queue and fetched later.

### M_JOB_SUBMIT

Same body as `M_EXEC_REQ` (script, entry, arg_count, args).

### M_JOB_SUBMIT_RESP

```
u8 ok
if ok:  str uuid
else:   str error
```

### M_JOB_RESULT_REQ

```
str uuid
```

### M_JOB_RESULT_RESP

```
u8  status
    0    = pending (not finished yet)
    1    = finished, result follows
    0xFF = unknown uuid
if status == 1:
    u8 ok
    if ok:  value
    else:   str error
```

## Provider protocol

A provider is a backend server (see `s5run generate provider`). After the
hello it answers metadata requests and serves API calls. API calls reuse
`M_CALLBACK_REQ` / `M_CALLBACK_RESP` with the same body format.

### M_PROVIDER_MOUNT_REQ

Client -> **server**: mount context entries from a remote provider. The
server connects to the provider itself, fetches the metadata for the
requested names, registers the entries (functions / classes / global
objects) in the script engine and relays the API callbacks to the
provider **directly** - the mounting client does not participate in the
callback chain. The provider connection lives as long as the client
connection.

```
str host
u16 port
u32 name_count
str name [name_count]
```

### M_PROVIDER_MOUNT_RESP

```
u8  ok
str error (empty when ok)
```

This is how `s5run run --provider "host:port,name1,name2" script.js`
works (the FrontendModel: the script server talks to the backend
provider directly).

### M_PROVIDER_META_REQ

```
str name    ("" = list everything registered)
```

### M_PROVIDER_META_RESP

```
u8  entry_count
entry [entry_count]:
    u8  kind
        1 = function
        2 = class
        3 = global object
    str name
    if kind == 1 (function):
        str return_type
        u32 arg_count
        { str arg_name; str arg_type } [arg_count]
    if kind == 2 or 3 (class / global object):
        u32 method_count
        {
            str method_name
            str return_type
            u32 arg_count
            { str arg_name; str arg_type } [arg_count]
        } [method_count]
        u32 prop_count
        { str name; str getter; str setter } [prop_count]
        if kind == 3:
            i32 this_id
```

Types in the metadata use the schema type names (`bool`, `int`, `string`,
`array<T>`, class names, `var`, `void` for returns) - see
[SCHEMA.md](SCHEMA.md).

## Endianness summary

| Field | Byte order |
|-------|------------|
| Frame length (u32) | **big-endian** |
| Everything in the body | **little-endian** |
