#!/usr/bin/env python3

# import sys
# from libs5run_client_code_generator.cli import main

from libs5run_client_code_generator import Generator

gen = Generator.from_file("s5run-client.json")
gen.generate_all()
print(gen.report())

# only cpp
# from libs5run_client_code_generator import load_schema
# from libs5run_client_code_generator import cpp
# schema = load_schema("s5run-client.json")
# open("example.h", "w").write(cpp.generate_header(schema))
# open("example.cpp", "w").write(cpp.generate_impl(schema))