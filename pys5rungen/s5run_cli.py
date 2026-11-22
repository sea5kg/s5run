#!/usr/bin/env python3
##################################################################################
#
#            ░░░░░░░
#           ░██▓▓▓▓▒
#  ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
#  ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
#   ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
#  ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
#   ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
#
# MIT License
#
# Copyright (c) 2026 Evgenii Sopov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Original repository: https://github.com/sea5kg/s5run
#
##################################################################################

"""CLI entry point."""

import json
import os
import sys

from .s5run_generator import S5RunCliGenerator
from .s5run_schema import S5RunCliSchema

SCHEMA_FILE = "s5run-schema.json"

DEFAULT_SCHEMA = {
    "options": {
        "cpp11": {
            "output-dir": "s5run-cli/",
            "namespace": "s5run_cli_example"
        },
        "java": {
            "output-dir": "src/",
            "package": "com.s5run.example"
        },
        "py": {
            "output-dir": "./",
            "module": "libs5run_cli_example"
        }
    },
    "version": 1,
    "functions": [
        {
            "name": "GetTimeNow",
            "return": "int",
            "arguments": []
        }
    ],
    "classes": [
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
                }
            ]
        }
    ]
}


def print_usage() -> None:
    print(
        "usage: python3 -m pys5rungen <command>\n"
        "\n"
        "commands:\n"
        f"  init    create a default example {SCHEMA_FILE} in the current directory\n"
        "  all     generate client code for all configured languages\n"
        "  cpp11   generate only the C++11 client\n"
        "  java21  generate only the Java 21 client\n"
        "  java17  generate only the Java 17 client\n"
        "  java11  generate only the Java 11 client\n"
        "  py      generate only the Python client\n"
        "  help    show this help"
    )


def cmd_init() -> int:
    if os.path.exists(SCHEMA_FILE):
        print(f"error: {SCHEMA_FILE} already exists in {os.getcwd()}", file=sys.stderr)
        return 1
    with open(SCHEMA_FILE, "w", encoding="utf-8") as f:
        json.dump(DEFAULT_SCHEMA, f, indent=2)
        f.write("\n")
    print(f"created {SCHEMA_FILE}")
    return 0


def _load_generator():
    if not os.path.isfile(SCHEMA_FILE):
        print(f"error: {SCHEMA_FILE} not found in {os.getcwd()}", file=sys.stderr)
        return None
    schema = S5RunCliSchema()
    schema.load(SCHEMA_FILE)
    return S5RunCliGenerator(schema)


def main(argv=None) -> int:
    if argv is None:
        argv = sys.argv[1:]

    if not argv or argv[0] in ("help", "-h", "--help"):
        print_usage()
        return 0 if argv else 2

    command = argv[0]
    if command == "init":
        return cmd_init()

    gen = _load_generator()
    if gen is None:
        return 1

    if command == "all":
        gen.generate_all()
    elif command == "cpp11":
        gen.generate_cpp11()
    elif command == "java21":
        gen.generate_java21()
    elif command == "java17":
        gen.generate_java17()
    elif command == "java11":
        gen.generate_java11()
    elif command == "py":
        gen.generate_py()
    else:
        print(f"error: unknown command: {command}", file=sys.stderr)
        print_usage()
        return 2

    print(gen.report(command))
    return 0


if __name__ == "__main__":
    sys.exit(main())
