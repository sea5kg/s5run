"""CLI entry point."""

import os
import sys

from .generator import Generator

SCHEMA_FILE = "sea5kg-run-scripts-client.json"


def main(argv=None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    if argv:
        print("usage: gen_clients_code.py", file=sys.stderr)
        return 2

    if not os.path.isfile(SCHEMA_FILE):
        print(f"{SCHEMA_FILE} not found in {os.getcwd()}", file=sys.stderr)
        return 1

    gen = Generator.from_file(SCHEMA_FILE)
    gen.generate_all()
    print(gen.report())
    return 0
