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

template<typename CounterDataT
    , auto CallableInputProc = [](const auto& inp) {return inp;}
    // WARNING: an auto parameter in a lambda in a template is a bug in GCC 14
    // it is fixed in GCC 14.3
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88313
    , auto CallableCurrentCounter = [](void) {} /// default is intentionally broken
    >

struct Counters {
    template<CounterNameT name>
    struct DataStruct {
        static inline std::optional<CounterDataT> data{};

        template<typename InpT>
        static void set(const InpT& inp) { data = CallableInputProc(inp); }

        template<typename InpT>
        static void increment(const InpT& inp) {
            if (!data.has_value()) {
                data.emplace(); // initialize the data
            }
            *data += CallableInputProc(inp);
        }

        static auto get(void) { return data; }

        template<auto CallableGetCurrentCount>
        struct ScopeCounter {
            decltype(CallableGetCurrentCount()) count_at_scope_start;

            ScopeCounter(void) : count_at_scope_start{CallableGetCurrentCount()} {};
            ScopeCounter(const auto& start_count) : count_at_scope_start{start_count} {};

            ~ScopeCounter() {
                const auto increment_in_scope = CallableGetCurrentCount() - count_at_scope_start;
                increment(increment_in_scope);
            }
        };

        static auto make_scope_counter(void) {
            return ScopeCounter<CallableCurrentCounter>();
        }

        static auto make_scope_counter(const auto& start_count) {
            return ScopeCounter<CallableCurrentCounter>(start_count);
        }

        template<typename ParentCounterT>
        struct SubCounters {
            template<CounterNameT subcount_name>
            using counter = DataStruct<subcount_name>;
            using sub_counters = SubCounters<SubCounters<ParentCounterT>>;
        };

        using sub_counters = SubCounters<Counters<CounterDataT, CallableInputProc, CallableCurrentCounter>>;
    };

    template<CounterNameT name>
    using counter = DataStruct<name>;

    // helper to convert the counters data to an std records structure
    template<typename ParentCounters, typename OutMapNameT, typename Subnode, typename... Rest>
    static void add_subnodes_to_map(std::map<OutMapNameT, RecordStd<CounterDataT>>& nodes_map, const TypePack<Subnode, Rest...> pack) {
        {
            // add the Subnode and its nested tree if present
            RecordStd<CounterDataT> rec;
            // get the subnode data from the parent type
            rec.data = ParentCounters::template counter<Subnode::node_name>::data;

            if constexpr (sizeof_pack(Subnode::subnodes_t) > 0) {
                using sub_counters_t = ParentCounters::template counter<Subnode::node_name>::sub_counters;
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
    static RecordStd<CounterDataT> nametree_to_record_std(const TypePack<Subnodes...>& pack) {
        auto rec_data = NodeCounter::data;

        if constexpr (sizeof...(Subnodes) == 0) {
            return RecordStd<CounterDataT>{rec_data};
        }

        else {
            RecordStd<CounterDataT> rec_tree;
            rec_tree.data = rec_data;

            using sub_counters_t = NodeCounter::sub_counters;
            add_subnodes_to_map<sub_counters_t>(rec_tree.sub_records, TypePack<Subnodes...>{});
            return rec_tree;
        }
    }

    template<typename Nodetree, typename... OtherNodetrees>
    static RecordStd<CounterDataT> nametree_to_record_std(void) {
        RecordStd<CounterDataT> top_counters_record; // TODO: currently Counters is a bit abnormal:
        // it contains counter<name> template
        // which instantiates a tree of subnodes
        // so Counters is unlike its subnodes, it is a set of these subnode trees
        // probably I should converge them - give Counters an ID name and turn it into a subnode tree itself
        // it makes sense, because the same data-related type parameters (data type and two processing lambdas)
        // can be used in a bunch of different areas of the code (the timing counters e.g.)

        using CounterT = Counters<CounterDataT, CallableInputProc, CallableCurrentCounter>::counter<Nodetree::node_name>;
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
