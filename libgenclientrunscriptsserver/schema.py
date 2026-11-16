"""Schema model and validation."""

import json
from dataclasses import dataclass, field
from typing import List, Optional

from .types import parse


@dataclass
class Argument:
    name: str
    type: str

    def type_ref(self):
        return parse(self.type)


@dataclass
class Function:
    name: str
    return_type: str
    arguments: List[Argument] = field(default_factory=list)

    @property
    def return_ref(self):
        return parse(self.return_type)


@dataclass
class Method:
    name: str
    return_type: str
    arguments: List[Argument] = field(default_factory=list)

    @property
    def return_ref(self):
        return parse(self.return_type)


@dataclass
class ClassDef:
    name: str
    aggregate_interface: str
    methods: List[Method] = field(default_factory=list)


@dataclass
class Schema:
    namespace: str
    version: int
    cpp_output_dir: str
    java_output_dir: str
    py_output_dir: str
    functions: List[Function] = field(default_factory=list)
    classes: List[ClassDef] = field(default_factory=list)

    # -- lookups --
    @property
    def iface_names(self) -> set:
        return {c.aggregate_interface for c in self.classes}

    @property
    def class_names(self) -> set:
        return {c.name for c in self.classes}

    def validate(self) -> None:
        if not self.namespace:
            raise ValueError("schema: 'namespace' is required")
        for key, value in (
            ("cpp-output-dir",  self.cpp_output_dir),
            ("java-output-dir", self.java_output_dir),
            ("py-output-dir",   self.py_output_dir),
        ):
            if not value:
                raise ValueError(f"schema: '{key}' is required")

        seen_types = set()

        def collect(ref, where):
            if ref.is_array():
                collect(parse(ref.inner), where)
            elif ref.is_iface() or ref.is_class():
                seen_types.add(ref.name)

        for f in self.functions:
            collect(f.return_ref, f"function {f.name}")
            for a in f.arguments:
                collect(a.type_ref(), f"function {f.name} arg {a.name}")
        for c in self.classes:
            for m in c.methods:
                collect(m.return_ref, f"{c.name}.{m.name}")
                for a in m.arguments:
                    collect(a.type_ref(), f"{c.name}.{m.name} arg {a.name}")

        unresolved = seen_types - self.iface_names - self.class_names
        if unresolved:
            raise ValueError(f"schema: unresolved types: {sorted(unresolved)}")


def load_schema(path: str) -> Schema:
    with open(path, "r", encoding="utf-8") as f:
        raw = json.load(f)

    functions = [
        Function(
            name=f["name"],
            return_type=f["return"],
            arguments=[Argument(a["name"], a["type"]) for a in f.get("arguments", [])],
        )
        for f in raw.get("functions", [])
    ]

    classes = [
        ClassDef(
            name=c["name"],
            aggregate_interface=c["aggregate-interface"],
            methods=[
                Method(
                    name=m["name"],
                    return_type=m["return"],
                    arguments=[Argument(a["name"], a["type"])
                               for a in m.get("arguments", [])],
                )
                for m in c.get("methods", [])
            ],
        )
        for c in raw.get("classes", [])
    ]

    schema = Schema(
        namespace=raw["namespace"],
        version=raw.get("version", 1),
        cpp_output_dir=raw.get("cpp-output-dir", ""),
        java_output_dir=raw.get("java-output-dir", ""),
        py_output_dir=raw.get("py-output-dir", ""),
        functions=functions,
        classes=classes,
    )
    schema.validate()
    return schema
