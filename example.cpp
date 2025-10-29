#include <chrono>
#include <iostream>
#include <thread>

#include "perf_counters.hpp"
#include "records.hpp"

void code_under_test(void) {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
}

Record<PerfCounter::CounterVal_t> translate_perf_record(
    PerfCounter::CounterDesc perf_count) {
  return {.column_name = perf_count.c_name, .value = perf_count.c_val};
}

int main() {
  PerfCounter count1{};
  count1.add_counter(PERF_COUNT_HW_INSTRUCTIONS);
  count1.add_counter(PERF_COUNT_HW_CPU_CYCLES);
  // count1.add_counter(PERF_COUNT_HW_STALLED_CYCLES_BACKEND);

  count1.start_count();
  code_under_test();
  count1.stop_count();

  auto res = count1.read_counters();

  Record<PerfCounter::CounterVal_t> bench("testing X");
  for (const auto& rec : res) {
    bench.subrecs.push_back(translate_perf_record(rec));
    // std::cout << rec.c_id << " : " << rec.c_name << " = " << rec.c_val <<
    // "\n";
  }

  std::cout << bench << "\n";

  return 0;
}
