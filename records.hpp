#pragma once
/// \file records.hpp
/// \brief Record class for damping a tree of perf values to HTML log

#include <unistd.h>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

using RecordString_t = std::string;

static std::string ind(unsigned lev = 1) {
  constexpr char ind[] = "  ";
  std::string res;
  for (unsigned num = 0; num < lev; num++) res += ind;
  return res;
}

template<typename RecordLike>
std::string RecordToHtml(RecordLike& record, unsigned lev = 0) {
  const auto& column_name = record.column_name;
  const auto& value_s = record.value_s();
  const auto& subrecs = record.subrecs;
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
  res += ind(lev_payload)
    + "<var class=\"perf_records_parameter\" data-name=\"" + column_name + "\">"
    + "<dfn>" + column_name + "</dfn>"
    " = "
    + "<data>" + value_s + "</data>"
    + "</var>\n";
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

// exactly the same stuff as above but for types
// TODO: use static classes and constexpr instead of types
template<typename RecordLikeT>
std::string RecordToHtml(unsigned lev = 0) {
  const std::string& column_name = RecordLikeT::name;
  const std::string& value_s = []() {
    constexpr bool using_optional = RecordLikeT::this_counters_type::using_optional;
    std::ostringstream oss;

    if constexpr (using_optional) {
      const auto& opt_data = RecordLikeT::data;
      if (opt_data.has_value()) oss << opt_data.data();
    }

    else {
      oss << RecordLikeT::data;
    }

    return oss.str();
  }();

  //const auto& subrecs = record.subrecs;
  //const auto& conditions = record.conditions;
  // ok, subrecords and conditions are types in the Counters
  // worse: they are template types - I cannot loop over them

  std::string res;
  //auto lev_payload = subrecs.size() > 0 ? lev + 1 : lev;
  auto lev_payload = lev;

  // let's mark the top node to always be able to scrap it
  if (lev == 0) res += "<div class=\"perf_records\">\n";

  //// let's just dump it as a tree
  //if (subrecs.size() > 0) {
  //  res += ind(lev) + "<details>\n";
  //  // details always has summary
  //  res += ind(lev_payload) + "<summary>\n";
  //}

  // record payload data:
  // the value in var
  // and a dump of condition records - vars or details-summary
  // use attribute for the var name, to easily select nodes in parsing:
  // soup.select_one("var[data-name=analysisID] > data").contents
  res += ind(lev_payload)
    + "<var class=\"perf_records_parameter\" data-name=\"" + column_name + "\">"
    + "<dfn>" + column_name + "</dfn>"
    " = "
    + "<data>" + value_s + "</data>"
    + "</var>\n";
  //if (conditions.size() > 0) {
  //  res += ind(lev_payload) + "<div class=\"perf_records_conditions\">\n";
  //  for (const auto& cond : conditions) {
  //    res += cond.html(lev_payload);
  //  }
  //  res += ind(lev_payload) + "</div>\n";
  //}

  //// nest further
  //if (subrecs.size() > 0) {
  //  res += ind(lev_payload) + "</summary>\n";
  //  res += ind(lev_payload) + "<div class=\"perf_records_nest\">\n";
  //  for (const auto& subr : subrecs) {
  //    res += subr.html(lev_payload + 1);
  //  }
  //  res += ind(lev_payload) + "</div>\n";
  //  res += ind(lev) + "</details>\n";
  //}

  if (lev == 0) res += "</div>\n";
  return res;
}


template <typename ValT>
struct Record {
  RecordString_t
      column_name;  //! short nickname of the record, used for columns etc
  pid_t pid{0};     // TODO: this is extra data about the column - it begins..
  int cpu{-1};

  std::optional<ValT> value = std::nullopt;
  std::vector<Record<ValT>> conditions{};
  std::vector<Record<ValT>> subrecs{};

  std::string value_s(void) const {
    return value ? std::to_string(*value) : "NA";
  }

  void to_ostream(std::ostream& outs, unsigned nesting = 0) const {
    for (unsigned nest = 0; nest < nesting; nest++) {
      outs << " ";
    }

    outs << column_name << " @ " << pid << " " << cpu << " : "
         << value_s() << "\n";

    for (const auto& subr : subrecs) {
      subr.to_ostream(outs, nesting + 1);
    }
  }

  friend std::ostream& operator<<(std::ostream& outs, const Record<ValT>& rec) {
    rec.to_ostream(outs, 0);
    return outs;
  }

  std::string html(unsigned lev = 0) const {
    return RecordToHtml(*this, lev);
  }
};
