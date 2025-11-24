#pragma once
#include <linux/perf_event.h>

using PerfEventAttr_config_t = decltype(perf_event_attr{}.config);
using PerfEventAttr_type_t = decltype(perf_event_attr{}.type);
using CounterName_t = std::string;

struct PerfEventDesc {
  PerfEventAttr_config_t perf_config{};
  PerfEventAttr_type_t perf_type{};
};

// $ cat /sys/bus/event_source/devices/cpu/format/event
// config:0-7
constexpr PerfEventAttr_config_t RawEventConfig(unsigned event,
                                                unsigned umask = 0) {
  // PerfEventAttr_config_t config = (event << 8) | umask;
  PerfEventAttr_config_t config = (umask << 8) | event;
  return config;
}

static const std::map<CounterName_t, PerfEventDesc> known_events_map_example = {
  {"PERF_COUNT_HW_INSTRUCTIONS",
   {.perf_config = PERF_COUNT_HW_INSTRUCTIONS,
    .perf_type = PERF_TYPE_HARDWARE}},
  {"PERF_COUNT_HW_CPU_CYCLES",
   {.perf_config = PERF_COUNT_HW_CPU_CYCLES,
    .perf_type = PERF_TYPE_HARDWARE}},

  {"topdown-bad-spec",
   {.perf_config = RawEventConfig(0x73, 0x6), .perf_type = PERF_TYPE_RAW}},
  {"topdown-be-bound",
   {.perf_config = RawEventConfig(0x74, 0), .perf_type = PERF_TYPE_RAW}},
  {"topdown-fe-bound",
   {.perf_config = RawEventConfig(0x71, 0), .perf_type = PERF_TYPE_RAW}},
  {"topdown-retiring",
   {.perf_config = RawEventConfig(0xc2, 0), .perf_type = PERF_TYPE_RAW}},

  {"cpu-cycles",
   {.perf_config = RawEventConfig(0x3c), .perf_type = PERF_TYPE_RAW}},
  {"cache-references",
   {.perf_config = RawEventConfig(0x2e, 0x4f), .perf_type = PERF_TYPE_RAW}},
  {"cache-misses",
   {.perf_config = RawEventConfig(0x2e, 0x41), .perf_type = PERF_TYPE_RAW}},
};

using PerfEventMap_t = decltype(known_events_map_example);