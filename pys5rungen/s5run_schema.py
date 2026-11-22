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

"""Schema model and validation."""

import os
import sys
import json
from typing import List

from .s5run_typeref import S5RunCliTypeRef, s5run_parse_type


class S5RunCliArgument:

    def __init__(self, type: str, name: str, convert_from: str = ""):
        self.__type = type
        self.__name = name
        self.__convert_from = convert_from

    def type(self) -> str:
        return self.__type

    def name(self) -> str:
        return self.__name

    def convert_from(self) -> str:
        return self.__convert_from

    def type_ref(self) -> S5RunCliTypeRef:
        return s5run_parse_type(self.__type)


class S5RunCliFunction:

    def __init__(self, name: str, return_type: str, arguments: List[S5RunCliArgument] = None):
        self.__name = name
        self.__return_type = return_type
        self.__arguments = arguments if arguments is not None else []

    def name(self) -> str:
        return self.__name

    def return_type(self) -> str:
        return self.__return_type

    def arguments(self) -> List[S5RunCliArgument]:
        return self.__arguments

    def return_ref(self):
        return s5run_parse_type(self.__return_type)


class S5RunCliMethod:

    def __init__(self, name: str, return_type: str, arguments: List[S5RunCliArgument] = None):
        self.__name = name
        self.__return_type = return_type
        self.__arguments = arguments if arguments is not None else []

    def name(self) -> str:
        return self.__name

    def return_type(self) -> str:
        return self.__return_type

    def arguments(self) -> List[S5RunCliArgument]:
        return self.__arguments

    def return_ref(self):
        return s5run_parse_type(self.__return_type)


class S5RunCliClass:

    def __init__(self, name: str, methods: List[S5RunCliMethod] = None):
        self.__name = name
        self.__aggregate_interface = "I" + name
        self.__methods = methods if methods is not None else []

    def name(self) -> str:
        return self.__name

    def aggregate_interface(self) -> str:
        return self.__aggregate_interface

    def methods(self) -> List[S5RunCliMethod]:
        return self.__methods


class S5RunCliSchemaCppOptions:
    """ helper class for options generator """
    def __init__(self, name: str, opt):
        self.__name = name
        self.__opt = opt
        if "namespace" not in opt:
            sys.exit("Missing 'namespace' in options." + name + " " + str(opt))
        # TODO validate "namespace" - because used as filename
        if "output-dir" not in opt:
            sys.exit("Missing 'output-dir' in options." + name + " " + str(opt))

    def get_name(self):
        """ return name """
        return self.__name

    def namespace(self):
        return self.__opt["namespace"]

    def output_dir(self):
        return self.__opt["output-dir"]

