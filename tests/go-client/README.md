# go-client (draft)

Generated Go client for the s5run binary protocol.

- Source schema: `s5run-schema.json` (`options.go`)
- Regenerate: `python3 -m pys5rungen go`
- Package: `s5runcli`

**TODO: the code has not been compiled yet** — no Go toolchain was
available at generation time. Review it, install Go and run:

```sh
cd tests/go-client
go build ./...
```

Planned example usage (mirrors the cpp/java/python clients):

```go
ctx := s5runcli.NewContext()
ctx.AddClass(MyFile{}).
    AddFunction(MyGetTimeNow{}).
    AddObject(MyConsole{}).
    AddServer(s5runcli.NewRemoteServerInfo().Address("s5run://127.0.0.1:7777")).
    SetScript(script)

result := s5runcli.RunScript(ctx, "main", []s5runcli.Value{s5runcli.Str("test.txt")})
if result.HasError {
    panic(result.Error)
}
```
