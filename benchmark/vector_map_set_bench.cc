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

#include <benchmark/benchmark.h>
#include <fermat/container/vector_map.h>
#include <fermat/container/vector_multimap.h>
#include <fermat/container/vector_set.h>
#include <fermat/container/vector_multiset.h>
#include <map>
#include <set>
#include <random>
#include <algorithm>
#include <vector>

namespace fermat_bench {
/// Small-scale batch build + sort + unique for VectorMap<std::string, int>
static void BM_VectorMap_String_BatchBuild(benchmark::State& state) {
    const size_t n = state.range(0);  // 100
    std::vector<std::string> keys;
    keys.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        keys.emplace_back("key_" + std::to_string(i));
    }
    std::shuffle(keys.begin(), keys.end(), std::mt19937{std::random_device{}()});
    std::vector<int> values(n);
    std::iota(values.begin(), values.end(), 0);
    std::shuffle(values.begin(), values.end(), std::mt19937{std::random_device{}()});

    for (auto _ : state) {
        fermat::VectorMap<std::string, int> map;
        // Correct way: push_back_unsorted(key, value)
        for (size_t i = 0; i < n; ++i) {
            auto key = keys[i];
            map.emplace_back_unsorted(key, values[i]);
        }
        std::sort(map.begin(), map.end(), map.value_comp());
        map.erase(std::unique(map.begin(), map.end(),
            [](const auto& a, const auto& b) { return a.first == b.first; }),
            map.end());
        benchmark::DoNotOptimize(static_cast<const void*>(&map));
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_VectorMap_String_BatchBuild)->Arg(100)->Name("VectorMap/string_batch_build/100");

/// Direct insertion (ordered) for VectorMap<std::string, int>
static void BM_VectorMap_String_InsertOrdered(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _ : state) {
        fermat::VectorMap<std::string, int> map;
        for (size_t i = 0; i < n; ++i) {
            // Use emplace to avoid overload ambiguity
            map.emplace("key_" + std::to_string(i), static_cast<int>(i));
        }
        benchmark::DoNotOptimize(static_cast<const void*>(&map));
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_VectorMap_String_InsertOrdered)->Arg(100)->Name("VectorMap/string_insert_ordered/100");

/// Direct insertion for std::map<std::string, int> as baseline
static void BM_StdMap_String_InsertOrdered(benchmark::State& state) {
    const size_t n = state.range(0);
    for (auto _ : state) {
        std::map<std::string, int> map;
        for (size_t i = 0; i < n; ++i) {
            map.emplace("key_" + std::to_string(i), static_cast<int>(i));
        }
        benchmark::DoNotOptimize(static_cast<const void*>(&map));
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdMap_String_InsertOrdered)->Arg(100)->Name("StdMap/string_insert_ordered/100");

/// Also test VectorSet<std::string> batch build
static void BM_VectorSet_String_BatchBuild(benchmark::State& state) {
    const size_t n = state.range(0);
    std::vector<std::string> keys;
    keys.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        keys.emplace_back("key_" + std::to_string(i));
    }
    std::shuffle(keys.begin(), keys.end(), std::mt19937{std::random_device{}()});

    for (auto _ : state) {
        fermat::VectorSet<std::string> set;
        for (const auto& k : keys) {
            set.push_back_unsorted(k);
        }
        std::sort(set.begin(), set.end(), set.value_comp());
        set.erase(std::unique(set.begin(), set.end()), set.end());
        benchmark::DoNotOptimize(static_cast<const void*>(&set));
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_VectorSet_String_BatchBuild)->Arg(100)->Name("VectorSet/string_batch_build/100");


    // Generate a vector of unique random integers
    std::vector<int> GenerateUniqueKeys(size_t n) {
        std::vector<int> keys(n);
        std::iota(keys.begin(), keys.end(), 0);
        std::shuffle(keys.begin(), keys.end(), std::mt19937{std::random_device{}()});
        return keys;
    }

    // Generate a vector of random integers (may contain duplicates for multiset tests)
    std::vector<int> GenerateRandomKeys(size_t n, bool unique = true) {
        if (unique) return GenerateUniqueKeys(n);
        std::vector<int> keys(n);
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, static_cast<int>(n * 1.5));
        for (size_t i = 0; i < n; ++i) keys[i] = dist(rng);
        return keys;
    }

