#include <chrono>
#include <iostream>
#include <thread>

#include "counters.hpp"
#include "records.hpp"

using Clock = std::chrono::steady_clock;

using timers_t = perf_records::CountersType<
    perf_records::EmptyT, std::chrono::microseconds,
    {.input_proc =
         [](const auto& duration) {
           return std::chrono::duration_cast<std::chrono::milliseconds>(
               duration);
         },
     .current_count_getter = [](void) { return Clock::now(); }}>;

using MainTimers = perf_records::Counters<"main", timers_t>;
using ThreadTimers = perf_records::Counters<"thread", timers_t>;

template <typename CountersT>
auto translate_counters()

{
  Record<decltype(CountersT::data)> bench{CountersT::name};
  // bench.pid = data.pid;
  // bench.cpu = data.cpu;
  // for (const auto& rec : data.counters) {
  //   bench.subrecs.push_back(translate_perf_record(rec.second));
  // }
  bench.value = CountersT::data;
  return bench;
}

struct AppStats {
  unsigned long long counter{0};
};

AppStats code_under_test(void) {
  AppStats stats;
  for (unsigned ind = 0; ind < 5; ind++) {
    std::cerr << ind << '\n';
    stats.counter++;
  }

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(0.1s);

  return stats;
}

int main() {
  std::chrono::time_point ts_main_start = Clock::now();

  auto thr = std::thread([&ts_main_start](void) {
    ThreadTimers::counter<"setup">::set(Clock::now() - ts_main_start);
    auto test_timing = ThreadTimers::counter<"test_full">::ScopeCounter();
    auto also_timing = ThreadTimers::counter<"also_full">::ScopeCounter();
    code_under_test();
    {
      auto test_timing = ThreadTimers::counter<"test_full">::counter<
          "second_part">::ScopeCounter();
      code_under_test();
    }
  });

  std::chrono::time_point ts_setup_done = Clock::now();
  MainTimers::counter<"setup">::set(ts_setup_done - ts_main_start);

  auto stats_main = code_under_test();
  MainTimers::counter<"test_done">::set(Clock::now() - ts_setup_done);

  thr.join();
  MainTimers::counter<"thread_joined">::set(Clock::now() - ts_setup_done);

  auto timing_main = MainTimers::nametree_to_record_std<
      perf_records::NameTreeNode<"setup">,
      perf_records::NameTreeNode<"test_done">,
      perf_records::NameTreeNode<"thread_joined">>();
  //timing_main.print();

  auto timing_thread = ThreadTimers::nametree_to_record_std<
      perf_records::NameTreeNode<"setup">,
      perf_records::NameTreeNode<"test_full",
                                 perf_records::NameTreeNode<"second_part">>,
      // perf_records::NameTreeNode<"test_full">, // no need for this one
      perf_records::NameTreeNode<"also_full">>();
  //timing_thread.print();

  decltype(timing_main) analysis_bench{"analysisID"};
  analysis_bench.sub_records.push_back(timing_main);
  analysis_bench.sub_records.push_back(timing_thread);

  // TODO: multiple-type records
  std::cerr << "stats_main: " << stats_main.counter << "\n";

  std::cout << analysis_bench.html() << "\n";

  return 0;
}
