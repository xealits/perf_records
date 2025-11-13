// set up perf
#include <errno.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>  // pid_t gettid(void)

#include <cstring>
// #include <linux/hw_breakpoint.h>
#include <asm/unistd.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

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

static std::map<CounterName_t, PerfEventDesc> known_events_map = {
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

class PerfCounter {
 public:
  using CounterFD_t = decltype(syscall(__NR_perf_event_open, 0, 0, 0, -1, 0));
  using CounterId_t = uint64_t;
  using CounterVal_t = uint64_t;
  using CounterRunning_t = uint64_t;
  using CounterEnabled_t = uint64_t;

  struct CounterDesc {
    CounterId_t c_id{};
    CounterVal_t c_val{0};
    CounterName_t c_name;
    PerfEventAttr_config_t c_config{};
    PerfEventAttr_type_t c_type{};
  };

  struct AllCountersData {
    pid_t pid{0};
    int cpu{-1};
    std::map<CounterId_t, CounterDesc> counters;
  };

 private:
  bool counter_is_running{false};
  // int group_fd{-1};  // this fd will be the counter group fd
  std::vector<CounterFD_t> counters_fds;
  // fd_cycles, fd_backend;
  pid_t pid_{0};
  int cpu_{-1};

  std::map<CounterId_t, CounterDesc> counters_;
  std::map<int, std::vector<CounterId_t>> counter_groups_;

  static constexpr unsigned perf_data_buffer_size_ = 4096;
  char buf[perf_data_buffer_size_] = {};

  // I need to handle groups better:
  // report when events are dropped because the group does not fit in counters
  // make a better UI to add individual separate events or groups
  struct read_format_group {
    uint64_t nr;
    uint64_t time_enabled;  // an event group is scheduled atomically
    uint64_t time_running;
    struct {
      uint64_t value;
      uint64_t id;
    }* values;
  };

  struct read_format_single {
    uint64_t value;
    uint64_t time_enabled;
    uint64_t time_running;
    uint64_t id;
  };

  static_assert(sizeof(read_format_single) < sizeof(buf));

  static constexpr auto basic_read_format = PERF_FORMAT_ID |
                                            PERF_FORMAT_TOTAL_TIME_ENABLED |
                                            PERF_FORMAT_TOTAL_TIME_RUNNING;

  // struct read_format* rf = (struct read_format*) buf;

  static CounterFD_t perf_event_open(struct perf_event_attr* hw_event,
                                     pid_t pid, int cpu, int group_fd,
                                     unsigned long flags) {
    CounterFD_t ret =
        syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
    // ret = sys_perf_event_open( hw_event, pid, cpu,
    //                group_fd, flags);

    return ret;
  }

  void read_counters(void) {
    std::vector<CounterDesc> res;
    stop_count();

    for (const auto& [group_fd, cids] : counter_groups_) {
      auto nread = read(group_fd, &(buf[0]), sizeof(buf));
      if (nread <= 0) {
        std::cout << "PerfCounter::read_counters read no data " << nread
                  << " returning\n";
        return;
      }

      if (cids.size() == 1) {
        struct read_format_single* pdata =
            reinterpret_cast<struct read_format_single*>(buf);

        std::cout << "PerfCounter::read_counters counter ran as " << std::dec
                  << pdata->time_running << " / " << pdata->time_enabled
                  << "\n";

        //
        auto counter_id = cids.at(0);
        if (counter_id != pdata->id) {
          throw std::runtime_error(
              "PerfCounter::read_counters read unexpected counter id");
        }

        auto& counter = counters_.at(counter_id);
        counter.c_val = pdata->value;
      }

      else {
        struct read_format_group* pdata =
            reinterpret_cast<struct read_format_group*>(buf);

        // FIXME this is not exactly correct:
        if ((sizeof(pdata->nr) + pdata->nr * sizeof(pdata->values[0])) >
            sizeof(buf)) {
          throw std::runtime_error(
              "PerfCounter::read_counters got more data than the internal "
              "buffer");
        }

        std::cout << "PerfCounter::read_counters group ran as " << std::dec
                  << pdata->time_running << " / " << pdata->time_enabled
                  << "\n";

        for (uint64_t count_i = 0; count_i < pdata->nr; count_i++) {
          const auto& counter_res = pdata->values[count_i];
          const CounterId_t& counter_id = counter_res.id;
          const CounterVal_t& counter_value = counter_res.value;
          std::cout << "PerfCounter::read_counters " << counter_id << " "
                    << counter_value << "\n";

          //
          auto& counter = counters_.at(counter_id);
          counter.c_val = counter_value;
          // res.push_back(counter);
        }
      }
    }
  }

 public:
  PerfCounter(void) : pid_{gettid()} {
    std::cerr << "PerfCounter constructed for tid " << pid_ << "\n";
  };
  PerfCounter(pid_t pid) : pid_{pid} {
    std::cerr << "PerfCounter constructed for given tid " << pid_ << "\n";
  };
  PerfCounter(const PerfCounter& ref) = delete;
  PerfCounter(PerfCounter&& ref) = delete;
  PerfCounter& operator=(const PerfCounter&) = delete;
  PerfCounter& operator=(PerfCounter&&) = delete;

  ~PerfCounter() {
    for (auto counter_fd : counters_fds) {
      close(counter_fd);
    }
  };

  auto add_counter(PerfEventAttr_config_t counter_config /* counter name */,
                   const CounterName_t& counter_name_s,
                   PerfEventAttr_type_t counter_type = PERF_TYPE_HARDWARE,
                   std::optional<int> group_fd_opt = std::nullopt)

  {
    std::cout << "PerfCounter::add_counter config=" << counter_config
              << " type=" << counter_type << "\n";

    struct perf_event_attr pe{};
    memset(&pe, 0, sizeof(struct perf_event_attr));
    // pe.type     = UNCORE_IMC_TYPE;
    // pe.config   = UNCORE_IMC_EVENT_READ;
    pe.type = counter_type;
    pe.config = counter_config;  // PERF_COUNT_HW_INSTRUCTIONS
    pe.size = sizeof(struct perf_event_attr);
    // pe.size     = sizeof(struct perf_event_attr);
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    // pe.exclude_kernel = 0;
    // pe.exclude_hv = 0;
    pe.exclude_user = 0;

    int group_fd = -1;
    if (group_fd_opt.has_value()) {
      if (*group_fd_opt != -1 &&
          counter_groups_.find(*group_fd_opt) == counter_groups_.end()) {
        throw std::runtime_error(
            "PerfCounter::add_counter given group fd has not been created");
      }

      pe.read_format = PERF_FORMAT_GROUP | basic_read_format;
      group_fd = *group_fd_opt;
    } else {
      pe.read_format = basic_read_format;
    }

    CounterFD_t new_counter_fd = perf_event_open(&pe, pid_, cpu_, group_fd, 0);
    if (new_counter_fd == -1) {
      std::cerr << "PerfCounter::add_counter perf_event_open error " << std::hex
                << pe.config << std::dec << errno << std::strerror(errno)
                << "\n";
      exit(EXIT_FAILURE);
    }
    counters_fds.push_back(new_counter_fd);

    CounterId_t new_id{};
    ioctl(new_counter_fd, PERF_EVENT_IOC_ID, &new_id);

    if (counters_.find(new_id) != counters_.end()) {
      throw std::runtime_error(
          "PerfCounter::add_counter somehow got a used id");
    }

    // counter_ids.push_back(new_id);
    // counter_types.push_back(counter_type);
    // counter_configs.push_back(counter_config);
    // counter_names.push_back(counter_name_s);
    counters_[new_id] = {.c_id = new_id,
                         .c_name = counter_name_s,
                         .c_config = counter_config,
                         .c_type = counter_type};

    if (group_fd == -1) {
      counter_groups_[new_counter_fd] = {new_id};
    }

    else {
      counter_groups_.at(group_fd).push_back(new_id);
    }

    return new_counter_fd;
  }

  CounterFD_t add_counter(const CounterName_t& known_event_s,
                          std::optional<int> group_fd = std::nullopt) {
    if (known_events_map.find(known_event_s) == known_events_map.end()) {
      std::cerr << "PerfCounter::add_counter could not find " << known_event_s
                << " in known events\n";

      throw std::runtime_error("PerfCounter::add_counter could not find " +
                               known_event_s + " in known events");

      return -1;
    }

    if (group_fd.has_value()) {
      std::cerr << "PerfCounter::add_counter adding event " << known_event_s
                << " from known events to group " << *group_fd << "\n";
    }

    else {
      std::cerr << "PerfCounter::add_counter adding a single event "
                << known_event_s << "\n";
    }

    auto event_desc = known_events_map.at(known_event_s);
    auto new_counter_fd = add_counter(event_desc.perf_config, known_event_s,
                                      event_desc.perf_type, group_fd);
    return new_counter_fd;
  }

  CounterFD_t add_group(const std::vector<CounterName_t>& names) {
    std::cerr << "PerfCounter::add_counter adding a group of " << names.size()
              << " events\n";
    auto group_fd = add_counter(
        names.at(0),
        {-1});  // first counter in the group has to have group fd -1
    for (size_t ind = 1; ind < names.size(); ++ind) {
      add_counter(names[ind], {group_fd});
    }
    return group_fd;
  }

  void start_count(void) {
    for (const auto& [group_fd, cids] : counter_groups_) {
      if (cids.size() > 1) {
        ioctl(group_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
        ioctl(group_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
      }

      else {
        // this enables only the very first event:
        ioctl(group_fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(group_fd, PERF_EVENT_IOC_ENABLE, 0);
      }
    }

    counter_is_running = true;
  }

  void stop_count(void) {
    if (!counter_is_running) {
      return;
    }

    for (const auto& [group_fd, cids] : counter_groups_) {
      if (cids.size() > 1) {
        ioctl(group_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
      }

      else {
        // this enables only the very first event:
        ioctl(group_fd, PERF_EVENT_IOC_DISABLE, 0);
      }
    }

    counter_is_running = false;
  }

  // return data to the user
  AllCountersData get_data(void) {
    read_counters();
    return {.pid = pid_, .cpu = cpu_, .counters = counters_};
  }
};
