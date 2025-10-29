#include <chrono>
#include <iostream>
#include <thread>

#include "perf_counters.hpp"

void code_under_test(void) {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
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
  for (const auto& rec : res) {
    std::cout << rec.c_id << " = " << rec.c_val << "\n";
  }

  return 0;
}
