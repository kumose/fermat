// list_benchmark.cpp
// Compile with: g++ -O2 -std=c++17 -I/path/to/fermat/include -lbenchmark -lpthread list_benchmark.cpp -o bench

#include <benchmark/benchmark.h>
#include <list>
#include <random>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>

#include <fermat/container/list.h>

/// Alias for fermat::List with default alignment.
template <typename T>
using FermatList = fermat::List<T>;

/// POD structure for testing.
struct Pod3Int {
    int a, b, c;
    Pod3Int() : a(0), b(0), c(0) {}
    Pod3Int(int x) : a(x), b(x), c(x) {} // for uniform initialization
    bool operator==(const Pod3Int& other) const { return a == other.a && b == other.b && c == other.c; }
    bool operator<(const Pod3Int& other) const { return a < other.a; }
};

/// Non-POD type: std::string.
using NonPod = std::string;

constexpr int64_t kSize = 100000;      ///< Number of elements for bulk tests.
constexpr int64_t kSmallSize = 1000;   ///< Size for insert/erase overhead tests.

/// Helper to generate random indices.
static int64_t random_index(int64_t max, int64_t seed_offset = 0) {
    static std::mt19937_64 rng(123456789 + seed_offset);
    std::uniform_int_distribution<int64_t> dist(0, max - 1);
    return dist(rng);
}

/// Helper to generate random integer.
static int random_int(int max, int64_t seed_offset = 0) {
    static std::mt19937_64 rng(987654321 + seed_offset);
    std::uniform_int_distribution<int> dist(0, max);
    return dist(rng);
}

/// Helper to generate random string (fixed length 8).
static std::string random_string(int64_t seed_offset = 0) {
    static std::mt19937_64 rng(1234567890 + seed_offset);
    std::uniform_int_distribution<char> dist('a', 'z');
    std::string s(8, ' ');
    for (char& c : s) c = dist(rng);
    return s;
}

