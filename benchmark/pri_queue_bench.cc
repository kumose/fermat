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
// #include "fermat/container/sorted_queue.h"

static constexpr size_t kMaxRandomDataSize = 100000;

static const std::vector<int>& GetGlobalRandomData() {
    static const std::vector<int> pool = [] {
        std::vector<int> data(kMaxRandomDataSize);
        std::mt19937 gen(42);
        std::uniform_int_distribution<int> dist(0, static_cast<int>(kMaxRandomDataSize * 10));
        std::generate(data.begin(), data.end(), [&]() { return dist(gen); });
        return data;
    }();
    return pool;
}

#define BENCHMARK_PQ_SIZES(Bench) \
    BENCHMARK(Bench)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(600)->Arg(700)->Arg(800)->Arg(900)->Arg(1000)->Arg(2000)->Arg(10000)->Arg(100000)

#define BENCHMARK_PQ_SIZES_2K(Bench) \
    BENCHMARK(Bench)->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(600)->Arg(700)->Arg(800)->Arg(900)->Arg(1000)->Arg(2000)

static constexpr size_t kBoundedStreamOps = 2000;

// ============================================================================
// std::priority_queue
// ============================================================================

static void BM_StdPriorityQueue_Push(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        std::priority_queue<int> pq;
        for (size_t i = 0; i < n; ++i) {
            pq.push(data[i]);
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_PQ_SIZES(BM_StdPriorityQueue_Push);

static void BM_StdPriorityQueue_Pop(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        std::priority_queue<int> pq(
            std::less<int>(),
            std::vector<int>(data.begin(), data.begin() + static_cast<ptrdiff_t>(n)));
        state.ResumeTiming();
        for (size_t i = 0; i < n; ++i) {
            pq.pop();
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_PQ_SIZES(BM_StdPriorityQueue_Pop);

static void BM_StdPriorityQueue_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        std::priority_queue<int> pq;
        for (size_t i = 0; i < n; ++i) {
            pq.push(data[i]);
        }
        for (size_t i = 0; i < n; ++i) {
            pq.pop();
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK_PQ_SIZES(BM_StdPriorityQueue_PushPop);

static void BM_StdPriorityQueue_ConstructFromIterators(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        std::priority_queue<int> pq(std::less<int>(),
                                    std::vector<int>(data.begin(), data.begin() + static_cast<ptrdiff_t>(n)));
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdPriorityQueue_ConstructFromIterators)->Arg(1000)->Arg(10000)->Arg(100000);

// ============================================================================
// fermat::PriorityQueue
// ============================================================================

static void BM_FermatPriorityQueue_Push(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq;
        for (size_t i = 0; i < n; ++i) {
            pq.push(data[i]);
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_PQ_SIZES(BM_FermatPriorityQueue_Push);

static void BM_FermatPriorityQueue_Pop(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        fermat::PriorityQueue<int, 0> pq(data.begin(), data.begin() + static_cast<ptrdiff_t>(n));
        state.ResumeTiming();
        for (size_t i = 0; i < n; ++i) {
            pq.pop();
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_PQ_SIZES(BM_FermatPriorityQueue_Pop);

static void BM_FermatPriorityQueue_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq;
        for (size_t i = 0; i < n; ++i) {
            pq.push(data[i]);
        }
        for (size_t i = 0; i < n; ++i) {
            pq.pop();
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK_PQ_SIZES(BM_FermatPriorityQueue_PushPop);

static void BM_FermatPriorityQueue_ConstructFromIterators(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq(data.begin(), data.begin() + static_cast<ptrdiff_t>(n));
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatPriorityQueue_ConstructFromIterators)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_FermatPriorityQueue_ChangeRemove(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    const size_t change_idx = n / 4;
    const size_t remove_idx = n / 2;
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq(data.begin(), data.begin() + static_cast<ptrdiff_t>(n));
        pq.get_container()[change_idx] = -pq.get_container()[change_idx];
        pq.change(change_idx);
        pq.remove(remove_idx);
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_FermatPriorityQueue_ChangeRemove)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_FermatPriorityQueue_GetContainer(benchmark::State& state) {
    const size_t n = state.range(0);
    fermat::PriorityQueue<int, 0> pq;
    for (size_t i = 0; i < n; ++i) {
        pq.push(static_cast<int>(i));
    }
    for (auto _ : state) {
        auto& cont = pq.get_container();
        benchmark::DoNotOptimize(cont);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatPriorityQueue_GetContainer)->Arg(1000)->Arg(10000)->Arg(100000);

// ============================================================================
// Bounded top-K stream (2000 ops, cap = Arg), n <= 2000
// ============================================================================

static void BM_FermatPriorityQueue_BoundedPushPop(benchmark::State& state) {
    const size_t limit = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq;
        for (size_t i = 0; i < kBoundedStreamOps; ++i) {
            pq.push(data[i]);
            if (pq.size() > limit)
                pq.pop();
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kBoundedStreamOps);
}
BENCHMARK_PQ_SIZES_2K(BM_FermatPriorityQueue_BoundedPushPop);

static void BM_StdPriorityQueue_BoundedPushPop(benchmark::State& state) {
    const size_t limit = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        std::priority_queue<int> pq;
        for (size_t i = 0; i < kBoundedStreamOps; ++i) {
            pq.push(data[i]);
            if (pq.size() > limit)
                pq.pop();
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kBoundedStreamOps);
}
BENCHMARK_PQ_SIZES_2K(BM_StdPriorityQueue_BoundedPushPop);

static void BM_FermatPriorityQueue_BoundedPushPop200(benchmark::State& state) {
    constexpr size_t kLimit = 200;
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::PriorityQueue<int, 0> pq;
        for (size_t i = 0; i < kBoundedStreamOps; ++i) {
            pq.push(data[i]);
            if (pq.size() > kLimit)
                pq.pop();
        }
        benchmark::DoNotOptimize(pq);
    }
    state.SetItemsProcessed(state.iterations() * kBoundedStreamOps);
}
BENCHMARK(BM_FermatPriorityQueue_BoundedPushPop200);

#if 0
// ============================================================================
// fermat::SortedQueue (sorted array; pop_back == max, like priority_queue::pop)
// Disabled: scenario-specific container, not compared in this benchmark suite.
// ============================================================================

static void BM_FermatSortedQueue_Push(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::SortedQueue<int, 0> sq;
        for (size_t i = 0; i < n; ++i) {
            sq.push(data[i]);
        }
        benchmark::DoNotOptimize(sq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_PQ_SIZES(BM_FermatSortedQueue_Push);

static void BM_FermatSortedQueue_Pop(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        state.PauseTiming();
        fermat::SortedQueue<int, 0> sq(data.begin(), data.begin() + static_cast<ptrdiff_t>(n));
        state.ResumeTiming();
        for (size_t i = 0; i < n; ++i) {
            sq.pop_back();
        }
        benchmark::DoNotOptimize(sq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_PQ_SIZES(BM_FermatSortedQueue_Pop);

static void BM_FermatSortedQueue_PushPop(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::SortedQueue<int, 0> sq;
        for (size_t i = 0; i < n; ++i) {
            sq.push(data[i]);
        }
        for (size_t i = 0; i < n; ++i) {
            sq.pop_back();
        }
        benchmark::DoNotOptimize(sq);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK_PQ_SIZES(BM_FermatSortedQueue_PushPop);

static void BM_FermatSortedQueue_ConstructFromIterators(benchmark::State& state) {
    const size_t n = state.range(0);
    const auto& data = GetGlobalRandomData();
    for (auto _ : state) {
        fermat::SortedQueue<int, 0> sq(data.begin(), data.begin() + static_cast<ptrdiff_t>(n));
        benchmark::DoNotOptimize(sq);
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_FermatSortedQueue_ConstructFromIterators)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_FermatSortedQueue_BoundedPushPop200(benchmark::State& state) {
    const auto& data = GetGlobalRandomData();
    constexpr size_t kLimit = 200;
    for (auto _ : state) {
        fermat::SortedQueue<int, 0> sq;
        for (size_t i = 0; i < kMaxRandomDataSize; ++i) {
            sq.push(data[i]);
            if (sq.size() > kLimit) {
                sq.pop_front();
            }
        }
        benchmark::DoNotOptimize(sq);
    }
    state.SetItemsProcessed(state.iterations() * kMaxRandomDataSize);
}
BENCHMARK(BM_FermatSortedQueue_BoundedPushPop200);
#endif

BENCHMARK_MAIN();
