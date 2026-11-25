cmake_minimum_required(VERSION 3.28..4)
project(perf_records LANGUAGES CXX)

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
add_library(perf_records_warnings INTERFACE)
add_library(perf_records::warnings ALIAS perf_records_warnings)

if(perf_records_WARNING_FLAGS)
  target_compile_options(perf_records_warnings INTERFACE ${perf_records_WARNING_FLAGS})
endif()

message(STATUS "${PROJECT_NAME} > in directory ${CMAKE_CURRENT_LIST_DIR}")

# main library targets
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/include/perf_records")

if(PROJECT_IS_TOP_LEVEL)
  message(STATUS "${PROJECT_NAME} > adding examples directory ${CMAKE_CURRENT_LIST_DIR}/examples")
  add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/examples")
endif()