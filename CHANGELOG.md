
# Changelog

## [v0.3.0] - (2026 ?? ??)

- Added wsjcpp-employees:v0.2.2 for a server
- Added sea5kg-logger:v2.0.1 for a server
- Added wsjcpp-yaml:v0.1.10 for a server
- Added sea5kg-sqlite3-wrapper:v0.1.1 for a server
- Added libhv:v1.3.3 for a server
- Added new type 'var' to schema and added some implementation
- In output client libraries renamed system classes and renaming class 'value' to 'variable'
- Redesigned 's5run run': '--script' removed and removed implementation from stdin
- Fixed run localhost:7777 (now possible run use a via domain)
- Redesigned 's5run server' command with default port 7777
- Redesigned tests any test will be read script from file.
- Implemented 'properties' and added tests
- Improved tests/cpp11-client prepare new tests without code-write just a files 'input_/script_/output_'
- Added new methods to s5run_variable 'to_string_s5run' and 'to_string_json' to easy using
- Implemented example for tests/cpp98-provider
- Added docs/SCHEMA.md

## [v0.2.0] - (2026 Nov 24)

- Added wsjcpp package manager
- Added send server hello "s5run <server-version> <protocol-version>"
- Fixed periodically crash in java-client
- Redesign command line
- Generator cpp11 moved to s5run app
- Added generators for cpp98, java11, java17, java21, go, py
- Renamed s5run-client.json -> s5run-schema.json
- Added './s5run generate client ..'
- Added './s5run run host:port script.js'
- Updated build env installed golang compiler and java sdk

## [v0.1.0] - (2026 Nov 20)

- Implemented first version
