#!/usr/bin/env python3
"""Entry point. The real work lives in libgenclientrunscriptsserver."""

# import sys
# from libgenclientrunscriptsserver.cli import main

from libgenclientrunscriptsserver import Generator

gen = Generator.from_file("schema-clients-run-scripts-server.json")
gen.generate_all()
print(gen.report())

# only cpp
# from libgenclientrunscriptsserver import load_schema
# from libgenclientrunscriptsserver import cpp
# schema = load_schema("schema-clients-run-scripts-server.json")
# open("example.h", "w").write(cpp.generate_header(schema))
# open("example.cpp", "w").write(cpp.generate_impl(schema))