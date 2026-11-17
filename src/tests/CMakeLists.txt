file(GLOB ALL_TESTS
     "test_*.cpp"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY tests)

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/sqlite3 ${CMAKE_CURRENT_BINARY_DIR}/sqlite3)

list (APPEND TEST_CTF01D_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../")


list (APPEND TEST_CTF01D_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../src/value_wire.cpp")
list (APPEND TEST_CTF01D_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../src/include/")

# wsjcpp-employees

list (APPEND TEST_CTF01D_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../src.wsjcpp/wsjcpp_employees/wsjcpp_employees.cpp")

include_directories(${TEST_CTF01D_INCLUDE_DIRS})

foreach(_TEST ${ALL_TESTS})
    get_filename_component(TEST_NAME ${_TEST} NAME_WE)
    add_executable(${TEST_NAME}
        ${_TEST}
        ${TEST_CTF01D_SOURCES}
    )
    target_link_libraries(${TEST_NAME} -lpthread -lssl -lcrypto ${WSJCPP_LIBRARIES})

    add_test(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:${TEST_NAME}>
    )
    message(${CMAKE_CURRENT_BINARY_DIR}/tests)
    set_target_properties (${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY tests)
endforeach()
