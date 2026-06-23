// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <benchmark/benchmark.h>

#include <fermat/container/heap.h>
#include <fermat/container/priority_queue.h>
#include <fermat/container/vector.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <queue>
#include <random>
#include <vector>

static constexpr size_t kStreamOps = 100000;

static const std::vector<int> &GetGlobalRandomData() {
    static const std::vector<int> pool = [] {
        std::vector<int> data(kStreamOps);
        std::mt19937 gen(42);
        std::uniform_int_distribution<int> dist(0, static_cast<int>(kStreamOps * 10));
        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
        return data;
    }();
    return pool;
}

#define BENCHMARK_K_SIZES(Bench) \
    BENCHMARK(Bench)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(600)->Arg(700)->Arg(800)->Arg(900)->Arg(1000)

using FermatMaxPriorityQueue = fermat::PriorityQueue<int, 0, fermat::Vector<int, 0>, std::greater<int>>;
using FermatMinPriorityQueue = fermat::PriorityQueue<int, 0, fermat::Vector<int, 0>, std::less<int>>;

template<typename Compare>
void RefMaxKPush(std::priority_queue<int, std::vector<int>, Compare> &pq, size_t k, int value) {
    if (k == 0)
        return;
    if (pq.size() >= k) {
        Compare comp;
        if (!comp(value, pq.top()))
            return;
        pq.pop();
    }
    pq.push(value);
}

template<typename Compare>
void RefMinKPush(std::priority_queue<int, std::vector<int>, Compare> &pq, size_t k, int value) {
    if (k == 0)
        return;
    if (pq.size() >= k) {
        Compare comp;
        if (!comp(value, pq.top()))
            return;
        pq.pop();
    }
    pq.push(value);
}

template<typename PQ>
void FermatNaiveBoundedPush(PQ &pq, size_t k, int value) {
    pq.push(value);
    if (pq.size() > k)
        pq.pop();
}

// ============================================================================
// Top-K max stream: MaxKQueue vs priority_queue (K = 50 .. 1000)
// ============================================================================

static void BM_MaxKQueue_BoundedPush(benchmark::State &state) {
    const size_t k = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::MaxKQueue<int> q(k);
        for (size_t i = 0; i < kStreamOps; ++i)
            q.push(data[i]);
        benchmark::DoNotOptimize(q);
    }
    state.SetItemsProcessed(state.iterations() * kStreamOps);
}
BENCHMARK_K_SIZES(BM_MaxKQueue_BoundedPush);

static void BM_StdPriorityQueue_MaxK_BoundedPush(benchmark::State &state) {
    const size_t k = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
        for (size_t i = 0; i < kStreamOps; ++i)
            RefMaxKPush<std::greater<int>>(pq, k, data[i]);
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kStreamOps);
}
BENCHMARK_K_SIZES(BM_StdPriorityQueue_MaxK_BoundedPush);

static void BM_FermatPriorityQueue_MaxK_BoundedPush(benchmark::State &state) {
    const size_t k = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        FermatMaxPriorityQueue pq;
        for (size_t i = 0; i < kStreamOps; ++i) {
            if (pq.size() >= k) {
                std::greater<int> comp;
                if (!comp(data[i], pq.top()))
                    continue;
                pq.pop();
            }
            pq.push(data[i]);
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kStreamOps);
}
BENCHMARK_K_SIZES(BM_FermatPriorityQueue_MaxK_BoundedPush);

static void BM_FermatPriorityQueue_NaiveBoundedPush(benchmark::State &state) {
    const size_t k = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        FermatMinPriorityQueue pq;
        for (size_t i = 0; i < kStreamOps; ++i)
            FermatNaiveBoundedPush(pq, k, data[i]);
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kStreamOps);
}
BENCHMARK_K_SIZES(BM_FermatPriorityQueue_NaiveBoundedPush);

// ============================================================================
// Top-K min stream: MinKQueue vs priority_queue (K = 50 .. 1000)
// ============================================================================

static void BM_MinKQueue_BoundedPush(benchmark::State &state) {
    const size_t k = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::MinKQueue<int> q(k);
        for (size_t i = 0; i < kStreamOps; ++i)
            q.push(data[i]);
        benchmark::DoNotOptimize(q);
    }
    state.SetItemsProcessed(state.iterations() * kStreamOps);
}
BENCHMARK_K_SIZES(BM_MinKQueue_BoundedPush);