class S5RunCliSchema:

    def __init__(self):
        self.__schema = {}
        self.__options = {}
        self.version = 1
        self.client_version = "0.0.0"
        self.java21_package = ""
        self.java21_output_dir = ""
        self.py_module = ""
        self.java17_output_dir = ""
        self.java17_package = ""
        self.java11_output_dir = ""
        self.java11_package = ""
        self.java11_output_dir = ""
        self.py_output_dir = ""
        self.go_output_dir = ""
        self.go_package = ""
        self.functions: List[S5RunCliFunction] = []
        self.classes: List[S5RunCliClass] = []

    def load(self, schema_filepath):
        if not os.path.isfile(schema_filepath):
            sys.exit("Not found file: " + schema_filepath)
        with open(schema_filepath, "r", encoding="utf-8") as f:
            self.__schema = json.load(f)
        self.__parse_version()
        self.__parse_client_version()
        self.__parse_java21_options()
        self.__parse_java17_options()
        self.__parse_java11_options()
        self.__parse_py_options()
        self.__parse_go_options()
        self.__resolve_names()
        self.__parse_functions()
        self.__parse_classes()
        self.__validate()

    def get_cpp_options(self, name) -> S5RunCliSchemaCppOptions:
        if name not in self.__options:
            sys.exit("Not found " + name + " in options")
        return self.__options[name]

    def __parse_version(self):
        if "version" in self.__schema:
            self.version = self.__schema["version"]

    def __parse_client_version(self):
        if "client-version" not in self.__schema:
            return
        value = self.__schema["client-version"]
        parts = str(value).split(".")
        if len(parts) != 3 or not all(p.isdigit() and p != "" for p in parts):
            sys.exit("S5RunCliSchema: invalid 'client-version' (expected X.Y.Z): " + str(value))
        self.client_version = str(value)

    def __parse_java21_options(self):
        section = self.__parse_options_section("java", required=False)
        if section is None:
            return
        self.java21_output_dir = section["output-dir"]
        if "package" in section:
            self.java21_package = section["package"]

    def __parse_java17_options(self):
        section = self.__parse_options_section("java17", required=False)
        if section is None:
            return
        self.java17_output_dir = section["output-dir"]
        if "package" in section:
            self.java17_package = section["package"]

    def __parse_java11_options(self):
        section = self.__parse_options_section("java11", required=False)
        if section is None:
            return
        self.java11_output_dir = section["output-dir"]
        if "package" in section:
            self.java11_package = section["package"]

    def __parse_py_options(self):
        section = self.__parse_options_section("py", required=False)
        if section is None:
            return
        self.py_output_dir = section["output-dir"]
        if "module" in section:
            self.py_module = section["module"]

    def __parse_go_options(self):
        section = self.__parse_options_section("go", required=False)
        if section is None:
            return
        self.go_output_dir = section["output-dir"]
        if "package" in section:
            self.go_package = section["package"]

    def __resolve_names(self):
        # Languages without an explicit name fall back to the shared namespace.
        if not self.java21_package:
            self.java21_package = self.namespace
        if not self.py_module:
            self.py_module = self.namespace

    def __parse_options_section(self, name, required=True):
        if "options" not in self.__schema:
            if required:
                sys.exit("Not found section 'options' in schema")
            return None
        options = self.__schema["options"]
        if name not in options:
            if required:
                sys.exit("Not found section 'options." + name + "' in schema")
            return None
        section = options[name]
        if "output-dir" not in section:
            sys.exit("Missing 'output-dir' in options." + name + " " + str(section))
        return section

    def __parse_arguments(self, func):
        if "arguments" not in func:
            return []
        raw = func["arguments"]
        if isinstance(raw, dict):
            # Object form: a "conversion function" — all arguments passed
            # from the script are collected into a single parameter.
            for key in ("convert-from", "type", "name"):
                if key not in raw:
                    sys.exit("Missing '" + key + "' in arguments (object form) of " + str(func))
            if raw["convert-from"] != "...":
                sys.exit("Unsupported 'convert-from' value '" + str(raw["convert-from"])
                         + "' in " + str(func))
            if raw["type"] != "array<string>":
                sys.exit("Unsupported 'type' for convert arguments (only array<string>): "
                         + str(raw["type"]) + " in " + str(func))
            return [
                S5RunCliArgument(
                    type=raw["type"],
                    name=raw["name"],
                    convert_from=raw["convert-from"],
                )
            ]
        ret = []
        for a in raw:
            if "type" not in a:
                sys.exit("Missing argument type in function " + str(func))
            if "name" not in a:
                sys.exit("Missing argument name in function " + str(func))

            a0 = S5RunCliArgument(type=a["type"], name=a["name"])
            ret.append(a0)
        return ret

    def __parse_functions(self):
        self.functions = []
        if "functions" in self.__schema:
            for f in self.__schema["functions"]:
                if "name" not in f:
                    sys.exit("Not found name in function define " + str(f))
                f_name = f["name"]
                f_return = "void"
                if "return" in f:
                    f_return = f["return"]
                f_args = []
                if "arguments" in f:
                    f_args = self.__parse_arguments(f)
                self.functions.append(
                    S5RunCliFunction(
                        name=f_name,
                        return_type=f_return,
                        arguments=f_args,
                    )
                )

    def __parse_classes(self):
        self.classes = []
        if "classes" in self.__schema:
            for c in self.__schema["classes"]:
                if "name" not in c:
                    sys.exit("Not found name in class define " + str(c))
                c_methods = []
                if "methods" in c:
                    for m in c["methods"]:
                        if "name" not in m:
                            sys.exit("Not found name in method define " + str(m))
                        m_name = m["name"]
                        m_return = "void"
                        if "return" in m:
                            m_return = m["return"]
                        m_args = []
                        if "arguments" in m:
                            m_args = self.__parse_arguments(m)
                        c_methods.append(
                            S5RunCliMethod(
                                name=m_name,
                                return_type=m_return,
                                arguments=m_args,
                            )
                        )
                self.classes.append(
                    S5RunCliClass(
                        name=c["name"],
                        methods=c_methods,
                    )
                )

    # -- lookups --
    @property
    def iface_names(self) -> set:
        return {c.aggregate_interface() for c in self.classes}

    @property
    def class_names(self) -> set:
        return {c.name() for c in self.classes}

    def __validate(self) -> None:
        known = self.iface_names | self.class_names
        unresolved = {}  # type name -> list of usages

        def collect(ref, where):
            if ref.is_array():
                collect(s5run_parse_type(ref.inner()), where)
            elif ref.is_ref():
                if ref.name() not in known:
                    unresolved.setdefault(ref.name(), []).append(where)
        for f in self.functions:
            collect(f.return_ref(), "function " + f.name() + " return")
            for a in f.arguments():
                collect(a.type_ref(), "function " + f.name() + " arg " + a.name())
        for c in self.classes:
            for m in c.methods():
                collect(m.return_ref(), "class " + c.name() + " method " + m.name() + " return")
                for a in m.arguments():
                    collect(a.type_ref(), "class " + c.name() + " method " + m.name() + " arg " + a.name())

        if unresolved:
            lines = ["S5RunCliSchema: unresolved types (no class defined):"]
            for t in sorted(unresolved):
                for where in unresolved[t]:
                    lines.append("  '" + t + "' used in " + where)
            sys.exit("\n".join(lines))
