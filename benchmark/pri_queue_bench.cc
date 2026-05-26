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
#include <queue>
#include <random>
#include <vector>
#include <algorithm>

#include "fermat/container/priority_queue.h"

static std::vector<int> GenerateRandomData(size_t n) {
    std::vector<int> data(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, static_cast<int>(n * 10));
    std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
    return data;
}

// ============================================================================
// std::priority_queue benchmarks
// ============================================================================

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

// ============================================================================
// fermat::PriorityQueue benchmarks
// ============================================================================

static void BM_FermatPriorityQueue_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq;
        for (int v : data) pq.push(v);
        for (size_t i = 0; i < n; ++i) pq.pop();
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_FermatPriorityQueue_PushPop)->Arg(1000)->Arg(10000)->Arg(100000);

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

// Additional fermat-specific features
static void BM_FermatPriorityQueue_ChangeRemove(benchmark::State& state) {
    const size_t n = state.range(0);
    auto data = GenerateRandomData(n);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> idx_dist(0, n - 1);
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq(data.begin(), data.end());
        size_t idx = idx_dist(gen);
        pq.get_container()[idx] = -pq.get_container()[idx];
        pq.change(idx);
        pq.remove(idx_dist(gen));
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_FermatPriorityQueue_ChangeRemove)->Arg(1000)->Arg(10000)->Arg(100000);

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
