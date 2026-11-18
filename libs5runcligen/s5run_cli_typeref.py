"""Language-agnostic type system: parsing and wire tags only.

Per-language type resolving lives in the generators:
s5run_cpp11_generator.type_to_cpp, s5run_py_generator.type_to_py,
s5run_java_generator.type_to_java.
"""

from typing import Optional

_PRIMITIVES = ["bool", "int", "uint", "float", "double", "string", "byte"]


class S5RunCliTypeRef:
    """Parsed type reference.

    kind is one of: 'prim', 'array', 'ref'.
    inner is set for 'array' (element type).
    'ref' is a reference to a remote object (interface or class name
    from the schema); over the wire it is just this_id.
    """

    def __init__(self, kind: str, name: str, inner: Optional[str] = None):
        self.__kind = kind
        self.__name = name
        self.__inner = inner

    def kind(self) -> str:
        return self.__kind

    def name(self) -> str:
        return self.__name

    def inner(self) -> Optional[str]:
        return self.__inner

    def is_prim(self) -> bool:
        return self.__kind == "prim"

    def is_array(self) -> bool:
        return self.__kind == "array"

    def is_ref(self) -> bool:
        return self.__kind == "ref"

    def is_void(self) -> bool:
        return self.__kind == "void"

    def wire(self) -> str:
        """Primitive name as declared in the schema.

        Translation into a concrete wire representation is decided by
        the language generators.
        """
        if self.is_prim():
            return self.__name
        raise ValueError(f"wire() is only for primitives, got {self.__name}")


def s5run_parse_type(raw: str) -> S5RunCliTypeRef:
    """Parse a type string like 'array<string>' or 'IFile' into S5RunCliTypeRef."""
    raw = raw.strip()
    if raw == "void":
        return S5RunCliTypeRef(kind="void", name="void")
    if raw.startswith("array<") and raw.endswith(">"):
        return S5RunCliTypeRef(kind="array", name=raw, inner=raw[6:-1].strip())
    if raw in _PRIMITIVES:
        return S5RunCliTypeRef(kind="prim", name=raw)
    return S5RunCliTypeRef(kind="ref", name=raw)
