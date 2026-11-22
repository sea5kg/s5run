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

"""
Generator
"""

import os

from .s5run_schema import S5RunCliSchema
from . import s5run_java21_generator
from . import s5run_java17_generator


class S5RunCliGenerator:
    def __init__(self, schema: S5RunCliSchema):
        self.schema = schema

    def generate_java21(self) -> None:
        s = self.schema
        if not s.java21_output_dir:
            return
        s5run_java21_generator.generate(s, s.java21_output_dir)

    def generate_java17(self) -> None:
        s = self.schema
        if not s.java17_output_dir:
            return
        import pys5rungen.s5run_java17_generator as gen17
        # temporarily point the java generator at the java17 dirs
        orig_dir, orig_pkg = s.java17_output_dir, s.java17_package
        s.java21_output_dir, s.java17_package = s.java17_output_dir, s.java17_package
        try:
            gen17.generate(s, s.java17_output_dir)
        finally:
            s.java17_output_dir, s.java17_package = orig_dir, orig_pkg

    def generate_java11(self) -> None:
        s = self.schema
        if not s.java11_output_dir:
            return
        import pys5rungen.s5run_java11_generator as gen11
        orig_dir, orig_pkg = s.java11_output_dir, s.java11_package
        s.java11_output_dir, s.java11_package = s.java11_output_dir, s.java11_package
        try:
            gen11.generate(s, s.java11_output_dir)
        finally:
            s.java11_output_dir, s.java11_package = orig_dir, orig_pkg


    def generate_all(self) -> None:
        self.generate_java21()
        self.generate_java17()
        self.generate_java11()

    def report(self, what: str = "all") -> str:
        s = self.schema
        lines = []
        if what in ("all", "java21") and s.java21_output_dir:
            lines.append(f"java21 -> {s.java21_output_dir}{s.java21_package.replace('.', '/')}/")
        if what in ("all", "java17") and s.java17_output_dir:
            lines.append(f"j17  -> {s.java17_output_dir}{s.java17_package.replace('.', '/')}/")
        if what in ("all", "java11") and s.java11_output_dir:
            lines.append(f"j11  -> {s.java11_output_dir}{s.java11_package.replace('.', '/')}/")
        return "\n".join(lines)
