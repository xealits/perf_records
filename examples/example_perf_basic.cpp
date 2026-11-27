#include <iostream>
#include <thread>

#include "perf_records/events_map.hpp"
#include "perf_records/perf_counters.hpp"

using PerfCounter = perf_records::PerfCounter<perf_records::perf_events_basic>;

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
  std::this_thread::sleep_for(0.1s);

  return stats;
}

int main() {
  PerfCounter count1{};
  count1.add_counter("PERF_COUNT_HW_INSTRUCTIONS");
  count1.add_counter("PERF_COUNT_HW_CPU_CYCLES");
  // count1.add_counter("cpu-cycles");
  // count1.add_counter("cache-references");
  // count1.add_counter("cache-misses");

  PerfCounter count_th{};
  count_th.add_counter("PERF_COUNT_HW_INSTRUCTIONS");
  count_th.add_counter("PERF_COUNT_HW_CPU_CYCLES");
  

  auto thr = std::thread([&count_th](void) {
    count_th.start_count();
    code_under_test();
    code_under_test();
    count_th.stop_count();
  });
  thr.join();

  count1.start_count();
  auto stats_main = code_under_test();
  count1.stop_count();

  auto bench = count1.to_records();
  auto bench_th = count_th.to_records();

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
