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
#include <fermat/memory/object_pool.h>
#include <vector>
#include <memory>

namespace fermat {

// A non‑trivial type to make allocation/deallocation realistic.
struct Payload {
    size_t data[8];
    Payload() noexcept = default;
    explicit Payload(size_t v) { data[0] = v; }
};

// -----------------------------------------------------------------------------
// Benchmark: ObjectPool::get_uninitialize / put_raw
// -----------------------------------------------------------------------------

static void BM_ObjectPool_SingleThread(benchmark::State& state) {
    using Pool = ObjectPool<Payload,0>;
    for (auto _ : state) {
        Payload* p = Pool::get_uninitialize();
        benchmark::DoNotOptimize(p);
        Pool::put_raw(p);
    }
}
BENCHMARK(BM_ObjectPool_SingleThread);

// With construction/destruction (get/put)
static void BM_ObjectPool_ConstructDestruct(benchmark::State& state) {
    using Pool = ObjectPool<Payload, 0>;
    for (auto _ : state) {
        Payload* p = Pool::get(42);
        benchmark::DoNotOptimize(p);
        Pool::put(p);
    }
}
BENCHMARK(BM_ObjectPool_ConstructDestruct);

// -----------------------------------------------------------------------------
// Baseline: direct new/delete (no pool)
// -----------------------------------------------------------------------------

static void BM_NewDelete_SingleThread(benchmark::State& state) {
    for (auto _ : state) {
        Payload* p = new Payload(42);
        benchmark::DoNotOptimize(p);
        delete p;
    }
}
BENCHMARK(BM_NewDelete_SingleThread);

// -----------------------------------------------------------------------------
// Multi‑threaded benchmarks (using thread‑local caches of ObjectPool)
// -----------------------------------------------------------------------------

static void BM_ObjectPool_MultiThread(benchmark::State& state) {
    using Pool = ObjectPool<Payload, 0>;
    for (auto _ : state) {
        Payload* p = Pool::get_uninitialize();
        benchmark::DoNotOptimize(p);
        Pool::put_raw(p);
    }
}
BENCHMARK(BM_ObjectPool_MultiThread)->Threads(4)->Threads(8)->Threads(16);

static void BM_NewDelete_MultiThread(benchmark::State& state) {
    for (auto _ : state) {
        Payload* p = new Payload(42);
        benchmark::DoNotOptimize(p);
        delete p;
    }
}
BENCHMARK(BM_NewDelete_MultiThread)->Threads(4)->Threads(8)->Threads(16);

// -----------------------------------------------------------------------------
// Mixed: allocate many objects at once (simulating batch allocation)
// -----------------------------------------------------------------------------

static void BM_ObjectPool_Batch(benchmark::State& state) {
    using Pool = ObjectPool<Payload, 0>;
    constexpr size_t BATCH = 128;
    std::vector<Payload*> vec;
    vec.reserve(BATCH);
    for (auto _ : state) {
        for (size_t i = 0; i < BATCH; ++i) {
            vec.push_back(Pool::get_uninitialize());
        }
        for (Payload* p : vec) {
            Pool::put_raw(p);
        }
        vec.clear();
    }
}
BENCHMARK(BM_ObjectPool_Batch);

static void BM_NewDelete_Batch(benchmark::State& state) {
    constexpr size_t BATCH = 128;
    std::vector<Payload*> vec;
    vec.reserve(BATCH);
    for (auto _ : state) {
        for (size_t i = 0; i < BATCH; ++i) {
            vec.push_back(new Payload(42));
        }
        for (Payload* p : vec) {
            delete p;
        }
        vec.clear();
    }
}
BENCHMARK(BM_NewDelete_Batch);

} // namespace fermat

BENCHMARK_MAIN();