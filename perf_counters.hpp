// set up perf
#include <errno.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>

#include <cstring>
// #include <linux/hw_breakpoint.h>
#include <asm/unistd.h>

#include <map>
#include <string>
#include <vector>

// a macro for the users
// the point is to to turn the perf_event.h enum into a string
// libpfm has pfm_get_event_name for that
#define add_counter(event, ...) add_counter_impl(event, #event, ##__VA_ARGS__)

class PerfCounter {
 public:
  using CounterFD_t = decltype(syscall(__NR_perf_event_open, 0, 0, 0, -1, 0));
  using CounterId_t = uint64_t;
  using CounterVal_t = uint64_t;
  using CounterName_t = std::string;

  struct CounterDesc {
    CounterId_t c_id{};
    CounterVal_t c_val{0};
    CounterName_t c_name;
    decltype(perf_event_attr{}.config) c_config{};
    decltype(perf_event_attr{}.type) c_type{};
  };

 private:
  bool counter_is_running{false};
  int group_fd{-1};  // this fd will be the counter group fd
  std::vector<CounterFD_t> counters_fds;
  // fd_cycles, fd_backend;
  pid_t pid_{0};
  int cpu_{-1};

  struct perf_event_attr pe{};

  std::map<CounterId_t, CounterDesc> counters_;

  static constexpr unsigned perf_data_buffer_size_ = 4096;
  char buf[perf_data_buffer_size_] = {};

  // TODO: does the linux header have this struct???
  struct read_format {
    uint64_t nr;
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

 public:
  PerfCounter(void) = default;  // no other constructors
  PerfCounter(const PerfCounter& ref) = delete;
  PerfCounter(PerfCounter&& ref) = delete;
  PerfCounter& operator=(const PerfCounter&) = delete;
  PerfCounter& operator=(PerfCounter&&) = delete;

  ~PerfCounter() {
    for (auto counter_fd : counters_fds) {
      close(counter_fd);
    }
  };

  void add_counter_impl(decltype(pe.config) counter_config /* counter name */,
                        CounterName_t counter_name_s,
                        decltype(pe.type) counter_type = PERF_TYPE_HARDWARE)

  {
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
    pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

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

  void start_count(void) {
    if (group_fd == -1) {
      throw std::runtime_error(
          "PerfCounter::start_count called without counters");
    }

    ioctl(group_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
    ioctl(group_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    counter_is_running = true;
  }

  void stop_count(void) {
    if (!counter_is_running) {
      return;
    }

    ioctl(group_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    counter_is_running = false;
  }

  std::vector<CounterDesc> read_counters(void) {
    std::vector<CounterDesc> res;
    stop_count();

    read(group_fd, &(buf[0]), sizeof(buf));
    struct read_format* pdata = reinterpret_cast<struct read_format*>(buf);

    if ((sizeof(pdata->nr) + pdata->nr * sizeof(pdata->values[0])) >
        sizeof(buf)) {
      throw std::runtime_error(
          "PerfCounter::read_counters got more data than the internal buffer");
    }

    for (int count_i = 0; count_i < pdata->nr; count_i++) {
      const CounterId_t& counter_id = pdata->values[count_i].id;
      const CounterVal_t& counter_value = pdata->values[count_i].value;

      //
      auto counter = counters_.at(counter_id);
      counter.c_val = counter_value;
      res.push_back(counter);
    }

    return res;
  }
};
