#pragma once
#ifndef PERF_RECORDS_NAMESPACE_NAME
#define PERF_RECORDS_NAMESPACE_NAME perf_records
#endif

#include <optional>
#include "records_std.hpp"

namespace PERF_RECORDS_NAMESPACE_NAME {

template<size_t t_str_size>
struct FixedString {
    size_t m_size;
    template<size_t n_chars>
    constexpr FixedString(const char (&str)[n_chars]) : m_size{n_chars} {
        static_assert(n_chars > 0, "it is always true that a char[] size is > 0");
        static_assert(n_chars + 1 < t_str_size, "the literal string must fit in the FixedString::value - because the literal string is used in the shaders, to match with the shaders the value must contain all of the attribute name");
        std::copy_n(str, n_chars, value);
        value[n_chars] = '\0';
    }

    char value[t_str_size] = "";
    constexpr const char* c_str(void) const noexcept {
        return &value[0];
    }

    constexpr bool operator==(const FixedString<t_str_size>& other) const {
        // compare up to the first \0 character or the end of strings
        for (size_t chi = 0; chi < t_str_size; chi++) {
            if (value[chi] != other.value[chi]) {
                return false;
            }

            if (value[chi] == '\0' || other.value[chi] == '\0') {
                return value[chi] == other.value[chi];
            }
        }

        return true;
    }

    // strict ordering is needed to use FixedString as a key in std::map
    bool operator<(const FixedString<t_str_size>& other) const {
        // compare up to the first \0 character or the end of strings
        for (size_t chi = 0; chi < t_str_size; chi++) {
            if (value[chi] != other.value[chi]) {
                return value[chi] < other.value[chi];
            }

            if (value[chi] == '\0' && other.value[chi] == '\0') {
              // the two strings are identical
              return false;
            }

            if (value[chi] == '\0' || other.value[chi] == '\0') {
                return value[chi] == '\0';
            }
        }

        // the two strings are identical
        return false;
    }

    //
    friend std::ostream& operator<<(std::ostream& outs, const FixedString<t_str_size>& fstr) {
        outs << fstr.c_str();
        return outs;
    }

    // also convert to std::string
    operator std::string(void) const {
        return std::string{value, value + m_size - 1};
    }
};

using CounterNameT = FixedString<32>;

template<typename... Ts>
struct TypePack {};

template<typename... Ts>
constexpr unsigned sizeof_pack(TypePack<Ts...> pack) {
  return sizeof...(Ts);
}

// the manual way to pull the info from the templated class
// by a given nested tree of template parameters
// ideally, this should be done with Cpp26 reflections
template<CounterNameT name, typename... Subnodes>
struct NameTreeNode {
  static constexpr inline CounterNameT node_name = name;
  static constexpr inline TypePack<Subnodes...> subnodes_t{};
};

template<class T, template<class> class U>
inline constexpr bool is_instance_of_v = std::false_type{};

template<template<class> class U, class V>
inline constexpr bool is_instance_of_v<U<V>,U> = std::true_type{};

template<typename DataT, bool using_optional>
struct StripOptional;

template<typename DataT>
struct StripOptional<DataT, true> {
  using type = DataT::value_type;
};

template<typename DataT>
struct StripOptional<DataT, false> {
  using type = DataT;
};

template<typename DataT>
struct OptionalOrBare {
  using type = DataT;
  static inline constexpr bool using_optional = is_instance_of_v<DataT, std::optional>;
  using data_t = StripOptional<DataT, using_optional>::type;
};

template<
  // WARNING: an auto parameter in a lambda in a template is a bug in GCC 14
  // it is fixed in GCC 14.3
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88313
  typename LambdaProcT = decltype([](const auto& inp) {return inp;}),
  /// default is intentionally broken
  typename LambdaCounterGetterT = decltype([](void) {})
>
struct CountersTypeOptionalParameters {
    LambdaProcT input_proc = {};
    LambdaCounterGetterT current_count_getter = {};
    //using current_count_t = decltype(current_count_getter());
};

template<typename ParentCounterT
    , typename CounterOptionalOrDataT
    , CountersTypeOptionalParameters params = CountersTypeOptionalParameters<>{}
>

struct CountersType {
    using parent_t = ParentCounterT;
    using optional_or_data_t = std::conditional<is_instance_of_v<CounterOptionalOrDataT, OptionalOrBare>,
          CounterOptionalOrDataT,
          OptionalOrBare<CounterOptionalOrDataT>
    >::type;
    static inline auto optional_params = params;
    static inline auto input_proc = params.input_proc;
    static inline auto current_count_getter = params.current_count_getter;
    //using current_count_t = params::current_count_t;
    using current_count_t = decltype(current_count_getter());
};

// a starting point for parent types
struct EmptyT {};

template<CounterNameT t_name, typename counters_type>
struct Counters {
    using this_t = Counters<t_name, counters_type>;
    static inline CounterNameT name = t_name;
    static inline counters_type::optional_or_data_t::type data{};

    template<CounterNameT subcount_name>
    using counter = Counters<subcount_name,
        CountersType<this_t
          , typename counters_type::optional_or_data_t
          , counters_type::optional_params
        >>;

    template<typename InpT>
    static void set(const InpT& inp) { data = counters_type::input_proc(inp); }

    template<typename InpT>
    static void increment(const InpT& inp) {
        if constexpr (counters_type::optional_or_data_t::using_optional) {
          if (!data.has_value()) {
              data.emplace(); // initialize the data
          }
          *data += counters_type::input_proc(inp);
        }

        else {
          data += counters_type::input_proc(inp);
        }
    }