/// ========== Construction ==========
template <typename List>
static void BM_ConstructEmpty(benchmark::State& state) {
    for (auto _ : state) {
        List d;
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_ConstructEmpty, std::list<int>);
BENCHMARK_TEMPLATE(BM_ConstructEmpty, FermatList<int>);
BENCHMARK_TEMPLATE(BM_ConstructEmpty, std::list<Pod3Int>);
BENCHMARK_TEMPLATE(BM_ConstructEmpty, FermatList<Pod3Int>);
BENCHMARK_TEMPLATE(BM_ConstructEmpty, std::list<NonPod>);
BENCHMARK_TEMPLATE(BM_ConstructEmpty, FermatList<NonPod>);

template <typename List, typename Value>
static void BM_ConstructFill(benchmark::State& state) {
    const int n = state.range(0);
    Value val;
    if constexpr (std::is_same_v<Value, NonPod>) {
        val = Value(8, 'x');
    } else {
        val = Value(42);
    }
    for (auto _ : state) {
        List d(n, val);
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(n);
}
#define BENCH_CONSTRUCT_FILL(ValueType, list_type) \
    BENCHMARK_TEMPLATE(BM_ConstructFill, list_type<ValueType>, ValueType)->Range(8, kSize)->Complexity()
BENCH_CONSTRUCT_FILL(int, std::list);
BENCH_CONSTRUCT_FILL(int, FermatList);
BENCH_CONSTRUCT_FILL(Pod3Int, std::list);
BENCH_CONSTRUCT_FILL(Pod3Int, FermatList);
BENCH_CONSTRUCT_FILL(NonPod, std::list);
BENCH_CONSTRUCT_FILL(NonPod, FermatList);

/// ========== push_back ==========
template <typename List, typename Value>
static void BM_PushBack(benchmark::State& state) {
    for (auto _ : state) {
        List d;
        for (int i = 0; i < state.range(0); ++i) {
            Value v;
            if constexpr (std::is_same_v<Value, NonPod>) {
                v = random_string(i);
            } else {
                v = Value(i);
            }
            d.push_back(v);
        }
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(state.range(0));
}
#define BENCH_PUSH_BACK(ValueType, list_type) \
    BENCHMARK_TEMPLATE(BM_PushBack, list_type<ValueType>, ValueType)->Range(8, kSize)->Complexity()
BENCH_PUSH_BACK(int, std::list);
BENCH_PUSH_BACK(int, FermatList);
BENCH_PUSH_BACK(Pod3Int, std::list);
BENCH_PUSH_BACK(Pod3Int, FermatList);
BENCH_PUSH_BACK(NonPod, std::list);
BENCH_PUSH_BACK(NonPod, FermatList);

/// ========== push_front ==========
template <typename List, typename Value>
static void BM_PushFront(benchmark::State& state) {
    for (auto _ : state) {
        List d;
        for (int i = 0; i < state.range(0); ++i) {
            Value v;
            if constexpr (std::is_same_v<Value, NonPod>) {
                v = random_string(i);
            } else {
                v = Value(i);
            }
            d.push_front(v);
        }
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(state.range(0));
}
#define BENCH_PUSH_FRONT(ValueType, list_type) \
    BENCHMARK_TEMPLATE(BM_PushFront, list_type<ValueType>, ValueType)->Range(8, kSize)->Complexity()
BENCH_PUSH_FRONT(int, std::list);
BENCH_PUSH_FRONT(int, FermatList);
BENCH_PUSH_FRONT(Pod3Int, std::list);
BENCH_PUSH_FRONT(Pod3Int, FermatList);
BENCH_PUSH_FRONT(NonPod, std::list);
BENCH_PUSH_FRONT(NonPod, FermatList);

/// ========== Sequential iteration (sum) ==========
template <typename List>
static void BM_Iteration(benchmark::State& state) {
    // Fill with values
    List d;
    if constexpr (std::is_same_v<typename List::value_type, int>) {
        for (int i = 0; i < kSize; ++i) d.push_back(i);
    } else if constexpr (std::is_same_v<typename List::value_type, Pod3Int>) {
        for (int i = 0; i < kSize; ++i) d.push_back(Pod3Int(i));
    } else {
        for (int i = 0; i < kSize; ++i) d.push_back(random_string(i));
    }

    for (auto _ : state) {
        typename List::value_type sum{};
        for (const auto& v : d) {
            if constexpr (std::is_same_v<typename List::value_type, int>) sum += v;
            else if constexpr (std::is_same_v<typename List::value_type, Pod3Int>) sum.a += v.a;
            else sum += v; // string concatenation
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK_TEMPLATE(BM_Iteration, std::list<int>);
BENCHMARK_TEMPLATE(BM_Iteration, FermatList<int>);
BENCHMARK_TEMPLATE(BM_Iteration, std::list<Pod3Int>);
BENCHMARK_TEMPLATE(BM_Iteration, FermatList<Pod3Int>);
BENCHMARK_TEMPLATE(BM_Iteration, std::list<NonPod>);
BENCHMARK_TEMPLATE(BM_Iteration, FermatList<NonPod>);

/// ========== Insert at middle ==========
template <typename List>
static void BM_InsertMiddle(benchmark::State& state) {
    for (auto _ : state) {
        List d;
        // Fill half with values
        for (int i = 0; i < kSmallSize; ++i) {
            if constexpr (std::is_same_v<typename List::value_type, int>) d.push_back(i);
            else if constexpr (std::is_same_v<typename List::value_type, Pod3Int>) d.push_back(Pod3Int(i));
            else d.push_back(random_string(i));
        }
        auto it = d.begin();
        std::advance(it, d.size() / 2);
        typename List::value_type val{};
        if constexpr (std::is_same_v<typename List::value_type, int>) val = 999;
        else if constexpr (std::is_same_v<typename List::value_type, Pod3Int>) val = Pod3Int(999);
        else val = "inserted";
        for (int i = 0; i < 10; ++i) {
            it = d.insert(it, val);
            ++it;
        }
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_InsertMiddle, std::list<int>);
BENCHMARK_TEMPLATE(BM_InsertMiddle, FermatList<int>);
BENCHMARK_TEMPLATE(BM_InsertMiddle, std::list<Pod3Int>);
BENCHMARK_TEMPLATE(BM_InsertMiddle, FermatList<Pod3Int>);
BENCHMARK_TEMPLATE(BM_InsertMiddle, std::list<NonPod>);
BENCHMARK_TEMPLATE(BM_InsertMiddle, FermatList<NonPod>);

/// ========== Erase from middle ==========
template <typename List>
static void BM_EraseMiddle(benchmark::State& state) {
    for (auto _ : state) {
        List d;
        for (int i = 0; i < kSmallSize; ++i) {
            if constexpr (std::is_same_v<typename List::value_type, int>) d.push_back(i);
            else if constexpr (std::is_same_v<typename List::value_type, Pod3Int>) d.push_back(Pod3Int(i));
            else d.push_back(random_string(i));
        }
        auto it = d.begin();
        std::advance(it, d.size() / 2);
        for (int i = 0; i < 10; ++i) {
            it = d.erase(it);
        }
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_EraseMiddle, std::list<int>);
BENCHMARK_TEMPLATE(BM_EraseMiddle, FermatList<int>);
BENCHMARK_TEMPLATE(BM_EraseMiddle, std::list<Pod3Int>);
BENCHMARK_TEMPLATE(BM_EraseMiddle, FermatList<Pod3Int>);
BENCHMARK_TEMPLATE(BM_EraseMiddle, std::list<NonPod>);
BENCHMARK_TEMPLATE(BM_EraseMiddle, FermatList<NonPod>);

/// ========== Clear ==========
template <typename List>
static void BM_Clear(benchmark::State& state) {
    for (auto _ : state) {
        List d(kSmallSize, typename List::value_type{});
        d.clear();
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_Clear, std::list<int>);
BENCHMARK_TEMPLATE(BM_Clear, FermatList<int>);
BENCHMARK_TEMPLATE(BM_Clear, std::list<Pod3Int>);
BENCHMARK_TEMPLATE(BM_Clear, FermatList<Pod3Int>);
BENCHMARK_TEMPLATE(BM_Clear, std::list<NonPod>);
BENCHMARK_TEMPLATE(BM_Clear, FermatList<NonPod>);

/// ========== Destructor overhead ==========
template <typename List>
static void BM_Destruct(benchmark::State& state) {
    for (auto _ : state) {
        List d(kSmallSize, typename List::value_type{});
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_Destruct, std::list<int>);
BENCHMARK_TEMPLATE(BM_Destruct, FermatList<int>);
BENCHMARK_TEMPLATE(BM_Destruct, std::list<Pod3Int>);
BENCHMARK_TEMPLATE(BM_Destruct, FermatList<Pod3Int>);
BENCHMARK_TEMPLATE(BM_Destruct, std::list<NonPod>);
BENCHMARK_TEMPLATE(BM_Destruct, FermatList<NonPod>);

/// ========== Sort (random values) ==========
template <typename List>
static void BM_Sort(benchmark::State& state) {
    const int n = state.range(0);
    using Value = typename List::value_type;
    for (auto _ : state) {
        List d;
        for (int i = 0; i < n; ++i) {
            if constexpr (std::is_same_v<Value, int>) d.push_back(random_int(n, i));
            else if constexpr (std::is_same_v<Value, Pod3Int>) d.push_back(Pod3Int(random_int(n, i)));
            else d.push_back(random_string(i));
        }
        d.sort();
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(n);
}
#define BENCH_SORT(ValueType, list_type) \
    BENCHMARK_TEMPLATE(BM_Sort, list_type<ValueType>)->Range(8, kSize)->Complexity()
BENCH_SORT(int, std::list);
BENCH_SORT(int, FermatList);
BENCH_SORT(Pod3Int, std::list);
BENCH_SORT(Pod3Int, FermatList);
BENCH_SORT(NonPod, std::list);
BENCH_SORT(NonPod, FermatList);

/// ========== Merge (sorted lists) ==========
template <typename List>
static void BM_Merge(benchmark::State& state) {
    const int n = state.range(0);
    using Value = typename List::value_type;
    for (auto _ : state) {
        List a, b;
        for (int i = 0; i < n; ++i) {
            if constexpr (std::is_same_v<Value, int>) {
                a.push_back(i * 2);
                b.push_back(i * 2 + 1);
            } else if constexpr (std::is_same_v<Value, Pod3Int>) {
                a.push_back(Pod3Int(i * 2));
                b.push_back(Pod3Int(i * 2 + 1));
            } else {
                a.push_back(std::to_string(i * 2));
                b.push_back(std::to_string(i * 2 + 1));
            }
        }
        a.merge(b);
        benchmark::DoNotOptimize(a);
    }
    state.SetComplexityN(n);
}
#define BENCH_MERGE(ValueType, list_type) \
    BENCHMARK_TEMPLATE(BM_Merge, list_type<ValueType>)->Range(8, kSize)->Complexity()
BENCH_MERGE(int, std::list);
BENCH_MERGE(int, FermatList);
BENCH_MERGE(Pod3Int, std::list);
BENCH_MERGE(Pod3Int, FermatList);
BENCH_MERGE(NonPod, std::list);
BENCH_MERGE(NonPod, FermatList);

/// ========== Splice (move entire list) ==========
template <typename List>
static void BM_Splice(benchmark::State& state) {
    const int n = state.range(0);
    using Value = typename List::value_type;
    for (auto _ : state) {
        List a, b;
        for (int i = 0; i < n; ++i) {
            if constexpr (std::is_same_v<Value, int>) {
                a.push_back(i);
                b.push_back(i + n);
            } else if constexpr (std::is_same_v<Value, Pod3Int>) {
                a.push_back(Pod3Int(i));
                b.push_back(Pod3Int(i + n));
            } else {
                a.push_back(std::to_string(i));
                b.push_back(std::to_string(i + n));
            }
        }
        a.splice(a.end(), b);
        benchmark::DoNotOptimize(a);
    }
    state.SetComplexityN(n);
}
#define BENCH_SPLICE(ValueType, list_type) \
    BENCHMARK_TEMPLATE(BM_Splice, list_type<ValueType>)->Range(8, kSize)->Complexity()
BENCH_SPLICE(int, std::list);
BENCH_SPLICE(int, FermatList);
BENCH_SPLICE(Pod3Int, std::list);
BENCH_SPLICE(Pod3Int, FermatList);
BENCH_SPLICE(NonPod, std::list);
BENCH_SPLICE(NonPod, FermatList);

/// ========== Mixed pop_front and pop_back ==========
template <typename List>
static void BM_PopFrontBack(benchmark::State& state) {
    const int n = state.range(0);
    using Value = typename List::value_type;
    for (auto _ : state) {
        List d;
        for (int i = 0; i < n; ++i) {
            if constexpr (std::is_same_v<Value, int>) d.push_back(i);
            else if constexpr (std::is_same_v<Value, Pod3Int>) d.push_back(Pod3Int(i));
            else d.push_back(random_string(i));
        }
        for (int i = 0; i < n / 2; ++i) {
            d.pop_front();
            d.pop_back();
        }
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(n);
}
#define BENCH_POPFRONTBACK(ValueType, list_type) \
    BENCHMARK_TEMPLATE(BM_PopFrontBack, list_type<ValueType>)->Range(8, kSize)->Complexity()
BENCH_POPFRONTBACK(int, std::list);
BENCH_POPFRONTBACK(int, FermatList);
BENCH_POPFRONTBACK(Pod3Int, std::list);
BENCH_POPFRONTBACK(Pod3Int, FermatList);
BENCH_POPFRONTBACK(NonPod, std::list);
BENCH_POPFRONTBACK(NonPod, FermatList);

BENCHMARK_MAIN();