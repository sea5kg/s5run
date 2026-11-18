cmake_minimum_required(VERSION 3.20)
project(s5run-cpp11-client-test CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_executable(cpp11-client
    main_gen.cpp
    s5run_cli_example/s5run_cli_example.cpp
)

target_include_directories(
    cpp11-client
    PRIVATE
    s5run_cli_example
)

target_link_libraries(cpp11-client -lpthread)