    // return a reference to the counter
    // in case someone wants to do something manual with it
    static auto& get(void) { return data; }

    static auto get_current(void) { return counters_type::current_count_getter(); }

    template<typename InpT>
    static auto increment_current_diff(const InpT& inp_offset) {
        auto current = get_current();
        const auto diff = current - inp_offset;
        increment(diff);
        return current;
    }

    template<bool log_increments_t = false>
    struct ScopeCounter {
        using count_t = decltype(counters_type::current_count_getter());
        //using count_t = counters_type::current_count_t;
        const count_t count_at_scope_start;

        ScopeCounter(void)
          : count_at_scope_start{counters_type::current_count_getter()} {};

        // TODO:
        // with the typical chrono clock timers, time_points and durations,
        // the start_count deduction stopped working after the bool log_increments was added
        // but it works fine in a simple separate example of a standalone ScopeCounter struct
        // without the nesting of types that happens here
        ScopeCounter(const auto& start_count)

        // here it says "no known conversions from time_point to count_t aka decltype()"
        // even though the type behind decltype is time_point
        //ScopeCounter(const count_t& start_count)
        //template<typename StartCountT>
        //ScopeCounter(const StartCountT& start_count)
          //: count_at_scope_start{static_cast<count_t>(start_count)}
          : count_at_scope_start{start_count}
        {};

        ~ScopeCounter() {
            const auto increment_in_scope =
              counters_type::current_count_getter() - count_at_scope_start;
            increment(increment_in_scope);

            if constexpr (log_increments_t) {
                std::cout << name << " ScopeCounter increment = "
                    << increment_in_scope << "\n";
            }
        }
    };

    template<bool log_increments_t = false>
    struct ScopeCounterConditional {
        decltype(counters_type::current_count_getter()) count_at_scope_start;
        bool m_do_increment_counter = false;

        ScopeCounterConditional(bool increment_or_no)
          : m_do_increment_counter{increment_or_no}
          , count_at_scope_start{counters_type::current_count_getter()} {};
        ScopeCounterConditional(bool increment_or_no, const auto& start_count)
          : m_do_increment_counter{increment_or_no}
          , count_at_scope_start{start_count} {};

        ~ScopeCounterConditional() {
            if (m_do_increment_counter) {
                const auto increment_in_scope =
                counters_type::current_count_getter() - count_at_scope_start;
                increment(increment_in_scope);

                if constexpr (log_increments_t) {
                    std::cout << name
                        << " ScopeCounterConditional increment = "
                        << increment_in_scope << "\n";
                }
            }
        }
    };

    // helper to convert the counters data to an std records structure
    template<typename ParentCounters, typename OutMapNameT, typename Subnode, typename... Rest>
    static void add_subnodes_to_map(
        std::map<OutMapNameT, RecordStd<typename counters_type::optional_or_data_t::data_t>>& nodes_map,
        const TypePack<Subnode, Rest...> pack)

    {
        {
            // add the Subnode and its nested tree if present
            RecordStd<typename counters_type::optional_or_data_t::data_t> rec;
            // get the subnode data from the parent counters_type
            rec.name = Subnode::node_name;

            // RecordStd data is always std::optional
            rec.data = ParentCounters::template counter<Subnode::node_name>::data;

            if constexpr (sizeof_pack(Subnode::subnodes_t) > 0) {
                using sub_counters_t =
                  ParentCounters::template counter<Subnode::node_name>;
                add_subnodes_to_map<sub_counters_t>(
                    rec.sub_records, Subnode::subnodes_t);
            }
            nodes_map[Subnode::node_name] = rec;
        }

        // add the Rest subnodes
        constexpr auto rest_size = sizeof...(Rest);
        if constexpr (rest_size > 0) {
            add_subnodes_to_map<ParentCounters>(nodes_map, TypePack<Rest...>{});
        }
    }

    template<typename NodeCounter, typename... Subnodes>
    static RecordStd<typename counters_type::optional_or_data_t::data_t>
    nametree_to_record_std(const TypePack<Subnodes...>& pack) {
        RecordStd<typename counters_type::optional_or_data_t::data_t> rec_std;
        rec_std.name = NodeCounter::name;
        rec_std.data = NodeCounter::data;

        if constexpr (sizeof...(Subnodes) > 0) {
            add_subnodes_to_map<NodeCounter>(rec_std.sub_records, TypePack<Subnodes...>{});
        }

        return rec_std;
    }

    template<typename Nodetree, typename... OtherNodetrees>
    static RecordStd<typename counters_type::optional_or_data_t::data_t> nametree_to_record_std(void) {
        RecordStd<typename counters_type::optional_or_data_t::data_t> top_counters_record;
        top_counters_record.name = name;
        top_counters_record.data = data;

        using CounterT = counter<Nodetree::node_name>;
        auto rec = nametree_to_record_std<CounterT>(Nodetree::subnodes_t);
        top_counters_record.sub_records[Nodetree::node_name] = rec;

        if constexpr (sizeof...(OtherNodetrees) > 0) {
            auto other_top_rec = nametree_to_record_std<OtherNodetrees...>();
            top_counters_record.sub_records.insert(
                other_top_rec.sub_records.begin(),
                other_top_rec.sub_records.end()
            );
        }

        return top_counters_record;
    }
};

};