    // ============================================================================
    // VectorMap vs std::map benchmarks (unique keys)
    // ============================================================================

    template<typename Map>
    void BM_InsertOrdered(benchmark::State &state) {
        size_t n = state.range(0);
        for (auto _: state) {
            Map m;
            for (size_t i = 0; i < n; ++i) {
                m.insert({static_cast<int>(i), i});
            }
            benchmark::DoNotOptimize(m);
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Map>
    void BM_InsertRandom(benchmark::State &state) {
        size_t n = state.range(0);
        auto keys = GenerateUniqueKeys(n);
        for (auto _: state) {
            Map m;
            for (size_t i = 0; i < n; ++i) {
                m.insert({keys[i], keys[i]});
            }
            benchmark::DoNotOptimize(m);
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Map>
    void BM_FindExisting(benchmark::State &state) {
        size_t n = state.range(0);
        Map m;
        for (size_t i = 0; i < n; ++i) {
            m.insert({static_cast<int>(i), i});
        }
        std::vector<int> keys = GenerateUniqueKeys(n);
        for (auto _: state) {
            for (int k: keys) {
                auto it = m.find(k);
                benchmark::DoNotOptimize(it);
            }
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Map>
    void BM_FindMissing(benchmark::State &state) {
        size_t n = state.range(0);
        Map m;
        for (size_t i = 0; i < n; ++i) {
            m.insert({static_cast<int>(i), i});
        }
        // Search for keys that are not present (n+1 .. 2n)
        for (auto _: state) {
            for (size_t i = 0; i < n; ++i) {
                auto it = m.find(static_cast<int>(n + i));
                benchmark::DoNotOptimize(it);
            }
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Map>
    void BM_Iterate(benchmark::State &state) {
        size_t n = state.range(0);
        Map m;
        for (size_t i = 0; i < n; ++i) {
            m.insert({static_cast<int>(i), i});
        }
        for (auto _: state) {
            for (auto &p: m) {
                benchmark::DoNotOptimize(static_cast<const void*>(&p));
            }
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Map>
    void BM_EraseByKey(benchmark::State &state) {
        size_t n = state.range(0);
        auto keys = GenerateUniqueKeys(n);
        for (auto _: state) {
            Map m;
            for (size_t i = 0; i < n; ++i) {
                m.insert({keys[i], keys[i]});
            }
            state.PauseTiming();
            // Create a copy of keys to erase (we'll erase all in random order)
            std::vector<int> erase_order = keys;
            std::shuffle(erase_order.begin(), erase_order.end(), std::mt19937{std::random_device{}()});
            state.ResumeTiming();
            for (int k: erase_order) {
                m.erase(k);
            }
            benchmark::DoNotOptimize(m);
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    using VectorMapIntInt = fermat::VectorMap<int, int>;
    using StdMapIntInt = std::map<int, int>;
    using VectorSetInt = fermat::VectorSet<int>;
    using StdSetInt = std::set<int>;

    // Register benchmarks for VectorMap and std::map
#define BENCHMARK_MAP(map_type, name) \
    BENCHMARK_TEMPLATE(BM_InsertOrdered, map_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/insert_ordered"); \
    BENCHMARK_TEMPLATE(BM_InsertRandom, map_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/insert_random"); \
    BENCHMARK_TEMPLATE(BM_FindExisting, map_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/find_existing"); \
    BENCHMARK_TEMPLATE(BM_FindMissing, map_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/find_missing"); \
    BENCHMARK_TEMPLATE(BM_Iterate, map_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/iterate"); \
    BENCHMARK_TEMPLATE(BM_EraseByKey, map_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/erase_by_key");

    BENCHMARK_MAP(VectorMapIntInt, "VectorMap");
    BENCHMARK_MAP(StdMapIntInt, "StdMap");


    // ============================================================================
    // VectorSet vs std::set benchmarks (unique keys)
    // ============================================================================

    template<typename Set>
    void BM_SetInsertOrdered(benchmark::State &state) {
        size_t n = state.range(0);
        for (auto _: state) {
            Set s;
            for (size_t i = 0; i < n; ++i) {
                s.insert(static_cast<int>(i));
            }
            benchmark::DoNotOptimize(s);
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Set>
    void BM_SetInsertRandom(benchmark::State &state) {
        size_t n = state.range(0);
        auto keys = GenerateUniqueKeys(n);
        for (auto _: state) {
            Set s;
            for (int k: keys) {
                s.insert(k);
            }
            benchmark::DoNotOptimize(s);
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Set>
    void BM_SetFindExisting(benchmark::State &state) {
        size_t n = state.range(0);
        Set s;
        for (size_t i = 0; i < n; ++i) {
            s.insert(static_cast<int>(i));
        }
        auto keys = GenerateUniqueKeys(n);
        for (auto _: state) {
            for (int k: keys) {
                auto it = s.find(k);
                benchmark::DoNotOptimize(it);
            }
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Set>
    void BM_SetFindMissing(benchmark::State &state) {
        size_t n = state.range(0);
        Set s;
        for (size_t i = 0; i < n; ++i) {
            s.insert(static_cast<int>(i));
        }
        for (auto _: state) {
            for (size_t i = 0; i < n; ++i) {
                auto it = s.find(static_cast<int>(n + i));
                benchmark::DoNotOptimize(it);
            }
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Set>
    void BM_SetIterate(benchmark::State &state) {
        size_t n = state.range(0);
        Set s;
        for (size_t i = 0; i < n; ++i) {
            s.insert(static_cast<int>(i));
        }
        for (auto _: state) {
            for (auto &v: s) {
                benchmark::DoNotOptimize(static_cast<const void*>(&v));
            }
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

    template<typename Set>
    void BM_SetEraseByKey(benchmark::State &state) {
        size_t n = state.range(0);
        auto keys = GenerateUniqueKeys(n);
        for (auto _: state) {
            Set s;
            for (int k: keys) {
                s.insert(k);
            }
            state.PauseTiming();
            std::vector<int> erase_order = keys;
            std::shuffle(erase_order.begin(), erase_order.end(), std::mt19937{std::random_device{}()});
            state.ResumeTiming();
            for (int k: erase_order) {
                s.erase(k);
            }
            benchmark::DoNotOptimize(s);
        }
        state.SetItemsProcessed(state.iterations() * n);
    }

#define BENCHMARK_SET(set_type, name) \
    BENCHMARK_TEMPLATE(BM_SetInsertOrdered, set_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/insert_ordered"); \
    BENCHMARK_TEMPLATE(BM_SetInsertRandom, set_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/insert_random"); \
    BENCHMARK_TEMPLATE(BM_SetFindExisting, set_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/find_existing"); \
    BENCHMARK_TEMPLATE(BM_SetFindMissing, set_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/find_missing"); \
    BENCHMARK_TEMPLATE(BM_SetIterate, set_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/iterate"); \
    BENCHMARK_TEMPLATE(BM_SetEraseByKey, set_type)->Arg(1000)->Arg(10000)->Arg(100000)->Arg(500000)->Name(name "/erase_by_key");

    BENCHMARK_SET(fermat::VectorSet<int>, "VectorSet");
    BENCHMARK_SET(std::set<int>, "StdSet");
} // namespace fermat_bench

BENCHMARK_MAIN();
