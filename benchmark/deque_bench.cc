// deque_benchmark.cpp
// Compile with: g++ -O2 -std=c++17 -lbenchmark -lpthread deque_benchmark.cpp -o bench

#include <benchmark/benchmark.h>
#include <deque>
#include <fermat/container/deque.h>

constexpr int64_t kSize = 100000;    // Number of elements for bulk tests
constexpr int64_t kSmallSize = 1000; // For insert/erase overhead

// Helper to generate random indices
static int64_t random_index(int64_t max, int64_t seed_offset = 0) {
    static std::mt19937_64 rng(123456789 + seed_offset);
    std::uniform_int_distribution<int64_t> dist(0, max - 1);
    return dist(rng);
}

// ========== Construction ==========
template <typename Deque>
static void BM_ConstructEmpty(benchmark::State& state) {
    for (auto _ : state) {
        Deque d;
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_ConstructEmpty, std::deque<int>);
BENCHMARK_TEMPLATE(BM_ConstructEmpty, fermat::Deque<int>);

template <typename Deque>
static void BM_ConstructFill(benchmark::State& state) {
    const int n = state.range(0);
    for (auto _ : state) {
        Deque d(n, 42);
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(n);
}
BENCHMARK_TEMPLATE(BM_ConstructFill, std::deque<int>)->Range(8, kSize)->Complexity();
BENCHMARK_TEMPLATE(BM_ConstructFill, fermat::Deque<int>)->Range(8, kSize)->Complexity();

// ========== push_back ==========
template <typename Deque>
static void BM_PushBack(benchmark::State& state) {
    for (auto _ : state) {
        Deque d;
        for (int i = 0; i < state.range(0); ++i) {
            d.push_back(i);
        }
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_TEMPLATE(BM_PushBack, std::deque<int>)->Range(8, kSize)->Complexity();
BENCHMARK_TEMPLATE(BM_PushBack, fermat::Deque<int>)->Range(8, kSize)->Complexity();

// ========== push_front ==========
template <typename Deque>
static void BM_PushFront(benchmark::State& state) {
    for (auto _ : state) {
        Deque d;
        for (int i = 0; i < state.range(0); ++i) {
            d.push_front(i);
        }
        benchmark::DoNotOptimize(d);
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK_TEMPLATE(BM_PushFront, std::deque<int>)->Range(8, kSize)->Complexity();
BENCHMARK_TEMPLATE(BM_PushFront, fermat::Deque<int>)->Range(8, kSize)->Complexity();

// ========== random access via operator[] ==========
template <typename Deque>
static void BM_RandomAccess(benchmark::State& state) {
    Deque d(kSize, 0);
    // Pre‑generate random indices to avoid measuring random number generation
    std::vector<int64_t> indices(kSize);
    for (int64_t i = 0; i < kSize; ++i) {
        indices[i] = random_index(kSize, i);
    }
    int64_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(d[indices[idx % kSize]]);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK_TEMPLATE(BM_RandomAccess, std::deque<int>);
BENCHMARK_TEMPLATE(BM_RandomAccess, fermat::Deque<int>);

// ========== iteration (sequential read) ==========
template <typename Deque>
static void BM_Iteration(benchmark::State& state) {
    Deque d(kSize, 42);
    for (auto _ : state) {
        int64_t sum = 0;
        for (const auto& v : d) {
            sum += v;
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK_TEMPLATE(BM_Iteration, std::deque<int>);
BENCHMARK_TEMPLATE(BM_Iteration, fermat::Deque<int>);

// ========== insert at middle ==========
template <typename Deque>
static void BM_InsertMiddle(benchmark::State& state) {
    for (auto _ : state) {
        Deque d;
        for (int i = 0; i < kSmallSize; ++i) {
            d.push_back(i);
        }
        auto it = d.begin() + d.size() / 2;
        for (int i = 0; i < 10; ++i) {
            it = d.insert(it, 999);
            ++it; // move past inserted element to avoid repeated insertion at same spot
        }
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_InsertMiddle, std::deque<int>);
BENCHMARK_TEMPLATE(BM_InsertMiddle, fermat::Deque<int>);

// ========== erase middle ==========
template <typename Deque>
static void BM_EraseMiddle(benchmark::State& state) {
    for (auto _ : state) {
        Deque d;
        for (int i = 0; i < kSmallSize; ++i) {
            d.push_back(i);
        }
        auto it = d.begin() + d.size() / 2;
        for (int i = 0; i < 10; ++i) {
            it = d.erase(it);
        }
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_EraseMiddle, std::deque<int>);
BENCHMARK_TEMPLATE(BM_EraseMiddle, fermat::Deque<int>);

// ========== clear ==========
template <typename Deque>
static void BM_Clear(benchmark::State& state) {
    for (auto _ : state) {
        Deque d(kSmallSize, 0);
        d.clear();
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_Clear, std::deque<int>);
BENCHMARK_TEMPLATE(BM_Clear, fermat::Deque<int>);

// ========== destructor overhead ==========
template <typename Deque>
static void BM_Destruct(benchmark::State& state) {
    for (auto _ : state) {
        Deque d(kSmallSize, 42);
        benchmark::DoNotOptimize(d);
    }
}
BENCHMARK_TEMPLATE(BM_Destruct, std::deque<int>);
BENCHMARK_TEMPLATE(BM_Destruct, fermat::Deque<int>);

BENCHMARK_MAIN();