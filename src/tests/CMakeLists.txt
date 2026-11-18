file(GLOB ALL_TESTS
     "test_*.cpp"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY tests)

list (APPEND TEST_S5RUN_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../")


list (APPEND TEST_S5RUN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../src/s5run/value_wire.cpp")
list (APPEND TEST_S5RUN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../../src/s5run/s5run_protocol.cpp")
list (APPEND TEST_S5RUN_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/s5run/include/")
list (APPEND TEST_S5RUN_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/s5run/")

include_directories(${TEST_S5RUN_INCLUDE_DIRS})

foreach(_TEST ${ALL_TESTS})
    get_filename_component(TEST_NAME ${_TEST} NAME_WE)
    add_executable(${TEST_NAME}
        ${_TEST}
        ${TEST_S5RUN_SOURCES}
    )
    target_link_libraries(${TEST_NAME} -lpthread -lssl -lcrypto ${WSJCPP_LIBRARIES})

    add_test(
      NAME ${TEST_NAME}
      COMMAND $<TARGET_FILE:${TEST_NAME}> $<TARGET_FILE:s5run-server>
    )
    message(${CMAKE_CURRENT_BINARY_DIR}/tests)
    set_target_properties (${TEST_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY tests)
endforeach()
