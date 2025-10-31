// set up perf
#include <errno.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <unistd.h>  // pid_t gettid(void)

#include <cstring>
// #include <linux/hw_breakpoint.h>
#include <asm/unistd.h>

#include <map>
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
  std::cout << "RawEventConfig " << std::hex << config << "\n";
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

//! \brief a string for column names, counter names, nicknames etc
struct ShortString {
  char data_[64];
  static constexpr unsigned data_size_ = sizeof(data_) / sizeof(data_[0]);
  operator std::string(void) const { return std::string(data_); }
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
  int group_fd{-1};  // this fd will be the counter group fd
  std::vector<CounterFD_t> counters_fds;
  // fd_cycles, fd_backend;
  pid_t pid_{0};
  int cpu_{-1};

  std::map<CounterId_t, CounterDesc> counters_;

  static constexpr unsigned perf_data_buffer_size_ = 4096;
  char buf[perf_data_buffer_size_] = {};

  // I need to handle groups better:
  // report when events are dropped because the group does not fit in counters
  // make a better UI to add individual separate events or groups
  struct read_format {
    uint64_t nr;
    uint64_t time_enabled;  // an event group is scheduled atomically
    uint64_t time_running;
    struct {
      uint64_t value;
      uint64_t id;
    } values[];
  };

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

    read(group_fd, &(buf[0]), sizeof(buf));
    struct read_format* pdata = reinterpret_cast<struct read_format*>(buf);

    if ((sizeof(pdata->nr) + pdata->nr * sizeof(pdata->values[0])) >
        sizeof(buf)) {
      throw std::runtime_error(
          "PerfCounter::read_counters got more data than the internal buffer");
    }

    std::cout << "PerfCounter::read_counters group ran as " << std::dec
              << pdata->time_running << " / " << pdata->time_enabled << "\n";

    for (int count_i = 0; count_i < pdata->nr; count_i++) {
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

  void add_counter(PerfEventAttr_config_t counter_config /* counter name */,
                   CounterName_t counter_name_s,
                   PerfEventAttr_type_t counter_type = PERF_TYPE_HARDWARE)

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
    // pe.exclude_kernel = 1;
    // pe.exclude_hv = 1;
    pe.exclude_kernel = 0;
    pe.exclude_hv = 0;
    pe.exclude_user = 0;

    pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID |
                     PERF_FORMAT_TOTAL_TIME_ENABLED |
                     PERF_FORMAT_TOTAL_TIME_RUNNING;

    CounterFD_t new_counter_fd = perf_event_open(&pe, pid_, cpu_, group_fd, 0);
    if (new_counter_fd == -1) {
      std::cerr << "PerfCounter::perf_event_open error opening leader "
                << std::hex << pe.config << std::dec << errno
                << std::strerror(errno) << "\n";
      exit(EXIT_FAILURE);
    }
    counters_fds.push_back(new_counter_fd);

    // if it is a brand new group:
    if (group_fd == -1) {
      group_fd = new_counter_fd;
    }

    CounterId_t new_id{};
    ioctl(new_counter_fd, PERF_EVENT_IOC_ID, &new_id);

    // counter_ids.push_back(new_id);
    // counter_types.push_back(counter_type);
    // counter_configs.push_back(counter_config);
    // counter_names.push_back(counter_name_s);
    counters_[new_id] = {.c_id = new_id,
                         .c_name = counter_name_s,
                         .c_config = counter_config,
                         .c_type = counter_type};
  }

  void add_counter(CounterName_t known_event_s) {
    if (known_events_map.find(known_event_s) == known_events_map.end()) {
      std::cerr << "PerfCounter::add_counter could not find " << known_event_s
                << " in known events\n";
      return;
    }

    auto event_desc = known_events_map.at(known_event_s);
    add_counter(event_desc.perf_config, known_event_s, event_desc.perf_type);
  }

  void start_count(void) {
    if (group_fd == -1) {
      throw std::runtime_error(
          "PerfCounter::start_count called without counters");
    }

    ioctl(group_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(group_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    // this enables only the very first event:
    // ioctl(group_fd, PERF_EVENT_IOC_RESET, 0);
    // ioctl(group_fd, PERF_EVENT_IOC_ENABLE, 0);
    counter_is_running = true;
  }

  void stop_count(void) {
    if (!counter_is_running) {
      return;
    }

    ioctl(group_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    // ioctl(group_fd, PERF_EVENT_IOC_DISABLE, 0);
    counter_is_running = false;
  }

  // return data to the user
  AllCountersData get_data(void) {
    read_counters();
    return {.pid = pid_, .cpu = cpu_, .counters = counters_};
  }
};
