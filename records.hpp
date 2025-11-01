#include <unistd.h>

#include <string>
#include <vector>
#include <optional>

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
    auto lev_payload = subrecs.size() > 0 ? lev+1 : lev;

    // let's just dump it as a tree
    if (subrecs.size() > 0) {
      res += ind(lev) + "<details>\n";
    }

    res += ind(lev_payload) + "<summary>\n";
    res += ind(lev_payload) + column_name + " <data>" + value_s() + "</data>\n";
    res += ind(lev_payload) + "</summary>\n";

    // nest further
    for (const auto& subr : subrecs) {
      res += subr.html(lev_payload+1);
    }

    if (subrecs.size() > 0) res += ind(lev) + "</details>\n";
    return res;
  }
};