static void BM_StdPriorityQueue_MinK_BoundedPush(benchmark::State &state) {
    const size_t k = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        std::priority_queue<int, std::vector<int>, std::less<int>> pq;
        for (size_t i = 0; i < kStreamOps; ++i)
            RefMinKPush<std::less<int>>(pq, k, data[i]);
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kStreamOps);
}
BENCHMARK_K_SIZES(BM_StdPriorityQueue_MinK_BoundedPush);

static void BM_FermatPriorityQueue_MinK_BoundedPush(benchmark::State &state) {
    const size_t k = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        FermatMinPriorityQueue pq;
        for (size_t i = 0; i < kStreamOps; ++i) {
            if (pq.size() >= k) {
                std::less<int> comp;
                if (!comp(data[i], pq.top()))
                    continue;
                pq.pop();
            }
            pq.push(data[i]);
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kStreamOps);
}
BENCHMARK_K_SIZES(BM_FermatPriorityQueue_MinK_BoundedPush);

// ============================================================================
// Drain: same heap, n elements — sort_heap + traverse vs n×pop
// Setup (build heap) in PauseTiming; only export timed.
// MaxKQueue: aes (asc) vs pop (asc).  MinKQueue: des (desc) vs pop (desc).
// Fermat PQ: sort_heap on get_container() vs pop (both max-first / desc).
// ============================================================================

static fermat::MaxKQueue<int> MakeMaxKQueue(size_t n, const std::vector<int> &data) {
    fermat::MaxKQueue<int> q(n);
    for (size_t i = 0; i < n; ++i)
        q.push(data[i]);
    return q;
}

static fermat::MinKQueue<int> MakeMinKQueue(size_t n, const std::vector<int> &data) {
    fermat::MinKQueue<int> q(n);
    for (size_t i = 0; i < n; ++i)
        q.push(data[i]);
    return q;
}

static FermatMinPriorityQueue MakeFermatPQ(size_t n, const std::vector<int> &data) {
    return FermatMinPriorityQueue(data.begin(), data.begin() + static_cast<ptrdiff_t>(n));
}

static void BM_MaxKQueue_SortTraverse(benchmark::State &state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        fermat::MaxKQueue<int> q = MakeMaxKQueue(n, data);
        state.ResumeTiming();

        fermat::Vector<int, 0> c = std::move(q).aes();
        int64_t sum = 0;
        for (int v : c)
            sum += v;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_MaxKQueue_PopLoop(benchmark::State &state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        fermat::MaxKQueue<int> q = MakeMaxKQueue(n, data);
        state.ResumeTiming();

        int64_t sum = 0;
        while (!q.empty()) {
            sum += q.top();
            q.pop();
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_MinKQueue_SortTraverse(benchmark::State &state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        fermat::MinKQueue<int> q = MakeMinKQueue(n, data);
        state.ResumeTiming();

        fermat::Vector<int, 0> c = std::move(q).des();
        int64_t sum = 0;
        for (int v : c)
            sum += v;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_MinKQueue_PopLoop(benchmark::State &state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        fermat::MinKQueue<int> q = MakeMinKQueue(n, data);
        state.ResumeTiming();

        int64_t sum = 0;
        while (!q.empty()) {
            sum += q.top();
            q.pop();
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_FermatPQ_SortTraverse(benchmark::State &state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        FermatMinPriorityQueue pq = MakeFermatPQ(n, data);
        state.ResumeTiming();

        auto &c = pq.get_container();
        fermat::sort_heap(c.begin(), c.end(), pq.comp);
        std::reverse(c.begin(), c.end());
        int64_t sum = 0;
        for (int v : c)
            sum += v;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_FermatPQ_PopLoop(benchmark::State &state) {
    const size_t n = static_cast<size_t>(state.range(0));
    const auto &data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        FermatMinPriorityQueue pq = MakeFermatPQ(n, data);
        state.ResumeTiming();

        int64_t sum = 0;
        while (!pq.empty()) {
            sum += pq.top();
            pq.pop();
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

#define BENCHMARK_DRAIN_SIZES(Bench) \
    BENCHMARK(Bench)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)

BENCHMARK_DRAIN_SIZES(BM_MaxKQueue_SortTraverse);
BENCHMARK_DRAIN_SIZES(BM_MaxKQueue_PopLoop);
BENCHMARK_DRAIN_SIZES(BM_MinKQueue_SortTraverse);
BENCHMARK_DRAIN_SIZES(BM_MinKQueue_PopLoop);
BENCHMARK_DRAIN_SIZES(BM_FermatPQ_SortTraverse);
BENCHMARK_DRAIN_SIZES(BM_FermatPQ_PopLoop);

BENCHMARK_MAIN();
