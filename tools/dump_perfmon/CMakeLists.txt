cmake_minimum_required(VERSION 3.16)
project(gen_pmu_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# TODO: check this
## Cache downloaded archives outside the build folder so clean rebuilds don't re-download
#if(NOT DEFINED FETCHCONTENT_BASE_DIR)
#    set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.build-tools/deps")
#endif()

include(FetchContent)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
)
FetchContent_MakeAvailable(json)

add_executable(dump_perfmon dump_perfmon.cpp)
target_link_libraries(dump_perfmon PRIVATE nlohmann_json::nlohmann_json)