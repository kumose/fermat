// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.

#include <benchmark/benchmark.h>
#include <atomic>
#include <thread>
#include <vector>

#include <fermat/memory/resource_pool.h>

namespace fermat {

// Define static members needed for ThreadShard (used by sharded pool)
std::atomic<int32_t> ThreadShard::g_thread_shard_id{0};

// Test object type (trivially constructible/destructible)
struct TestObject {
    int64_t data;
    TestObject() : data(0) {}
    explicit TestObject(int64_t v) : data(v) {}
};

// Type aliases for both pool implementations
using ShardedPool = ResourcePool<TestObject, 8, 64, 1024, 64>;

// Helper to allocate and immediately release an object (warm up)
template<typename Pool>
static void BM_AllocFree(benchmark::State& state) {
    for (auto _ : state) {
        uint64_t rid;
        TestObject* obj = Pool::get_uninitialize(rid);
        if (obj) {
            ::new (obj) TestObject(1);
            benchmark::DoNotOptimize(obj);
            Pool::put_raw(rid);
        } else {
            state.SkipWithError("Allocation failed");
        }
    }
    state.SetItemsProcessed(state.iterations());
}

// Benchmark allocate+free (with construction/destruction)
template<typename Pool>
static void BM_GetPut(benchmark::State& state) {
    for (auto _ : state) {
        uint64_t rid;
        TestObject* obj = Pool::get(rid, 42);
        if (obj) {
            benchmark::DoNotOptimize(obj);
            Pool::put(rid);
        } else {
            state.SkipWithError("Allocation failed");
        }
    }
    state.SetItemsProcessed(state.iterations());
}

// Multi-threaded benchmark: each thread repeatedly allocates and frees
template<typename Pool>
static void BM_MultiThread(benchmark::State& state) {
    const int num_threads = state.range(0);
    std::atomic<bool> start{false};
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&] {
            while (!start.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            for (auto _ : state) {
                uint64_t rid;
                TestObject* obj = Pool::get(rid, t);
                if (obj) {
                    benchmark::DoNotOptimize(obj);
                    Pool::put(rid);
                } else {
                    state.SkipWithError("Allocation failed");
                }
            }
        });
    }

    start = true;
    for (auto& th : threads) {
        th.join();
    }
    state.SetItemsProcessed(state.iterations() * num_threads);
}

// Register benchmarks for ShardedPool
BENCHMARK_TEMPLATE(BM_AllocFree, ShardedPool)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_GetPut, ShardedPool)->Unit(benchmark::kNanosecond);
BENCHMARK_TEMPLATE(BM_MultiThread, ShardedPool)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();


// Optional: compare under contention by pinning threads to cores? Not needed.

}  // namespace fermat

BENCHMARK_MAIN();