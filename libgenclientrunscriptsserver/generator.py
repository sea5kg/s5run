"""Orchestration: load schema, invoke all generators, write files."""

import os

from .schema import Schema, load_schema
from . import cpp, py, java


class Generator:
    def __init__(self, schema: Schema):
        self.schema = schema

    @classmethod
    def from_file(cls, path: str) -> "Generator":
        return cls(load_schema(path))

    def generate_all(self) -> None:
        s = self.schema
        ns = s.namespace

        os.makedirs(s.cpp_output_dir, exist_ok=True)
        os.makedirs(s.py_output_dir, exist_ok=True)
        os.makedirs(s.java_output_dir, exist_ok=True)

        with open(os.path.join(s.cpp_output_dir, f"{ns}.h"), "w", encoding="utf-8") as f:
            f.write(cpp.generate_header(s))
        with open(os.path.join(s.cpp_output_dir, f"{ns}.cpp"), "w", encoding="utf-8") as f:
            f.write(cpp.generate_impl(s))
        with open(os.path.join(s.py_output_dir, f"{ns}.py"), "w", encoding="utf-8") as f:
            f.write(py.generate(s))
        java.generate(s, s.java_output_dir)

    def report(self) -> str:
        s = self.schema
        ns = s.namespace
        return "\n".join([
            f"cpp  -> {s.cpp_output_dir}{ns}.h, {s.cpp_output_dir}{ns}.cpp",
            f"py   -> {s.py_output_dir}{ns}.py",
            f"java -> {s.java_output_dir}{ns}/",
        ])
