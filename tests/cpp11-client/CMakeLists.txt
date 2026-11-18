cmake_minimum_required(VERSION 3.20)
project(s5run-cpp-client-test CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_executable(cpp-client-test
    test_client.cpp
    ../../src/s5run/s5run_protocol.cpp
    ../../src/s5run/value_wire.cpp
)

target_include_directories(
    cpp-client-test
    PRIVATE
    ../../src/s5run
)

target_link_libraries(cpp-client-test -lpthread)

add_executable(cpp-gen-test
    main_gen.cpp
    s5run_cli_example/s5run_cli_example.cpp
)

target_include_directories(
    cpp-gen-test
    PRIVATE
    s5run_cli_example
)

target_link_libraries(cpp-gen-test -lpthread)
