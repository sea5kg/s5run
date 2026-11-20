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

cmake_minimum_required(VERSION 3.20)
project(s5run C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${s5run_SOURCE_DIR})

include(${CMAKE_CURRENT_SOURCE_DIR}/src.wsjcpp/CMakeLists.txt)

#### BEGIN_WSJCPP_APPEND
#### END_WSJCPP_APPEND

find_package(Threads REQUIRED)

# include header dirs
list (APPEND WSJCPP_INCLUDE_DIRS "src")
list (APPEND WSJCPP_INCLUDE_DIRS "src/s5run")
list (APPEND WSJCPP_INCLUDE_DIRS "src/s5run/include")

# # Lua-engine
# file(GLOB LUA_SRC CONFIGURE_DEPENDS src/third_party/lua/*.c)
# if(NOT LUA_SRC)
#   message(FATAL_ERROR
#     "Lua sources not found in src/third_party/lua. "
#     "Run: git clone -b v5.4.6 https://github.com/lua/lua third_party/lua")
# endif()
# list(FILTER LUA_SRC EXCLUDE REGEX "/lua\\.c$")
# list(FILTER LUA_SRC EXCLUDE REGEX "/luac\\.c$")
# add_library(lua_static STATIC ${LUA_SRC})
# target_include_directories(lua_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/lua)
# set_target_properties(lua_static PROPERTIES
#   LINKER_LANGUAGE C
#   C_STANDARD 99
# )

# QuickJS-engine
file(GLOB QJS_SRC CONFIGURE_DEPENDS
    src/third_party/quickjs/quickjs.c
    src/third_party/quickjs/cutils.c
    src/third_party/quickjs/dtoa.c
    src/third_party/quickjs/libregexp.c
    src/third_party/quickjs/libunicode.c
)
if(NOT QJS_SRC)
  message(FATAL_ERROR
    "QuickJS sources not found in src/third_party/quickjs. "
    "Run: git clone -b v0.17.0 https://github.com/quickjs-ng/quickjs.git src/third_party/quickjs")
endif()

add_library(quickjs_static STATIC ${QJS_SRC})
target_include_directories(quickjs_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/quickjs)
set_target_properties(quickjs_static PROPERTIES
  LINKER_LANGUAGE C
  C_STANDARD 11
)

add_subdirectory(src/third_party/sqlite3)
# https://www.sqlite.org/threadsafe.html
# target_compile_definitions(sqlite3-static
#   PRIVATE
#     SQLITE_THREADSAFE=0
# )
list (APPEND WSJCPP_LIBRARIES sqlite3-static)
list (APPEND WSJCPP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src/third_party/sqlite3/src")

list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src")
list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/main.cpp")
list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/s5run/script_engines/quickjs_script_engine.cpp")
# list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/s5run/script_engines/lua_script_engine.cpp")
list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/s5run/s5run_protocol.cpp")
list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/s5run/s5run_server.cpp")
list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/s5run/s5run_queue.cpp")
list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/s5run/s5run_thread_pool.cpp")
list (APPEND WSJCPP_SOURCES "${PROJECT_SOURCE_DIR}/src/s5run/value_wire.cpp")


# server
add_executable(s5run ${WSJCPP_SOURCES})

target_include_directories(
  s5run
  PRIVATE
  ${WSJCPP_INCLUDE_DIRS}
)

target_link_libraries(
  s5run
  PRIVATE
  # lua_static
  quickjs_static
  Threads::Threads
  -lssl -lcrypto
  ${WSJCPP_LIBRARIES}
)

# enable testing functionality
enable_testing()
add_subdirectory(src/tests)
