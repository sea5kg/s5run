"""Code generator for the run-scripts-server client protocol.

Reads s5run-client.json and emits typed stubs
for C++, Python and Java.
"""
from .generator import Generator
from .schema import Schema, load_schema

__all__ = ["Generator", "Schema", "load_schema"]
__version__ = "0.1.0"
