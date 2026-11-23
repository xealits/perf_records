cmake_minimum_required(VERSION 3.28..4)
project(perf_records LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

# only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

include(cmake/DevSettings.cmake)

# run cmake --compile-no-warning-as-error to avoid this
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

message(STATUS "${PROJECT_NAME} > in directory ${PROJECT_SOURCE_DIR}")

add_library(perf_counters INTERFACE perf_counters.hpp)
target_include_directories(perf_counters INTERFACE ${PROJECT_SOURCE_DIR}/)

add_library(records INTERFACE records.hpp)
target_include_directories(records INTERFACE ${PROJECT_SOURCE_DIR}/)

add_library(counters INTERFACE counters.hpp)
target_include_directories(counters INTERFACE ${PROJECT_SOURCE_DIR}/)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE perf_counters records)

add_executable(example_perf example_perf.cpp)
target_link_libraries(example_perf PRIVATE perf_counters records)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  add_test(
    NAME workflow_session_example
    COMMAND ${CMAKE_COMMAND}
    -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DCMD=$<TARGET_FILE:example>
    -DOUT=${CMAKE_CURRENT_BINARY_DIR}/workflow_session_example_log.markdown
    -P ${CMAKE_CURRENT_SOURCE_DIR}/workflow_test.cmake
  )
endif()