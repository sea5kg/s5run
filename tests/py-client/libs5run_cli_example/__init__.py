# AUTO-GENERATED. DO NOT EDIT.
# package: libs5run_cli_example
"""Generated s5run client (1.0.0). Inherit the base classes and register them on a Context."""

CLIENT_VERSION = "1.0.0"


def client_version() -> str:
    return CLIENT_VERSION


from .variable import Ref
from .types import RemoteServerInfo, RunResult, S5RunFunction, S5RunObject
from .types import File
from .types import FileSystem
from .types import console
from .types import KeyValueStorage
from .types import GlobalFunction_GetFile
from .types import GlobalFunction_GetFileSize
from .types import GlobalFunction_GetTimeNow
from .types import GlobalFunction_HasFile
from .types import GlobalFunction_GetFileList
from .types import GlobalFunction_GetOpenedFiles
from .types import GlobalFunction_CloseFiles
from .types import GlobalFunction_GetPi
from .types import GlobalFunction_GetServerPort
from .context import Context, run_script
