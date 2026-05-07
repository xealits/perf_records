#pragma once

namespace perf_records {

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
};

using CounterNameT = FixedString<32>;

template<typename CounterDataT
    , auto CallableInputProc = [](const auto& inp) {return inp;}
    // WARNING: an auto parameter in a lambda in a template is a bug in GCC 14 as of 2024-07:
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88313
    // it is fixed in GCC 14.3
    , auto CallableCurrentCounter = [](void) {} /// default is intentionally broken
    >

struct Counters {
    template<CounterNameT name>
    struct DataStruct {
        static inline CounterDataT data{};

        template<typename InpT>
        static void set(const InpT& inp) { data = CallableInputProc(inp); }

        template<typename InpT>
        static void increment(const InpT& inp) { data += CallableInputProc(inp); }

        static CounterDataT get(void) { return data; }

        template<auto CallableGetCurrentCount>
        struct ScopeCounter {
            decltype(CallableGetCurrentCount()) count_at_scope_start;

            ScopeCounter(void) : count_at_scope_start{CallableGetCurrentCount()} {};

            ~ScopeCounter() {
                const auto increment_in_scope = CallableGetCurrentCount() - count_at_scope_start;
                increment(increment_in_scope);
            }
        };

        static auto make_scope_counter(void) {
            return ScopeCounter<CallableCurrentCounter>();
        } 
    };

    template<CounterNameT name>
    using counter = DataStruct<name>;

    template<typename ParentCounterT>
    struct SubCounters {
        template<CounterNameT name>
        using counter = DataStruct<name>;
        using sub_counters = SubCounters<SubCounters<ParentCounterT>>;
    };

    using sub_counters = SubCounters<Counters<CounterDataT>>;
};

};
