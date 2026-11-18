"""Orchestration: load schema, invoke all generators, write files."""

import os

from .s5run_cli_schema import S5RunCliSchema
from . import s5run_cpp11_generator, s5run_py_generator, s5run_java_generator


class Generator:
    def __init__(self, schema: S5RunCliSchema):
        self.schema = schema

    @classmethod
    def from_file(cls, path: str) -> "Generator":
        schema = S5RunCliSchema()
        schema.load(path)
        return cls(schema)

    def generate_all(self) -> None:
        s = self.schema
        ns = s.namespace

        if s.cpp_output_dir:
            os.makedirs(s.cpp_output_dir, exist_ok=True)
            with open(os.path.join(s.cpp_output_dir, f"{ns}.h"), "w", encoding="utf-8") as f:
                f.write(s5run_cpp11_generator.generate_header(s))
            with open(os.path.join(s.cpp_output_dir, f"{ns}.cpp"), "w", encoding="utf-8") as f:
                f.write(s5run_cpp11_generator.generate_impl(s))
        if s.py_output_dir:
            os.makedirs(s.py_output_dir, exist_ok=True)
            with open(os.path.join(s.py_output_dir, f"{s.py_module}.py"), "w", encoding="utf-8") as f:
                f.write(s5run_py_generator.generate(s))
        if s.java_output_dir:
            s5run_java_generator.generate(s, s.java_output_dir)

    def report(self) -> str:
        s = self.schema
        lines = []
        if s.cpp_output_dir:
            lines.append(f"cpp  -> {s.cpp_output_dir}{s.namespace}.h, {s.cpp_output_dir}{s.namespace}.cpp")
        if s.py_output_dir:
            lines.append(f"py   -> {s.py_output_dir}{s.py_module}.py")
        if s.java_output_dir:
            lines.append(f"java -> {s.java_output_dir}{s.java_package.replace('.', '/')}/")
        return "\n".join(lines)
