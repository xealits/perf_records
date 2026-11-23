#include <iostream>
#include <thread>

#include "events_map.hpp"
#include "perf_counters.hpp"
// #include "known_events_gracemont.hpp"

using PerfCounter = perf_counters::PerfCounter<perf_events_basic>;

struct AppStats {
  unsigned long long counter{0};
};

auto code_under_test(void) {
  AppStats stats;
  for (unsigned ind = 0; ind < 5; ind++) {
    std::cout << ind << '\n';
    stats.counter++;
  }

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);

  return stats;
}

int main() {
  PerfCounter count1{};
  count1.add_counter("PERF_COUNT_HW_INSTRUCTIONS");
  count1.add_counter("PERF_COUNT_HW_CPU_CYCLES");
  // count1.add_counter("cpu-cycles");
  // count1.add_counter("cache-references");
  // count1.add_counter("cache-misses");
  // count1.add_counter("CPU_CLK_UNHALTED.CORE_P"); //("cpu-cycles");
  // count1.add_counter("LONGEST_LAT_CACHE.REFERENCE"); //("cache-references");

  // count1.add_group(
  //     {"topdown-fe-bound", "topdown-retiring"});
  // count1.add_group(
  //    { "topdown-fe-bound",
  //      "topdown-be-bound",
  //      "topdown-retiring"});
  // count1.add_counter("PERF_COUNT_HW_STALLED_CYCLES_BACKEND");

  // count1.add_group(
  //     { "TOPDOWN_FE_BOUND.ALL",
  //       "TOPDOWN_BE_BOUND.ALL",
  //       "UOPS_RETIRED.ALL"
  //     });

  // decltype(translate_perf_counters("n", PerfCounter{}.get_data())) bench_th;
  PerfCounter count_th{};
  count_th.add_counter("PERF_COUNT_HW_INSTRUCTIONS");
  count_th.add_counter("PERF_COUNT_HW_CPU_CYCLES");
  // count_th.add_counter("CPU_CLK_UNHALTED.CORE_P"); //("cpu-cycles");
  // count_th.add_counter("TOPDOWN_FE_BOUND.ALL");
  // count_th.add_counter("UOPS_RETIRED.ALL");

  auto thr = std::thread([&count_th](void) {
    count_th.start_count();
    code_under_test();
    code_under_test();
    count_th.stop_count();

    // auto res = count_th.get_data();
    // bench_th = translate_perf_counters("thr1", res);
  });
  thr.join();
  // std::cout << bench_th << "\n";

  count1.start_count();
  auto stats_main = code_under_test();
  count1.stop_count();

  // auto res = count1.get_data();
  // auto bench = translate_perf_counters("thrMain", res);
  auto bench = count1.to_records();
  auto bench_th = count_th.to_records();
  // std::cout << bench << "\n\n"; // TODO: no << in RecordsStd yet

  decltype(bench) analysis_bench{"analysisID"};
  analysis_bench.value = 1;
  analysis_bench.conditions.push_back({.name = "n_thr", .value = 2});
  analysis_bench.conditions.push_back({.name = "instr", .value = 1});
  analysis_bench.sub_records.push_back(
      {.name = "app_count", .value = stats_main.counter});
  analysis_bench.sub_records.push_back(bench);
  analysis_bench.sub_records.push_back(bench_th);

  std::cout << analysis_bench.html() << "\n";

  return 0;
}
