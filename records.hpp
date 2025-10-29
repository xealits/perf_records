#include <string>
#include <vector>

template <typename ValT>
struct Record {
  using RecordString_t = std::string;
  RecordString_t
      column_name;  //! short nickname of the record, used for columns etc
  ValT value;
  std::vector<Record<ValT>> subrecs;

  friend std::ostream& operator<<(std::ostream& outs, const Record<ValT>& rec) {
    outs << rec.column_name << " : " << rec.value << "\n";
    for (const auto& subr : rec.subrecs) {
      outs << " " << subr.column_name << " : " << subr.value << ",";
    }
    return outs;
  }
};
