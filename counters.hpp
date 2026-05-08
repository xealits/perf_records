#pragma once
#ifndef PERF_RECORDS_NAMESPACE_NAME
#define PERF_RECORDS_NAMESPACE_NAME perf_records
#endif

#include <optional>
#include "records_std.hpp"

namespace PERF_RECORDS_NAMESPACE_NAME {

template<unsigned str_size>
struct FixedString {
    template<auto n_chars>
    constexpr FixedString(const char (&str)[n_chars]) {
        static_assert(n_chars + 1 < str_size, "the literal string must fit in the FixedString::value - because the literal string is used in the shaders, to match with the shaders the value must contain all of the attribute name");
        std::copy_n(str, n_chars, value);
        value[n_chars] = '\0';
    }

    char value[str_size] = "";
    constexpr const char* c_str(void) const noexcept {
        return &value[0];
    }

    constexpr bool operator==(const FixedString<str_size>& other) const {
        // compare up to the first \0 character or the end of strings
        for (size_t chi = 0; chi < str_size; chi++) {
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
    bool operator<(const FixedString<str_size>& other) const {
        // compare up to the first \0 character or the end of strings
        for (size_t chi = 0; chi < str_size; chi++) {
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
    friend std::ostream& operator<<(std::ostream& outs, const FixedString<str_size>& fstr) {
        outs << fstr.c_str();
        return outs;
    }

    // also convert to std::string
    operator std::string(void) const {
        return std::string{value, value + str_size};
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

/// a wrapper around DataT that does nothing but implements std::optional-like interface
/// std::optional interface follows the iterators interface
/// the wrapper just provides elements of this interface for a simple data type
template<typename DataT>
struct IteratorLikeWrapper {
  using this_t = IteratorLikeWrapper<DataT>;
  DataT payload{};

  constexpr bool has_value(void) const {return true;}
  constexpr void emplace(void) const {}
  constexpr DataT& value(void) {return payload;}
  constexpr const DataT& value(void) const {return payload;}
  this_t& operator=(const DataT& new_val) {payload = new_val; return *this;}
  DataT& operator*(void) {return payload;}
  DataT* operator->(void) {return &payload;}
};

template<typename DataT, bool use_optional>
struct OptionalOrBare;

template<typename DataT>
struct OptionalOrBare<DataT, true> {
  using type = std::optional<DataT>;
};

template<typename DataT>
struct OptionalOrBare<DataT, false> {
  using type = IteratorLikeWrapper<DataT>;
};

template<typename ParentCounterT
    , typename CounterDataT
    , auto CallableInputProc = [](const auto& inp) {return inp;}
    // WARNING: an auto parameter in a lambda in a template is a bug in GCC 14
    // it is fixed in GCC 14.3
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88313
    , auto CallableCurrentCounter = [](void) {} /// default is intentionally broken
    , bool UseOptional = true
>

struct CountersType {
    using parent_t = ParentCounterT;
    using data_t = CounterDataT;
    using wrapped_data_t = OptionalOrBare<CounterDataT, UseOptional>::type;
    static inline auto input_proc = CallableInputProc;
    static inline auto current_count_getter = CallableCurrentCounter;
    static inline constexpr auto use_optional = UseOptional;
};

// a starting point for parent types
struct EmptyT {};

template<CounterNameT t_name, typename counters_type>
struct Counters {
    using this_t = Counters<t_name, counters_type>;
    static inline CounterNameT name = t_name;
    static inline counters_type::wrapped_data_t data{};

    template<CounterNameT subcount_name>
    using counter = Counters<subcount_name,
        CountersType<this_t,
            typename counters_type::data_t, counters_type::input_proc, counters_type::current_count_getter, counters_type::use_optional>>;

    template<typename InpT>
    static void set(const InpT& inp) { data = counters_type::input_proc(inp); }

    template<typename InpT>
    static void increment(const InpT& inp) {
        if (!data.has_value()) {
            data.emplace(); // initialize the data
        }
        *data += counters_type::input_proc(inp);
    }

    static auto get(void) { return data; }

    struct ScopeCounter {
        decltype(counters_type::current_count_getter()) count_at_scope_start;

        ScopeCounter(void) : count_at_scope_start{counters_type::current_count_getter()} {};
        ScopeCounter(const auto& start_count) : count_at_scope_start{start_count} {};

        ~ScopeCounter() {
            const auto increment_in_scope = counters_type::current_count_getter() - count_at_scope_start;
            increment(increment_in_scope);
        }
    };

    // helper to convert the counters data to an std records structure
    template<typename ParentCounters, typename OutMapNameT, typename Subnode, typename... Rest>
    static void add_subnodes_to_map(
        std::map<OutMapNameT, RecordStd<typename counters_type::data_t>>& nodes_map,
        const TypePack<Subnode, Rest...> pack)

    {
        {
            // add the Subnode and its nested tree if present
            RecordStd<typename counters_type::data_t> rec;
            // get the subnode data from the parent counters_type
            rec.name = Subnode::node_name;

            // RecordStd data is always std::optional
            if constexpr (counters_type::use_optional) {
                rec.data = ParentCounters::template counter<Subnode::node_name>::data;
            } else {
                // assume the value -- because the wrapper always has a value
                rec.data = ParentCounters::template counter<Subnode::node_name>::data.value();
            }

            if constexpr (sizeof_pack(Subnode::subnodes_t) > 0) {
                using sub_counters_t = ParentCounters::template counter<Subnode::node_name>;
                add_subnodes_to_map<sub_counters_t>(rec.sub_records, Subnode::subnodes_t);
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
    static RecordStd<typename counters_type::data_t> nametree_to_record_std(const TypePack<Subnodes...>& pack) {
        //auto rec_data = NodeCounter::data;
        RecordStd<typename counters_type::data_t> rec_std;
        rec_std.name = NodeCounter::name;

        if constexpr (counters_type::use_optional) {
            rec_std.data = NodeCounter::data;
        } else {
            rec_std.data = NodeCounter::data.value();
        }

        if constexpr (sizeof...(Subnodes) > 0) {
            add_subnodes_to_map<NodeCounter>(rec_std.sub_records, TypePack<Subnodes...>{});
        }

        return rec_std;
    }

    template<typename Nodetree, typename... OtherNodetrees>
    static RecordStd<typename counters_type::data_t> nametree_to_record_std(void) {
        RecordStd<typename counters_type::data_t> top_counters_record;
        top_counters_record.name = name;

        if constexpr (counters_type::use_optional) {
            top_counters_record.data = data;
        } else {
            top_counters_record.data = data.value();
        }

        using CounterT = counter<Nodetree::node_name>;
        auto rec = nametree_to_record_std<CounterT>(Nodetree::subnodes_t);
        top_counters_record.sub_records[Nodetree::node_name] = rec;

        if constexpr (sizeof...(OtherNodetrees) > 0) {
            auto other_top_rec = nametree_to_record_std<OtherNodetrees...>();
            top_counters_record.sub_records.insert(other_top_rec.sub_records.begin(), other_top_rec.sub_records.end());
        }

        return top_counters_record;
    }
};

};