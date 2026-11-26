cmake_minimum_required(VERSION 3.20)
project(s5run-cpp98-provider-test CXX)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_executable(cpp98-provider
    main_provider_test.cpp
    s5run_provider/d2_s5run_provider.cpp
)

target_include_directories(
    cpp98-provider
    PRIVATE
    s5run_provider
)

if (WIN32)
    target_link_libraries(cpp98-provider ws2_32)
else()
    target_link_libraries(cpp98-provider -lpthread)
endif()
