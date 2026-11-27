function(perf_records_setup_dump_perfmon PATH_TO_PERFMON_DIR PERFMON_FAMILY_MODEL MAP_NAME GEN_HEADER_PATH GEN_TARGET)
message(STATUS "${PROJECT_NAME} > BuildTimeTools perf_records_setup_dump_perfmon for ${PERFMON_FAMILY_MODEL} in ${GEN_HEADER_PATH}")

include(ExternalProject)

# the build tools are kept in an isolated Release cache
set(TOOLS_CACHE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.build-tools/dump_perfmon")
set(TOOL_BIN_DIR    "${TOOLS_CACHE_DIR}/bin")
set(GEN_PMU_EXE     "${TOOL_BIN_DIR}/dump_perfmon${CMAKE_EXECUTABLE_SUFFIX}")

ExternalProject_Add(pmu_tool_proj
    PREFIX     "${TOOLS_CACHE_DIR}"
    SOURCE_DIR "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/dump_perfmon"
    BINARY_DIR "${TOOLS_CACHE_DIR}/build"
    STAMP_DIR  "${TOOLS_CACHE_DIR}/stamp"
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${TOOL_BIN_DIR}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${TOOL_BIN_DIR}
        -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${TOOL_BIN_DIR}
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS "${GEN_PMU_EXE}"
)

## helper function to generate a header for any Intel CPUID Family Model
#function(add_perfmon_dump OUT_VAR JSON_FILE HEADER_NAME MAP_NAME)
#    #set(OUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/${HEADER_NAME}")
#    add_custom_command(
#        #OUTPUT "${OUT_HEADER}"
#        COMMAND "${GEN_PMU_EXE}" "${PATH_TO_PERFMON_DIR}" "${MAP_NAME}" "${GEN_HEADER_PATH}"
#        DEPENDS pmu_tool_proj "${JSON_FILE}"
#        COMMENT "Generating ${HEADER_NAME} from ${JSON_FILE}"
#        VERBATIM
#    )
#    set(${OUT_VAR} "${OUT_HEADER}" PARENT_SCOPE)
#endfunction()
#
## 3. Generate your header(s) and attach to your C++ target
#add_perfmon_dump(
#    GRACEMONT_HDR
#    "${CMAKE_CURRENT_SOURCE_DIR}/data/gracemont_core.json"
#    "gracemont_events.hpp"
#    "known_events_gracemont"
#)

add_custom_command(
    OUTPUT "${GEN_HEADER_PATH}" # to trigger the command a target must include the header in its sources
    COMMAND "${GEN_PMU_EXE}" "${PATH_TO_PERFMON_DIR}" "${PERFMON_FAMILY_MODEL}" "${MAP_NAME}" "${GEN_HEADER_PATH}"
    DEPENDS pmu_tool_proj "${PATH_TO_PERFMON_DIR}/mapfile.csv" # TODO: hardcoding the internals: mapfile.csv
    COMMENT "Generating ${GEN_HEADER_PATH} from ${PATH_TO_PERFMON_DIR}"
    VERBATIM
)

# same type INTERFACE target as all the headers include/perf_records
message(STATUS "${PROJECT_NAME} > BuildTimeTools perf_records_setup_dump_perfmon add target header for ${MAP_NAME} in ${GEN_TARGET}")

set(target_name perf_records_${GEN_TARGET})
add_library(${target_name} INTERFACE "${GEN_HEADER_PATH}")
set_target_properties(${target_name} PROPERTIES EXPORT_NAME ${GEN_TARGET})
target_include_directories(${target_name} INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>"
  "$<INSTALL_INTERFACE:include>"
)
set(target_alias "perf_records::${GEN_TARGET}")
add_library(perf_records::${GEN_TARGET} ALIAS ${target_name})

message(STATUS "${PROJECT_NAME} > BuildTimeTools perf_records_setup_dump_perfmon added ${target_name} ${target_alias}")
endfunction()