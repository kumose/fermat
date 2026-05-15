/// @file priority_queue_benchmark.cc
/// @brief Benchmark comparing fermat::PriorityQueue with std::priority_queue.

#include <benchmark/benchmark.h>
#include <queue>
#include <random>
#include <vector>
#include <algorithm>

#include "fermat/container/priority_queue.h"   ///< Assumed correct path.

/// Generate a vector of random integers.
static std::vector<int> GenerateRandomData(size_t n) {
    std::vector<int> data(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(n * 10));
    std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
    return data;
}

/// ---------------------------------------------------------------------------
/// Benchmarks for std::priority_queue
/// ---------------------------------------------------------------------------

/// BM_StdPriorityQueue_PushPop: Measure push and pop operations.
static void BM_StdPriorityQueue_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);

    for (auto _ : state) {
        std::priority_queue<int> pq;
        for (int v : data) pq.push(v);
        for (size_t i = 0; i < n; ++i) pq.pop();
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_StdPriorityQueue_PushPop)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_StdPriorityQueue_ConstructFromIterators: Construct queue from iterator range.
static void BM_StdPriorityQueue_ConstructFromIterators(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);

    for (auto _ : state) {
        std::priority_queue<int> pq(std::less<int>(), std::vector<int>(data.begin(), data.end()));
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdPriorityQueue_ConstructFromIterators)->Arg(1000)->Arg(10000)->Arg(100000);

/// ---------------------------------------------------------------------------
/// Benchmarks for fermat::PriorityQueue
/// ---------------------------------------------------------------------------

/// BM_FermatPriorityQueue_PushPop: Measure push and pop operations.
static void BM_FermatPriorityQueue_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);

    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq;   ///< Alignment = 0 (default).
        for (int v : data) pq.push(v);
        for (size_t i = 0; i < n; ++i) pq.pop();
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_FermatPriorityQueue_PushPop)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_FermatPriorityQueue_ConstructFromIterators: Construct queue from iterator range.
static void BM_FermatPriorityQueue_ConstructFromIterators(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);

    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq(data.begin(), data.end());
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatPriorityQueue_ConstructFromIterators)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_FermatPriorityQueue_ChangeRemove: Measure change and remove extensions.
static void BM_FermatPriorityQueue_ChangeRemove(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);
    // Choose a random index to modify or remove.
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> idx_dist(0, n - 1);

    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq(data.begin(), data.end());
        size_t idx = idx_dist(gen);
        // For change: we need to modify the value at index and then call change.
        // However, we don't have direct access to the underlying container,
        // so we use get_container().
        pq.get_container()[idx] = -pq.get_container()[idx];   ///< Modify priority.
        pq.change(idx);
        pq.remove(idx_dist(gen));   ///< Remove another random element.
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_FermatPriorityQueue_ChangeRemove)->Arg(1000)->Arg(10000)->Arg(100000);

/// BM_FermatPriorityQueue_GetContainer: Access underlying container.
static void BM_FermatPriorityQueue_GetContainer(benchmark::State& state) {
    const size_t n = state.range(0);
    fermat::PriorityQueue<int, 0> pq;
    for (size_t i = 0; i < n; ++i) pq.push(static_cast<int>(i));

    for (auto _ : state) {
        auto& cont = pq.get_container();
        benchmark::DoNotOptimize(cont);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatPriorityQueue_GetContainer)->Arg(1000)->Arg(10000)->Arg(100000);

BENCHMARK_MAIN();
