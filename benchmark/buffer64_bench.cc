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
#include <vector>
#include <fermat/container/buffer.h>
#include <random>
#include <algorithm>



// Helper: generate deterministic data of given size
static std::vector<int> GenerateData(size_t n) {
    std::vector<int> data(n);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 1000000);
    for (auto& v : data) v = dist(rng);
    return data;
}

// 1. Construction from size (value-initialized)
template<typename Vec>
static void BM_ConstructSize(benchmark::State& state) {
    size_t n = state.range(0);
    for (auto _ : state) {
        Vec v(n);
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(n * state.iterations());
}
BENCHMARK_TEMPLATE(BM_ConstructSize, std::vector<int>)->RangeMultiplier(2)->Range(4, 1024);
BENCHMARK_TEMPLATE(BM_ConstructSize, fermat::Buffer<int, 64>)->RangeMultiplier(2)->Range(4, 1024);

// 2. Push back (sequential) – small capacity growth
template<typename Vec>
static void BM_PushBackSmall(benchmark::State& state) {
    size_t n = state.range(0);
    for (auto _ : state) {
        Vec v;
        v.reserve(n);  // avoid reallocation to measure pure push_back cost
        for (size_t i = 0; i < n; ++i) {
            v.push_back(static_cast<int>(i));
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(n * state.iterations());
}

template<typename Vec>
static void BM_AppendBackSmall(benchmark::State& state) {
    size_t n = state.range(0);
    for (auto _ : state) {
        Vec v;
        v.reserve(n);  // avoid reallocation to measure pure push_back cost
        for (size_t i = 0; i < n; ++i) {
            v.append(static_cast<int>(i));
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(n * state.iterations());
}
BENCHMARK_TEMPLATE(BM_PushBackSmall, std::vector<int>)->RangeMultiplier(2)->Range(4, 1024);
BENCHMARK_TEMPLATE(BM_AppendBackSmall, fermat::Buffer<int, 64>)->RangeMultiplier(2)->Range(4, 1024);

// 3. Emplace back (construct in place)
// no emplace
// 4. Sequential iteration (read)
template<typename Vec>
static void BM_IterationSmall(benchmark::State& state) {
    size_t n = state.range(0);
    auto data = GenerateData(n);
    volatile long sum = 0;
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        for (const auto& x : v) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(n * state.iterations());
}
BENCHMARK_TEMPLATE(BM_IterationSmall, std::vector<int>)->RangeMultiplier(2)->Range(4, 1024);
BENCHMARK_TEMPLATE(BM_IterationSmall, fermat::Buffer<int, 64>)->RangeMultiplier(2)->Range(4, 1024);

// 5. Random access (operator[])
template<typename Vec>
static void BM_RandomAccessSmall(benchmark::State& state) {
    size_t n = state.range(0);
    auto data = GenerateData(n);
    Vec v(data.begin(), data.end());
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    volatile int sum = 0;
    for (auto _ : state) {
        for (size_t i = 0; i < 100; ++i) {
            sum += v[dist(rng)];
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(100 * state.iterations());
}
BENCHMARK_TEMPLATE(BM_RandomAccessSmall, std::vector<int>)->RangeMultiplier(2)->Range(4, 1024);
BENCHMARK_TEMPLATE(BM_RandomAccessSmall, fermat::Buffer<int, 64>)->RangeMultiplier(2)->Range(4, 1024);

// 6. Insert at middle (copy)
template<typename Vec>
static void BM_InsertMiddleSmall(benchmark::State& state) {
    size_t n = state.range(0);
    auto data = GenerateData(n);
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        auto pos = v.begin() + v.size() / 2;
        for (int i = 0; i < 10; ++i) {  // small number of inserts
            v.insert(pos, i);
            pos = v.begin() + v.size() / 2; // keep middle
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(10 * state.iterations());
}
BENCHMARK_TEMPLATE(BM_InsertMiddleSmall, std::vector<int>)->RangeMultiplier(2)->Range(4, 1024);
BENCHMARK_TEMPLATE(BM_InsertMiddleSmall, fermat::Buffer<int, 64>)->RangeMultiplier(2)->Range(4, 1024);

// 7. Erase at middle
template<typename Vec>
static void BM_EraseMiddleSmall(benchmark::State& state) {
    size_t n = state.range(0);
    auto data = GenerateData(n);
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        for (int i = 0; i < 10 && v.size() > 0; ++i) {
            auto pos = v.begin() + v.size() / 2;
            v.erase(pos);
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(10 * state.iterations());
}
BENCHMARK_TEMPLATE(BM_EraseMiddleSmall, std::vector<int>)->RangeMultiplier(2)->Range(4, 1024);
BENCHMARK_TEMPLATE(BM_EraseMiddleSmall, fermat::Buffer<int, 64>)->RangeMultiplier(2)->Range(4, 1024);

// 8. Clear and shrink_to_fit
template<typename Vec>
static void BM_ClearShrinkSmall(benchmark::State& state) {
    size_t n = state.range(0);
    auto data = GenerateData(n);
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        v.clear();
        v.shrink_to_fit();
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(n * state.iterations());
}
BENCHMARK_TEMPLATE(BM_ClearShrinkSmall, std::vector<int>)->RangeMultiplier(2)->Range(4, 1024);
BENCHMARK_TEMPLATE(BM_ClearShrinkSmall, fermat::Buffer<int, 64>)->RangeMultiplier(2)->Range(4, 1024);

// Helper to generate random integers
static std::vector<int> GenerateRandomData(size_t n) {
    std::vector<int> data(n);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(1, 1000000);
    for (auto& v : data) v = dist(rng);
    return data;
}

// -----------------------------------------------------------------------------
// 1. Push back (sequential)
// -----------------------------------------------------------------------------
template<typename Vec>
static void BM_PushBack(benchmark::State& state) {
    for (auto _ : state) {
        Vec v;
        v.reserve(state.range(0));
        for (int i = 0; i < state.range(0); ++i) {
            v.push_back(i);
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(state.range(0) * state.iterations());
}

template<typename Vec>
static void BM_Append(benchmark::State& state) {
    for (auto _ : state) {
        Vec v;
        v.reserve(state.range(0));
        for (int i = 0; i < state.range(0); ++i) {
            v.append(i);
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(state.range(0) * state.iterations());
}

BENCHMARK_TEMPLATE(BM_PushBack, std::vector<int>)->Arg(1000)->Arg(10000)->Arg(100000);
BENCHMARK_TEMPLATE(BM_Append, fermat::Buffer<int, 64>)->Arg(1000)->Arg(10000)->Arg(100000);

// -----------------------------------------------------------------------------
// 2. Emplace back (with construction)
// -----------------------------------------------------------------------------
// emplace

// -----------------------------------------------------------------------------
// 3. Sequential iteration (read)
// -----------------------------------------------------------------------------
template<typename Vec>
static void BM_Iteration(benchmark::State& state) {
    Vec v;
    v.resize(state.range(0));
    for (size_t i = 0; i < v.size(); ++i) v[i] = static_cast<int>(i);
    volatile int sum = 0;
    for (auto _ : state) {
        for (const auto& x : v) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.range(0) * state.iterations());
}

BENCHMARK_TEMPLATE(BM_Iteration, std::vector<int>)->Arg(1000)->Arg(10000)->Arg(100000);
BENCHMARK_TEMPLATE(BM_Iteration, fermat::Buffer<int, 64>)->Arg(1000)->Arg(10000)->Arg(100000);

// -----------------------------------------------------------------------------
// 4. Random access (operator[])
// -----------------------------------------------------------------------------
template<typename Vec>
static void BM_RandomAccess(benchmark::State& state) {
    Vec v;
    v.resize(state.range(0));
    for (size_t i = 0; i < v.size(); ++i) v[i] = static_cast<int>(i);
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> dist(0, v.size() - 1);
    volatile int sum = 0;
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            sum += v[dist(rng)];
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(1000 * state.iterations());
}

BENCHMARK_TEMPLATE(BM_RandomAccess, std::vector<int>)->Arg(10000)->Arg(100000);
BENCHMARK_TEMPLATE(BM_RandomAccess, fermat::Buffer<int, 64>)->Arg(10000)->Arg(100000);

// -----------------------------------------------------------------------------
// 5. Insert at middle (copy)
// -----------------------------------------------------------------------------
template<typename Vec>
static void BM_InsertMiddle(benchmark::State& state) {
    auto data = GenerateRandomData(state.range(0));
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        auto pos = v.begin() + v.size() / 2;
        for (int i = 0; i < 100; ++i) {
            v.insert(pos, i);
            pos = v.begin() + v.size() / 2; // keep middle
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(100 * state.iterations());
}

BENCHMARK_TEMPLATE(BM_InsertMiddle, std::vector<int>)->Arg(1000)->Arg(10000);
BENCHMARK_TEMPLATE(BM_InsertMiddle, fermat::Buffer<int, 64>)->Arg(1000)->Arg(10000);

// -----------------------------------------------------------------------------
// 6. Erase at middle
// -----------------------------------------------------------------------------
template<typename Vec>
static void BM_EraseMiddle(benchmark::State& state) {
    auto data = GenerateRandomData(state.range(0));
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        for (int i = 0; i < 100; ++i) {
            auto pos = v.begin() + v.size() / 2;
            v.erase(pos);
        }
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(100 * state.iterations());
}

BENCHMARK_TEMPLATE(BM_EraseMiddle, std::vector<int>)->Arg(1000)->Arg(10000);
BENCHMARK_TEMPLATE(BM_EraseMiddle, fermat::Buffer<int, 64>)->Arg(1000)->Arg(10000);

// -----------------------------------------------------------------------------
// 7. Sorting
// -----------------------------------------------------------------------------
template<typename Vec>
static void BM_Sort(benchmark::State& state) {
    auto data = GenerateRandomData(state.range(0));
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        std::sort(v.begin(), v.end());
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(state.range(0) * state.iterations());
}

BENCHMARK_TEMPLATE(BM_Sort, std::vector<int>)->Arg(10000)->Arg(100000);
BENCHMARK_TEMPLATE(BM_Sort, fermat::Buffer<int, 64>)->Arg(10000)->Arg(100000);

// -----------------------------------------------------------------------------
// 8. Clear & reallocate (capacity shrink)
// -----------------------------------------------------------------------------
template<typename Vec>
static void BM_ClearAndShrink(benchmark::State& state) {
    auto data = GenerateRandomData(state.range(0));
    for (auto _ : state) {
        Vec v(data.begin(), data.end());
        v.clear();
        v.shrink_to_fit();
        benchmark::DoNotOptimize(v);
    }
    state.SetItemsProcessed(state.range(0) * state.iterations());
}

BENCHMARK_TEMPLATE(BM_ClearAndShrink, std::vector<int>)->Arg(10000)->Arg(100000);
BENCHMARK_TEMPLATE(BM_ClearAndShrink, fermat::Buffer<int, 64>)->Arg(10000)->Arg(100000);


// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------
BENCHMARK_MAIN();
