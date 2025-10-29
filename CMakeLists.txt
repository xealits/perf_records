cmake_minimum_required(VERSION 3.28..4)

project(perf_counters
  LANGUAGES CXX)

# only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

file(WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")
# run cmake --compile-no-warning-as-error to avoid this
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(perf_counters INTERFACE perf_counters.hpp)
target_include_directories(perf_counters INTERFACE ${PROJECT_SOURCE_DIR}/)

add_executable(example example.cpp)
target_link_libraries(example PRIVATE perf_counters)

