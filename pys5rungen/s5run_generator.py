#
# MIT License
#
#           ░░░░░░░
#          ░██▓▓▓▓▒
# ░▓▓▓▓▓▓░ ░█▓░░░    ▓▓▒▓█ ▒█▒   ██░ ░▓▓▒▓██▓
# ▒█▓░░░░░ ▒██▓▓▓█▓  ██▓   ▓█▓   ██░ ▒██░  ▓█▒
#  ░▒▓▓██▒       ██▒ ██░   ▓█▓   ██░ ▒█▓   ▓█▒
# ▒▓▒░░▒█▓ ▒█▓▒▒▓█▓  ██░   ▒██▒░▒██░ ▒█▓   ▓█▒
#  ░▒▒▒▒░   ░▒▒▒▒░   ▒▒░    ░▒▒▒ ▒▒░ ░▒▒   ░▒░
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

"""
Generator
"""

import os

from .s5run_schema import S5RunCliSchema
from . import s5run_cpp11_generator, s5run_java_generator, s5run_py_generator


class S5RunCliGenerator:
    def __init__(self, schema: S5RunCliSchema):
        self.schema = schema

    def generate_cpp11(self) -> None:
        s = self.schema
        if not s.cpp_output_dir:
            return
        os.makedirs(s.cpp_output_dir, exist_ok=True)
        with open(os.path.join(s.cpp_output_dir, f"{s.namespace}.h"), "w", encoding="utf-8") as f:
            f.write(s5run_cpp11_generator.generate_header(s))
        with open(os.path.join(s.cpp_output_dir, f"{s.namespace}.cpp"), "w", encoding="utf-8") as f:
            f.write(s5run_cpp11_generator.generate_impl(s))

    def generate_py(self) -> None:
        s = self.schema
        if not s.py_output_dir:
            return
        os.makedirs(s.py_output_dir, exist_ok=True)
        with open(os.path.join(s.py_output_dir, f"{s.py_module}.py"), "w", encoding="utf-8") as f:
            f.write(s5run_py_generator.generate(s))

    def generate_java(self) -> None:
        s = self.schema
        if not s.java_output_dir:
            return
        s5run_java_generator.generate(s, s.java_output_dir)

    def generate_all(self) -> None:
        self.generate_cpp11()
        self.generate_py()
        self.generate_java()

    def report(self, what: str = "all") -> str:
        s = self.schema
        lines = []
        if what in ("all", "cpp11") and s.cpp_output_dir:
            lines.append(f"cpp  -> {s.cpp_output_dir}{s.namespace}.h, {s.cpp_output_dir}{s.namespace}.cpp")
        if what in ("all", "py") and s.py_output_dir:
            lines.append(f"py   -> {s.py_output_dir}{s.py_module}.py")
        if what in ("all", "java") and s.java_output_dir:
            lines.append(f"java -> {s.java_output_dir}{s.java_package.replace('.', '/')}/")
        return "\n".join(lines)
