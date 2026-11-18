"""Code generator for the run-scripts-server client protocol.

Reads s5run-client.json and emits typed stubs
for C++, Python and Java.
"""
from .generator import Generator
from .s5run_cli_schema import S5RunCliSchema

__all__ = ["Generator", "S5RunCliSchema"]
__version__ = "0.1.0"
