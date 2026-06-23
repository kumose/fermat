/// @file lru_cache.cc
/// @brief 2x2 matrix: (std::unordered_map, turbo::flat_hash_map) × (std::list, fermat::List)
/// @note Four combinations written separately.

#include <benchmark/benchmark.h>
#include <random>
#include <string>
#include <list>
#include <unordered_map>
#include <turbo/container/flat_hash_map.h>
#include "fermat/container/lru_cache.h"
#include "fermat/container/list.h"

/// Generate random string of given length.
static std::string GenRandomString(size_t len) {
    static const char alphanum[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, sizeof(alphanum) - 2);
    std::string s(len, '\0');
    for (size_t i = 0; i < len; ++i) s[i] = alphanum[dist(gen)];
    return s;
}

/// Generate a vector of random keys.
static std::vector<std::string> GenerateKeys(size_t n, size_t key_len = 8) {
    std::vector<std::string> keys(n);
    for (size_t i = 0; i < n; ++i) keys[i] = GenRandomString(key_len);
    return keys;
}

using ValueType = int;
using FermatStringList = fermat::List<std::string>;

/// ---------------------------------------------------------------------------
/// Four cache types (2x2 matrix)
/// ---------------------------------------------------------------------------

/// 1) std::unordered_map + std::list
using Cache_StdMap_StdList = fermat::LruCache<
    std::string, ValueType,
    std::list<std::string>,
    std::unordered_map<std::string, std::pair<ValueType, std::list<std::string>::iterator>>
>;

/// 2) std::unordered_map + fermat::List
using Cache_StdMap_FermatList = fermat::LruCache<
    std::string, ValueType,
    FermatStringList,
    std::unordered_map<std::string, std::pair<ValueType, FermatStringList::iterator>>
>;

/// 3) turbo::flat_hash_map + std::list
using Cache_TurboMap_StdList = fermat::LruCache<
    std::string, ValueType,
    std::list<std::string>,
    turbo::flat_hash_map<std::string, std::pair<ValueType, std::list<std::string>::iterator>>
>;

/// 4) turbo::flat_hash_map + fermat::List
using Cache_TurboMap_FermatList = fermat::LruCache<
    std::string, ValueType,
    FermatStringList,
    turbo::flat_hash_map<std::string, std::pair<ValueType, FermatStringList::iterator>>
>;

/// ---------------------------------------------------------------------------
/// Factory functions (explicit constructor call, no list initialization)
/// ---------------------------------------------------------------------------
static Cache_StdMap_StdList Make_StdMap_StdList(size_t cap) {
    return Cache_StdMap_StdList(cap, [](const std::string&) -> ValueType { return 0; }, {});
}
static Cache_StdMap_FermatList Make_StdMap_FermatList(size_t cap) {
    return Cache_StdMap_FermatList(cap, [](const std::string&) -> ValueType { return 0; }, {});
}
static Cache_TurboMap_StdList Make_TurboMap_StdList(size_t cap) {
    return Cache_TurboMap_StdList(cap, [](const std::string&) -> ValueType { return 0; }, {});
}
static Cache_TurboMap_FermatList Make_TurboMap_FermatList(size_t cap) {
    return Cache_TurboMap_FermatList(cap, [](const std::string&) -> ValueType { return 0; }, {});
}

/// ---------------------------------------------------------------------------
/// Benchmarks for combination 1: StdMap + StdList
/// ---------------------------------------------------------------------------
static void BM_StdMap_StdList_InsertDistinct(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_StdList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        benchmark::DoNotOptimize(cache);
        cache.clear();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdMap_StdList_InsertDistinct)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_std_list");

