cmake_minimum_required(VERSION 3.20)
project(s5run-cpp98-client-test CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_executable(cpp98-client
    main_test.cpp
    s5run_cli/s5run_cli_cpp98.cpp
)

target_include_directories(
    cpp98-client
    PRIVATE
    s5run_cli
)

if(WIN32)
    target_link_libraries(cpp98-client ws2_32)
else()
    target_link_libraries(cpp98-client -lpthread)
endif()
