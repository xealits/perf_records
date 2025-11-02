#include <unistd.h>

#include <optional>
#include <string>
#include <vector>

using RecordString_t = std::string;

static std::string ind(unsigned lev = 1) {
  constexpr char ind[] = "  ";
  std::string res;
  for (unsigned num = 0; num < lev; num++) res += ind;
  return res;
}

template <typename ValT>
struct Record {
  RecordString_t
      column_name;  //! short nickname of the record, used for columns etc
  pid_t pid{0};     // TODO: this is extra data about the column - it begins..
  int cpu{-1};

  std::optional<ValT> value = std::nullopt;
  std::vector<Record<ValT>> conditions;
  std::vector<Record<ValT>> subrecs;

  std::string value_s(void) const {
    return value ? std::to_string(*value) : "NA";
  }

  friend std::ostream& operator<<(std::ostream& outs, const Record<ValT>& rec) {
    outs << rec.column_name << " @ " << rec.pid << " " << rec.cpu << " : "
         << rec.value_s() << "\n";
    for (const auto& subr : rec.subrecs) {
      outs << " " << subr.column_name << " : " << subr.value_s() << ",";
    }
    return outs;
  }

  std::string html(unsigned lev = 0) const {
    std::string res;
    auto lev_payload = subrecs.size() > 0 ? lev + 1 : lev;

    // let's mark the top node to always be able to scrap it
    if (lev == 0) res += "<div class=\"perf_records\">\n";

    // let's just dump it as a tree
    if (subrecs.size() > 0) {
      res += ind(lev) + "<details>\n";
      // details always has summary
      res += ind(lev_payload) + "<summary>\n";
    }

    // record payload data:
    // the value in var
    // and a dump of condition records - vars or details-summary
    res += ind(lev_payload) + "<var>" + column_name + " <data>" + value_s() +
           "</data>" + "</var>\n";
    if (conditions.size() > 0) {
      res += ind(lev_payload) + "<div class=\"perf_records_conditions\">\n";
      for (const auto& cond : conditions) {
        res += cond.html(lev_payload);
      }
      res += ind(lev_payload) + "</div>\n";
    }

    // nest further
    if (subrecs.size() > 0) {
      res += ind(lev_payload) + "</summary>\n";
      res += ind(lev_payload) + "<div class=\"perf_records_nest\">\n";
      for (const auto& subr : subrecs) {
        res += subr.html(lev_payload + 1);
      }
      res += ind(lev_payload) + "</div>\n";
      res += ind(lev) + "</details>\n";
    }

    if (lev == 0) res += "</div>\n";
    return res;
  }
};
