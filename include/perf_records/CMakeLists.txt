message(STATUS "${PROJECT_NAME} > adding targets in directory ${CMAKE_CURRENT_LIST_DIR}")

foreach(hdr perf_counters records counters events_map)
  set(target_name perf_records_${hdr}) # internal hard-to-collide name
  add_library(${target_name} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/${hdr}.hpp")
  set_target_properties(${target_name} PROPERTIES EXPORT_NAME ${hdr})
  target_include_directories(${target_name} INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>"
    "$<INSTALL_INTERFACE:include>"
  )
  add_library(perf_records::${hdr} ALIAS ${target_name})
endforeach()