cmake_minimum_required(VERSION 3.20)
project(sea5kg-run-scripts-server C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${sea5kg-run-scripts-server_SOURCE_DIR})

find_package(Threads REQUIRED)

# Lua-engine
file(GLOB LUA_SRC CONFIGURE_DEPENDS src/third_party/lua/*.c)
if(NOT LUA_SRC)
  message(FATAL_ERROR
    "Lua sources not found in src/third_party/lua. "
    "Run: git clone -b v5.4.6 https://github.com/lua/lua third_party/lua")
endif()
list(FILTER LUA_SRC EXCLUDE REGEX "/lua\\.c$")
list(FILTER LUA_SRC EXCLUDE REGEX "/luac\\.c$")
add_library(lua_static STATIC ${LUA_SRC})
target_include_directories(lua_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party/lua)
set_target_properties(lua_static PROPERTIES
  LINKER_LANGUAGE C
  C_STANDARD 99
)

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
add_executable(sea5kg-run-scripts-server
    src/main.cpp
    src/s5run/script_engines/quickjs_script_engine.cpp
    src/s5run/script_engines/lua_script_engine.cpp
    src/s5run/s5run_protocol.cpp
    src/s5run/s5run_server.cpp
    src/s5run/thread_pool.cpp
)

target_include_directories(
  sea5kg-run-scripts-server
  PRIVATE
  src/s5run
  src/s5run/include
)

target_link_libraries(
  sea5kg-run-scripts-server
  PRIVATE
  lua_static
  quickjs_static
  Threads::Threads
)

# client for tests
add_executable(
  test-client
  src/test_client/example/example.cpp
  src/test_client/example/run_script_client.cpp
  src/test_client/client.cpp
  src/s5run/s5run_protocol.cpp
)
target_include_directories(
  test-client
  PRIVATE
  src/s5run
)

# enable testing functionality
enable_testing()
add_subdirectory(src/tests)
