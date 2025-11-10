#include <chrono>
#include <iostream>
#include <thread>

#include "perf_counters.hpp"
#include "records.hpp"

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

Record<PerfCounter::CounterVal_t> translate_perf_record(
    PerfCounter::CounterDesc perf_count) {
  return {.column_name = perf_count.c_name, .value = perf_count.c_val};
}

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

int main() {
  PerfCounter count1{};
  // count1.add_counter("PERF_COUNT_HW_INSTRUCTIONS");
  // count1.add_counter("PERF_COUNT_HW_CPU_CYCLES");
  count1.add_counter("cpu-cycles");
  count1.add_counter("cache-references");
  // count1.add_counter("cache-misses");

  // count1.add_group(
  //     {"topdown-fe-bound", "topdown-retiring"});
  count1.add_group(
      {"topdown-fe-bound", "topdown-be-bound", "topdown-retiring"});
  // count1.add_counter("PERF_COUNT_HW_STALLED_CYCLES_BACKEND");

  decltype(translate_perf_counters("n", PerfCounter{}.get_data())) bench_th;
  auto thr = std::thread([&bench_th](void) {
    PerfCounter count_th{};
    count_th.add_counter("cpu-cycles");
    count_th.add_counter("topdown-fe-bound");
    count_th.add_counter("topdown-retiring");

    count_th.start_count();
    code_under_test();
    code_under_test();
    count_th.stop_count();

    auto res = count_th.get_data();
    bench_th = translate_perf_counters("thr1", res);
  });
  thr.join();
  std::cout << bench_th << "\n";

  count1.start_count();
  auto stats_main = code_under_test();
  count1.stop_count();

  auto res = count1.get_data();
  auto bench = translate_perf_counters("thrMain", res);
  std::cout << bench << "\n\n";

  decltype(bench) analysis_bench("analysisID");
  analysis_bench.value = 1;
  analysis_bench.conditions.push_back({.column_name = "n_thr", .value = 2});
  //analysis_bench.conditions.push_back({.column_name = "instr", .value = 1});
  //analysis_bench.subrecs.push_back(
      //{.column_name = "app_count", .value = stats_main.counter});
  analysis_bench.subrecs.push_back(bench);
  analysis_bench.subrecs.push_back(bench_th);

  std::cout << analysis_bench.html() << "\n";

  return 0;
}
