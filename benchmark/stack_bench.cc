/// @file stack_benchmark.cc
/// @brief Benchmark comparing fermat::stack with std::stack.

#include <benchmark/benchmark.h>
#include <stack>
#include <vector>
#include <random>

#include "fermat/container/stack.h"   ///< Assumed correct path.

/// Generate a random integer sequence.
static std::vector<int> GenerateRandomData(size_t n) {
    std::vector<int> data(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(n * 10));
    std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
    return data;
}

/// ---------------------------------------------------------------------------
/// Benchmarks for std::stack (default container = std::deque<int>)
/// ---------------------------------------------------------------------------

/// BM_StdStack_PushPop: Push N elements then pop them all.
static void BM_StdStack_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);

    for (auto _ : state) {
        std::stack<int> st;
        for (int v : data) st.push(v);
        for (size_t i = 0; i < n; ++i) st.pop();
        benchmark::DoNotOptimize(st);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_StdStack_PushPop)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_StdStack_Top: Repeatedly access the top element.
static void BM_StdStack_Top(benchmark::State& state) {
    const size_t n = state.range(0);
    std::stack<int> st;
    for (size_t i = 0; i < n; ++i) st.push(static_cast<int>(i));

    for (auto _ : state) {
        volatile int x = st.top();
        benchmark::DoNotOptimize(x);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdStack_Top)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_StdStack_Emplace: Emplace N elements.
static void BM_StdStack_Emplace(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _ : state) {
        std::stack<int> st;
        for (size_t i = 0; i < n; ++i) st.emplace(static_cast<int>(i));
        benchmark::DoNotOptimize(st);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdStack_Emplace)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_StdStack_ConstructFromContainer: Construct from an existing container.
static void BM_StdStack_ConstructFromContainer(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);
    std::deque<int> init(data.begin(), data.end());

    for (auto _ : state) {
        std::stack<int> st(init);
        benchmark::DoNotOptimize(st);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdStack_ConstructFromContainer)->Arg(1000)->Arg(10000)->Arg(100000);

/// ---------------------------------------------------------------------------
/// Benchmarks for fermat::stack (default container = fermat::Vector<int,0>)
/// ---------------------------------------------------------------------------

/// BM_FermatStack_PushPop: Push N elements then pop them all.
static void BM_FermatStack_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);

    for (auto _ : state) {
        fermat::stack<int, 0> st;
        for (int v : data) st.push(v);
        for (size_t i = 0; i < n; ++i) st.pop();
        benchmark::DoNotOptimize(st);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_FermatStack_PushPop)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_FermatStack_Top: Repeatedly access the top element.
static void BM_FermatStack_Top(benchmark::State& state) {
    const size_t n = state.range(0);
    fermat::stack<int, 0> st;
    for (size_t i = 0; i < n; ++i) st.push(static_cast<int>(i));

    for (auto _ : state) {
        volatile int x = st.top();
        benchmark::DoNotOptimize(x);
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FermatStack_Top)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_FermatStack_Emplace: Emplace N elements.
static void BM_FermatStack_Emplace(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _ : state) {
        fermat::stack<int, 0> st;
        for (size_t i = 0; i < n; ++i) st.emplace(static_cast<int>(i));
        benchmark::DoNotOptimize(st);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatStack_Emplace)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_FermatStack_ConstructFromContainer: Construct from an existing container.
static void BM_FermatStack_ConstructFromContainer(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);
    fermat::Vector<int, 0> init(data.begin(), data.end());   ///< Use fermat::Vector as source.

    for (auto _ : state) {
        fermat::stack<int, 0> st(init);
        benchmark::DoNotOptimize(st);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatStack_ConstructFromContainer)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_FermatStack_GetContainer: Access the underlying container.
static void BM_FermatStack_GetContainer(benchmark::State& state) {
    const size_t n = state.range(0);
    fermat::stack<int, 0> st;
    for (size_t i = 0; i < n; ++i) st.push(static_cast<int>(i));

    for (auto _ : state) {
        auto& cont = st.get_container();
        benchmark::DoNotOptimize(cont);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatStack_GetContainer)->Arg(1000)->Arg(10000)->Arg(100000);

BENCHMARK_MAIN();