"""Type system shared by all generators."""

from dataclasses import dataclass
from typing import Optional

PRIMITIVES = {
    "bool":   {"cpp": "bool",        "py": "bool",  "java": "boolean", "wire": "bool"},
    "int":    {"cpp": "int64_t",     "py": "int",   "java": "long",    "wire": "i64"},
    "uint":   {"cpp": "uint64_t",    "py": "int",   "java": "long",    "wire": "u64"},
    "float":  {"cpp": "double",      "py": "float", "java": "double",  "wire": "f64"},
    "double": {"cpp": "double",      "py": "float", "java": "double",  "wire": "f64"},
    "string": {"cpp": "std::string", "py": "str",   "java": "String",  "wire": "str"},
    "byte":   {"cpp": "uint8_t",     "py": "int",   "java": "byte",    "wire": "u8"},
}


@dataclass(frozen=True)
class TypeRef:
    """Parsed type reference.

    kind is one of: 'prim', 'array', 'iface', 'class'.
    inner is set for 'array' (element type) and for 'iface'/'class' (name).
    """
    kind: str
    name: str          # original string, e.g. "array<string>"
    inner: Optional[str] = None

    def is_prim(self) -> bool:  return self.kind == "prim"
    def is_array(self) -> bool: return self.kind == "array"
    def is_iface(self) -> bool: return self.kind == "iface"
    def is_class(self) -> bool: return self.kind == "class"

    # ---- language mappings ----
    def cpp(self) -> str:
        if self.is_prim():
            return PRIMITIVES[self.name]["cpp"]
        if self.is_array():
            return f"std::vector<{parse(self.inner).cpp()}>"
        if self.is_iface():
            return f"std::shared_ptr<{self.name}>"
        return self.name

    def py(self) -> str:
        if self.is_prim():
            return PRIMITIVES[self.name]["py"]
        if self.is_array():
            return f"List[{parse(self.inner).py()}]"
        return f'"{self.name}"'

    def java(self) -> str:
        if self.is_prim():
            return PRIMITIVES[self.name]["java"]
        if self.is_array():
            inner = parse(self.inner).java()
            boxed = {"long": "Long", "boolean": "Boolean",
                     "double": "Double", "byte": "Byte"}
            return f"List<{boxed.get(inner, inner)}>"
        return self.name

    def wire(self) -> str:
        if self.is_prim():
            return PRIMITIVES[self.name]["wire"]
        raise ValueError(f"wire() is only for primitives, got {self.name}")


def parse(raw: str) -> TypeRef:
    """Parse a type string like 'array<string>' or 'IFile' into TypeRef."""
    raw = raw.strip()
    if raw.startswith("array<") and raw.endswith(">"):
        return TypeRef(kind="array", name=raw, inner=raw[6:-1].strip())
    if raw in PRIMITIVES:
        return TypeRef(kind="prim", name=raw)
    if raw.startswith("I") and len(raw) > 1:
        return TypeRef(kind="iface", name=raw)
    return TypeRef(kind="class", name=raw)
