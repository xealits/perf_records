#include <chrono>
#include <iostream>
#include <thread>

#include "perf_counters.hpp"
#include "records.hpp"

void code_under_test(void) {
  for (unsigned ind = 0; ind < 5; ind++) {
    std::cout << ind << '\n';
  }

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
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

  auto thr = std::thread([](void) {
    PerfCounter count_th{};
    count_th.add_counter("cpu-cycles");
    count_th.add_counter("topdown-fe-bound");
    count_th.add_counter("topdown-retiring");

    count_th.start_count();
    code_under_test();
    code_under_test();
    count_th.stop_count();

    auto res = count_th.get_data();
    auto bench = translate_perf_counters("testing thread", res);
    std::cout << bench << "\n";
  });
  thr.join();

  count1.start_count();
  code_under_test();
  count1.stop_count();

  auto res = count1.get_data();
  auto bench = translate_perf_counters("testing X", res);
  std::cout << bench << "\n";

  return 0;
}
