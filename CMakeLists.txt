cmake_minimum_required(VERSION 3.28..4)
project(perf_records LANGUAGES CXX)

# the cxx_standard if it is not set by the project user
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

if(PROJECT_IS_TOP_LEVEL)
  include(cmake/GitIgnoreBuilds.cmake)
endif()

# project config settings
# override these variable to change the config

set(perf_records_NAMESPACE_NAME "" CACHE STRING "Set the namespace name for the library to avoid name collisions")
if(perf_records_NAMESPACE_NAME)
  set(perf_records_NAMESPACE_NAME_DEFINE PERF_RECORDS_NAMESPACE_NAME="${perf_records_NAMESPACE_NAME}")
else()
  set(perf_records_NAMESPACE_NAME_DEFINE "")
endif()

if(NOT DEFINED perf_records_WERROR)
  set(perf_records_WERROR ${PROJECT_IS_TOP_LEVEL})
endif()

# call dump_perfmon.py to create a header with a map of RAW perf events from Intel's perfmon JSON maps
set(perf_records_dump_perfmon "" CACHE STRING "Dump Intel PMU events for the given architecture in perfmon jsons (e.g. GenuineIntel-6-BE)")
if(perf_records_dump_perfmon)
  message(STATUS "${PROJECT_NAME} > dumping perfmon info for CPUID Family-Model: ${perf_records_dump_perfmon}")
  #set(DUMP_PERFMON_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/dump_perfmon.py")
  #set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${DUMP_PERFMON_SCRIPT}")
  #set(GENERATED_HEADER "${CMAKE_CURRENT_LIST_DIR}/include/perf_records/events_map_raw.hpp")
  set(GENERATED_HEADER "${CMAKE_CURRENT_LIST_DIR}/include/perf_records/events_map_raw_testing.hpp")
  get_filename_component(GENERATED_TARGET "${GENERATED_HEADER}" NAME_WE)
  message(STATUS "${PROJECT_NAME} > got ${GENERATED_TARGET}")

  #find_package(Python3 REQUIRED COMPONENTS Interpreter)
  #execute_process(
  #  COMMAND
  #  "${Python3_EXECUTABLE}" "${DUMP_PERFMON_SCRIPT}"
  #  "${CMAKE_CURRENT_LIST_DIR}/intel_perfmon/"
  #  "${perf_records_dump_perfmon}" perf_events_raw
  #  "${GENERATED_HEADER}"
  #  WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
  #  RESULT_VARIABLE dump_perfmon_result
  #)

  #if(NOT dump_perfmon_result EQUAL 0)
  #  message(FATAL "${PROJECT_NAME} > dump_perfmon.py ${perf_records_dump_perfmon} raw_events > ${GENERATED_HEADER} failed")
  #endif()

  include(tools/BuildTimeTools.cmake)
  perf_records_setup_dump_perfmon("intel_perfmon/" "${perf_records_dump_perfmon}" "perf_events_raw" "${GENERATED_HEADER}" "${GENERATED_TARGET}")
endif()

# compile warnings as error
if(NOT DEFINED perf_records_WERROR)
  set(perf_records_WERROR ${PROJECT_IS_TOP_LEVEL})
endif()
set(CMAKE_COMPILE_WARNING_AS_ERROR ${perf_records_WERROR})
message(STATUS "${PROJECT_NAME} > CMAKE_COMPILE_WARNING_AS_ERROR ${CMAKE_COMPILE_WARNING_AS_ERROR}")

# default warnings list for the development
if(NOT DEFINED perf_records_WARNING_FLAGS)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/DefaultWarnings.cmake)
    perf_records_set_default_warnings(perf_records_WARNING_FLAGS)
  else()
    set(perf_records_WARNING_FLAGS "")
  endif()
endif()
message(STATUS "${PROJECT_NAME} > perf_records_WARNING_FLAGS ${perf_records_WARNING_FLAGS}")

# helper targets for the project build
add_library(perf_records_compile_settings INTERFACE)
add_library(perf_records::compile_settings ALIAS perf_records_compile_settings)

if(perf_records_WARNING_FLAGS)
  message(STATUS "${PROJECT_NAME} > setting warning flags: ${perf_records_WARNING_FLAGS}")
  target_compile_options(perf_records_compile_settings INTERFACE ${perf_records_WARNING_FLAGS})
endif()

if(perf_records_NAMESPACE_NAME_DEFINE)
  message(STATUS "${PROJECT_NAME} > setting definitions: ${perf_records_NAMESPACE_NAME_DEFINE}")
  target_compile_definitions(perf_records_compile_settings INTERFACE ${perf_records_NAMESPACE_NAME_DEFINE})
endif()

# main library targets
message(STATUS "${PROJECT_NAME} > in directory ${CMAKE_CURRENT_LIST_DIR} adding include/perf_records")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/include/perf_records")

if(PROJECT_IS_TOP_LEVEL)
  message(STATUS "${PROJECT_NAME} > adding examples directory ${CMAKE_CURRENT_LIST_DIR}/examples")
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/examples")
endif()