static void BM_StdMap_StdList_LookupHighHit(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_StdList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_StdList_LookupHighHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_std_list");

static void BM_StdMap_StdList_LookupLowHit(benchmark::State& state) {
    const size_t n = state.range(0);
    const size_t cap = n / 2;
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_StdList(cap);
    for (size_t i = 0; i < cap; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_StdList_LookupLowHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_std_list");

static void BM_StdMap_StdList_Update(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_StdList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.assign(keys[idx % n], static_cast<ValueType>(idx));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_StdList_Update)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_std_list");

static void BM_StdMap_StdList_EraseAll(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_StdList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        for (const auto& k : keys) cache.erase(k);
        benchmark::DoNotOptimize(cache);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_StdMap_StdList_EraseAll)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_std_list");

static void BM_StdMap_StdList_Touch(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_StdList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.touch(keys[idx % n]);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_StdList_Touch)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_std_list");

/// ---------------------------------------------------------------------------
/// Benchmarks for combination 2: StdMap + FermatList
/// ---------------------------------------------------------------------------
static void BM_StdMap_FermatList_InsertDistinct(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_FermatList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        benchmark::DoNotOptimize(cache);
        cache.clear();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdMap_FermatList_InsertDistinct)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_fermat_list");

static void BM_StdMap_FermatList_LookupHighHit(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_FermatList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_FermatList_LookupHighHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_fermat_list");

static void BM_StdMap_FermatList_LookupLowHit(benchmark::State& state) {
    const size_t n = state.range(0);
    const size_t cap = n / 2;
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_FermatList(cap);
    for (size_t i = 0; i < cap; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_FermatList_LookupLowHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_fermat_list");

static void BM_StdMap_FermatList_Update(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_FermatList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.assign(keys[idx % n], static_cast<ValueType>(idx));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_FermatList_Update)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_fermat_list");

static void BM_StdMap_FermatList_EraseAll(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_FermatList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        for (const auto& k : keys) cache.erase(k);
        benchmark::DoNotOptimize(cache);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_StdMap_FermatList_EraseAll)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_fermat_list");

static void BM_StdMap_FermatList_Touch(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_StdMap_FermatList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.touch(keys[idx % n]);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_FermatList_Touch)->Arg(1000)->Arg(10000)->Arg(100000)->Name("std_map_fermat_list");

/// ---------------------------------------------------------------------------
/// Benchmarks for combination 3: TurboMap + StdList
/// ---------------------------------------------------------------------------
static void BM_TurboMap_StdList_InsertDistinct(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_StdList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        benchmark::DoNotOptimize(cache);
        cache.clear();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_TurboMap_StdList_InsertDistinct)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_std_list");

static void BM_TurboMap_StdList_LookupHighHit(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_StdList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_StdList_LookupHighHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_std_list");

static void BM_TurboMap_StdList_LookupLowHit(benchmark::State& state) {
    const size_t n = state.range(0);
    const size_t cap = n / 2;
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_StdList(cap);
    for (size_t i = 0; i < cap; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_StdList_LookupLowHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_std_list");

static void BM_TurboMap_StdList_Update(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_StdList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.assign(keys[idx % n], static_cast<ValueType>(idx));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_StdList_Update)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_std_list");

static void BM_TurboMap_StdList_EraseAll(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_StdList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        for (const auto& k : keys) cache.erase(k);
        benchmark::DoNotOptimize(cache);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_TurboMap_StdList_EraseAll)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_std_list");

static void BM_TurboMap_StdList_Touch(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_StdList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.touch(keys[idx % n]);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_StdList_Touch)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_std_list");

/// ---------------------------------------------------------------------------
/// Benchmarks for combination 4: TurboMap + FermatList
/// ---------------------------------------------------------------------------
static void BM_TurboMap_FermatList_InsertDistinct(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_FermatList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        benchmark::DoNotOptimize(cache);
        cache.clear();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_TurboMap_FermatList_InsertDistinct)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_fermat_list");

static void BM_TurboMap_FermatList_LookupHighHit(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_FermatList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_FermatList_LookupHighHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_fermat_list");

static void BM_TurboMap_FermatList_LookupLowHit(benchmark::State& state) {
    const size_t n = state.range(0);
    const size_t cap = n / 2;
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_FermatList(cap);
    for (size_t i = 0; i < cap; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_FermatList_LookupLowHit)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_fermat_list");

static void BM_TurboMap_FermatList_Update(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_FermatList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.assign(keys[idx % n], static_cast<ValueType>(idx));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_FermatList_Update)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_fermat_list");

static void BM_TurboMap_FermatList_EraseAll(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_FermatList(n * 2);
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        for (const auto& k : keys) cache.erase(k);
        benchmark::DoNotOptimize(cache);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_TurboMap_FermatList_EraseAll)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_fermat_list");

static void BM_TurboMap_FermatList_Touch(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = Make_TurboMap_FermatList(n);
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.touch(keys[idx % n]);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TurboMap_FermatList_Touch)->Arg(1000)->Arg(10000)->Arg(100000)->Name("turbo_map_fermat_list");

BENCHMARK_MAIN();