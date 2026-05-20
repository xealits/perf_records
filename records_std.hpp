#pragma once
// serialisation into language standard types
// for easier and faster building and linkage

#ifndef PERF_RECORDS_NAMESPACE_NAME
#define PERF_RECORDS_NAMESPACE_NAME perf_records
#endif

#include <iostream>
#include <string>
#include <map>
#include <optional>

namespace PERF_RECORDS_NAMESPACE_NAME {

template<typename DataT>
struct RecordStd {
  std::optional<DataT> data{};
  std::string name{};
  std::map<std::string, RecordStd<DataT>> sub_records{};

  void print(unsigned nesting = 0) const {
    for (unsigned ind = 0; ind < nesting; ind++) std::cout << " ";
    if (data.has_value()) {
      std::cout << "RecordStd " << name << " = " << data.value() << '\n';
    }

    else {
      std::cout << "RecordStd " << name << " = NA\n";
    }

    for (const auto& [name, rec_std] : sub_records) {
      rec_std.print(nesting+1);
    }
  }
};

};
