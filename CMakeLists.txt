cmake_minimum_required(VERSION 3.20)
project(s5run C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${s5run_SOURCE_DIR})

find_package(Threads REQUIRED)

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

# server
add_executable(s5run
    src/main.cpp
    src/s5run/script_engines/quickjs_script_engine.cpp
    # src/s5run/script_engines/lua_script_engine.cpp
    src/s5run/s5run_protocol.cpp
    src/s5run/s5run_server.cpp
    src/s5run/s5run_thread_pool.cpp
    src/s5run/value_wire.cpp
)

target_include_directories(
  s5run
  PRIVATE
  src/s5run
  src/s5run/include
)

target_link_libraries(
  s5run
  PRIVATE
  # lua_static
  quickjs_static
  Threads::Threads
)

# enable testing functionality
enable_testing()
add_subdirectory(src/tests)
