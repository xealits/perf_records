cmake_minimum_required(VERSION 3.28..4)
project(perf_records LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

# the cxx_standard if it is not set by the project user
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

include(cmake/GitIgnoreBuilds.cmake)

# project config settings
# override these variable to change the config

# compile warnings as error
if(NOT DEFINED perf_records_WERROR)
  set(perf_records_WERROR ${PROJECT_IS_TOP_LEVEL})
endif()
set(CMAKE_COMPILE_WARNING_AS_ERROR ${perf_records_WERROR})

# default warnings list for the development
if(NOT DEFINED perf_records_WARNING_FLAGS)
  if(PROJECT_IS_TOP_LEVEL)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/DefaultWarnings.cmake)
    perf_records_set_default_warnings(perf_records_WARNING_FLAGS)
  else()
    set(perf_records_WARNING_FLAGS "")
  endif()
endif()

# helper targets for the project build
add_library(perf_records_warnnings INTERFACE)
add_library(perf_records::warnings ALIAS perf_records_warnnings)

if(perf_records_WARNING_FLAGS)
  target_compile_options(perf_records_warnnings INTERFACE ${perf_records_WARNING_FLAGS})
endif()

message(STATUS "${PROJECT_NAME} > in directory ${CMAKE_CURRENT_LIST_DIR}")

add_subdirectory("include/perf_records")

message(STATUS "${PROJECT_NAME} > adding example targets in directory ${CMAKE_CURRENT_LIST_DIR}")

add_executable(example example.cpp)
target_link_libraries(example PRIVATE perf_records::perf_counters perf_records::records)

add_executable(example_perf example_perf.cpp)
target_link_libraries(example_perf PRIVATE perf_records::perf_counters perf_records::records)

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