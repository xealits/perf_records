#include <unistd.h>

#include <string>
#include <vector>

using RecordString_t = std::string;

template <typename ValT>
struct Record {
  RecordString_t
      column_name;  //! short nickname of the record, used for columns etc
  pid_t pid{0};     // TODO: this is extra data about the column - it begins..
  int cpu{-1};

  ValT value;
  std::vector<Record<ValT>> subrecs;

  friend std::ostream& operator<<(std::ostream& outs, const Record<ValT>& rec) {
    outs << rec.column_name << " @ " << rec.pid << " " << rec.cpu << " : "
         << rec.value << "\n";
    for (const auto& subr : rec.subrecs) {
      outs << " " << subr.column_name << " : " << subr.value << ",";
    }
    return outs;
  }
};
