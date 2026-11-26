#pragma once
// serialisation into language standard types
// for easier and faster building and linkage

#ifndef PERF_RECORDS_NAMESPACE_NAME
#define PERF_RECORDS_NAMESPACE_NAME perf_records
#endif

#include <cstddef>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace PERF_RECORDS_NAMESPACE_NAME {

namespace impl {

template <typename RecordLike>
std::string RecordToHtml(const RecordLike& record, unsigned lev = 0) {
  auto ind = [](size_t lev = 1) -> std::string {
    constexpr char ind_char = ' ';
    constexpr size_t ind_num_chars = 2;
    std::string res(ind_num_chars * lev, ind_char);
    return res;
  };

  const auto& column_name = record.name;
  const auto& value_s = record.value_s();
  const auto& subrecs = record.sub_records;
  const auto& conditions = record.conditions;

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
  // use attribute for the var name, to easily select nodes in parsing:
  // soup.select_one("var[data-name=analysisID] > data").contents
  res += ind(lev_payload) +
         "<var class=\"perf_records_parameter\" data-name=\"" + column_name +
         "\">" + "<dfn>" + column_name +
         "</dfn>"
         " = " +
         "<data>" + value_s + "</data>" + "</var>\n";
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
};  // namespace impl

template <typename ValueT>
struct Record {
  using RecordString_t = std::string;

  RecordString_t name;
  pid_t pid{0};
  int cpu{-1};

  std::optional<ValueT> value = std::nullopt;
  std::vector<Record<ValueT>> sub_records{};
  std::vector<Record<ValueT>> conditions{};

  std::string value_s(void) const {
    if (value) {
      //
      std::ostringstream oss;
      oss << value.value();
      return oss.str();
    } else
      return "NA";
  }

  void to_ostream(std::ostream& outs, unsigned nesting = 0) const {
    for (unsigned nest = 0; nest < nesting; nest++) {
      outs << " ";
    }

    outs << name << " @ " << pid << " " << cpu << " : " << value_s() << "\n";

    for (const auto& subr : sub_records) {
      subr.to_ostream(outs, nesting + 1);
    }
  }

  friend std::ostream& operator<<(std::ostream& outs,
                                  const Record<ValueT>& rec) {
    rec.to_ostream(outs, 0);
    return outs;
  }

  std::string html(size_t lev = 0) const {
    return impl::RecordToHtml(*this, lev);
  }
};

};  // namespace PERF_RECORDS_NAMESPACE_NAME
