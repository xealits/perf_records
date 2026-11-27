message(STATUS "${PROJECT_NAME} > adding example targets in ${CMAKE_CURRENT_LIST_DIR}/")

add_executable(example example.cpp)
target_link_libraries(example PRIVATE perf_records::perf_counters perf_records::records perf_records::compile_settings)

add_executable(example_perf_basic example_perf_basic.cpp)
target_link_libraries(example_perf_basic PRIVATE perf_records::perf_counters perf_records::records perf_records::compile_settings)

if(perf_records_dump_perfmon STREQUAL "GenuineIntel-6-BE")
    message(STATUS "${PROJECT_NAME} > adding Gracemont GenuineIntel-6-BE example")
    add_executable(example_perf_gracemont example_perf_gracemont.cpp)
    target_link_libraries(example_perf_gracemont PRIVATE perf_records::perf_counters perf_records::records perf_records::compile_settings)
endif()

include(CTest)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
message(STATUS "${PROJECT_NAME} > adding CTest workflow_session_example")
add_test(
    NAME workflow_session_example
    COMMAND ${CMAKE_COMMAND}
    -DSRC_DIR=${CMAKE_CURRENT_LIST_DIR}
    -DCMD=$<TARGET_FILE:example>
    -DOUT=${CMAKE_CURRENT_BINARY_DIR}/workflow_session_example_log.markdown
    -P ${CMAKE_CURRENT_LIST_DIR}/workflow_test.cmake
)
endif()