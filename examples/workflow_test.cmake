message(STATUS "> workflow_test.cmake is running a typical complete workflow session")
message(STATUS "> workflow_test.cmake given: CMD=${CMD} OUT=${OUT}")

execute_process(
    COMMAND ${CMD}
    OUTPUT_VARIABLE example_out
    ERROR_VARIABLE example_err
    RESULT_VARIABLE res
)

if(NOT res EQUAL 0)
    message(FATAL_ERROR "Command ${CMD} failed with ${res}:\n${example_err}")
endif()

string(CONFIGURE [[
---
title: A study a log
date: November 1, 2025
---

Some log testing this code
```cpp

Record<PerfCounter::CounterVal_t> translate_perf_counters(
    RecordString_t rec_name, const PerfCounter::AllCountersData& data) {
  Record<PerfCounter::CounterVal_t> bench(rec_name);
  bench.pid = data.pid;
  bench.cpu = data.cpu;
  for (const auto& rec : data.counters) {
    bench.subrecs.push_back(translate_perf_record(rec.second));
  }
  return bench;
}
```

And looking at (not really) its code:
```assembly
.LVL3:
  .loc 1 17 12 view .LVU10
  .p2align 6

; kudos GitHub does highlight assembly syntax

.L6:
.LVL4:
.LBB4:
.LBB5:
  .loc 1 21 7 is_stmt 1 view .LVU11
  movl  (%rax), %edx
  .loc 1 21 20 is_stmt 0 view .LVU12
  movl  %edx, %ecx
  andl  $1771, %ecx
  .loc 1 21 12 view .LVU13
  addl  %ecx, %edx
  movl  %edx, (%rax)
  .loc 1 19 5 is_stmt 1 discriminator 3 view .LVU14
  addq  $4, %rax
.LVL5:
.L5:
  .loc 1 19 23 discriminator 1 view .LVU15
  leaq  AData(%rip), %rdx
  leaq  1024000000(%rdx), %rdx
  cmpq  %rdx, %rax
  jne .L6
```

Using Intel perfmon description of events, and RAW PMU events in `perf_event`:
```
./dump_perfmon.py intel_perfmon/ GenuineIntel-6-BE perf_events_raw_gracemont include/perf_records/events_map_raw.hpp
```


# testing an idea 1

Metrics with a basic config without the app counter:
${example_out}
]] session_log)

file(WRITE "${OUT}" "${session_log}")

# post-processing of the log:
# convert to HTML to parse the data
set(PANDOC_ARGS
    --from markdown-markdown_in_html_blocks+raw_html
    --citeproc
    --filter pandoc-csv2table
    --css "${SRC_DIR}/pandoc_default_allow_table_overflow.css"
)

set(PANDOC_LOG_HTML "${OUT}.html")

execute_process(
    COMMAND pandoc ${PANDOC_ARGS} --standalone "${OUT}"
    OUTPUT_FILE "${PANDOC_LOG_HTML}"
    ERROR_VARIABLE pandoc_err
    RESULT_VARIABLE res
)

if(NOT res EQUAL 0)
    message(FATAL_ERROR "Pandoc failed with ${res}:\n${pandoc_err}")
endif()

message(NOTICE "Python ${SRC_DIR}/log_to_csv.py")

# parse the HTML to CSV
execute_process(
    COMMAND python "${SRC_DIR}/log_to_csv.py" "${PANDOC_LOG_HTML}"
    OUTPUT_VARIABLE csv_out
    ERROR_VARIABLE csv_err
    RESULT_VARIABLE res
)

if(NOT res EQUAL 0)
    message(FATAL_ERROR "log_to_csv failed with ${res}:\n${csv_err}")
endif()

# the result of post-processing: a section with CSV
string(CONFIGURE [[

# Render HTML with Pandoc

Add the `--from` option to make Pandoc correctly treat custom tags as
not a part of markdown:
```bash
$ pandoc --from markdown-markdown_in_html_blocks+raw_html \
    --standalone log_example.md  > log_example.html
```

HTML can be parsed on its own:
```Python
>>> from bs4 import BeautifulSoup
>>> example_s = "..."
>>> soup = BeautifulSoup(example_s, 'html.parser')
>>> soup.select_one("var[data-name=analysisID] > data").contents
['1']
```

And parse it to CSV:
```{.table caption="The data of the whole session" header="yes"}
${csv_out}
```
]] SECTION_WITH_CSV)

file(APPEND "${OUT}" "${SECTION_WITH_CSV}")

# final HTML
set(PANDOC_FINAL_OUT "${OUT}_final.html")

execute_process(
    COMMAND pandoc ${PANDOC_ARGS} --standalone "${OUT}"
    OUTPUT_FILE "${PANDOC_FINAL_OUT}"
    ERROR_VARIABLE pandoc_err
    RESULT_VARIABLE res
)

if(NOT res EQUAL 0)
    message(FATAL_ERROR "Final Pandoc failed with ${res}:\n${pandoc_err}")
endif()
