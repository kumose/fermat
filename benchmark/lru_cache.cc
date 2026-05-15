/// @file lru_cache_benchmark.cc
/// @brief Benchmark comparing fermat::LruCache with different underlying maps.
///        Compares turbo::flat_hash_map vs std::unordered_map.

#include <benchmark/benchmark.h>
#include <random>
#include <string>
#include <unordered_map>
#include <turbo/container/flat_hash_map.h>
#include "fermat/container/lru_cache.h"   ///< Assumed correct path.

/// Generate a random string of given length.
static std::string GenRandomString(size_t len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
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

/// A simple value type (integer) for caching.
using ValueType = int;

/// Helper to create a cache with a given capacity and underlying map type.
/// For turbo::flat_hash_map.
static auto MakeTurboCache(size_t capacity) {
    return  fermat::LruCache<std::string, ValueType,
                             std::list<std::string>,
                             turbo::flat_hash_map<std::string,
                                                  std::pair<ValueType,
                                                            std::list<std::string>::iterator>>>
        (capacity, [](const std::string&) { return 0; }, nullptr);

}

/// For std::unordered_map.
static auto MakeStdCache(size_t capacity) {
    return  fermat::LruCache<std::string, ValueType,
                             std::list<std::string>,
                             std::unordered_map<std::string,
                                                std::pair<ValueType,
                                                          std::list<std::string>::iterator>>>
        (capacity, [](const std::string&) { return 0; }, nullptr);
}

/// ---------------------------------------------------------------------------
/// Benchmark: Insert N distinct keys (cache capacity >= N)
/// ---------------------------------------------------------------------------
template<typename CacheType>
static void BM_InsertDistinct(benchmark::State& state, CacheType& cache,
                              const std::vector<std::string>& keys) {
    const size_t n = keys.size();
    for (auto _ : state) {
        for (size_t i = 0; i < n; ++i) {
            cache.insert(keys[i], static_cast<ValueType>(i));
        }
        benchmark::DoNotOptimize(cache);
        // Reset for next iteration
        cache.clear();
    }
    state.SetItemsProcessed(state.iterations() * n);
}

static void BM_TurboCache_InsertDistinct(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeTurboCache(n * 2);  // ample capacity to avoid eviction
    BM_InsertDistinct(state, cache, keys);
}
BENCHMARK(BM_TurboCache_InsertDistinct)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_StdCache_InsertDistinct(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeStdCache(n * 2);
    BM_InsertDistinct(state, cache, keys);
}
BENCHMARK(BM_StdCache_InsertDistinct)->Arg(1000)->Arg(10000)->Arg(100000);

/// ---------------------------------------------------------------------------
/// Benchmark: Lookup (get) with high hit rate (cache capacity >= N)
/// ---------------------------------------------------------------------------
template<typename CacheType>
static void BM_LookupHighHit(benchmark::State& state, CacheType& cache,
                             const std::vector<std::string>& keys) {
    const size_t n = keys.size();
    // Pre-insert all keys
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_TurboCache_LookupHighHit(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeTurboCache(n);
    BM_LookupHighHit(state, cache, keys);
}
BENCHMARK(BM_TurboCache_LookupHighHit)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_StdCache_LookupHighHit(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeStdCache(n);
    BM_LookupHighHit(state, cache, keys);
}
BENCHMARK(BM_StdCache_LookupHighHit)->Arg(1000)->Arg(10000)->Arg(100000);

/// ---------------------------------------------------------------------------
/// Benchmark: Lookup with low hit rate (cache capacity smaller than working set)
/// ---------------------------------------------------------------------------
template<typename CacheType>
static void BM_LookupLowHit(benchmark::State& state, CacheType& cache,
                            const std::vector<std::string>& keys) {
    const size_t n = keys.size();          // working set size
    const size_t cap = state.range(1);     // cache capacity
    // Pre-insert first 'cap' keys
    for (size_t i = 0; i < cap; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        // Access keys in round‑robin; most accesses will miss once working set > capacity
        auto v = cache.get(keys[idx % n]);
        benchmark::DoNotOptimize(v);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_TurboCache_LookupLowHit(benchmark::State& state) {
    const size_t n = state.range(0);
    const size_t cap = n / 2;   // capacity half of working set -> many misses
    auto keys = GenerateKeys(n);
    auto cache = MakeTurboCache(cap);
    BM_LookupLowHit(state, cache, keys);
}
BENCHMARK(BM_TurboCache_LookupLowHit)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_StdCache_LookupLowHit(benchmark::State& state) {
    const size_t n = state.range(0);
    const size_t cap = n / 2;
    auto keys = GenerateKeys(n);
    auto cache = MakeStdCache(cap);
    BM_LookupLowHit(state, cache, keys);
}
BENCHMARK(BM_StdCache_LookupLowHit)->Arg(1000)->Arg(10000)->Arg(100000);

/// ---------------------------------------------------------------------------
/// Benchmark: Update (assign) existing keys
/// ---------------------------------------------------------------------------
template<typename CacheType>
static void BM_Update(benchmark::State& state, CacheType& cache,
                      const std::vector<std::string>& keys) {
    const size_t n = keys.size();
    // Pre-insert all keys
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.assign(keys[idx % n], static_cast<ValueType>(idx));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_TurboCache_Update(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeTurboCache(n);
    BM_Update(state, cache, keys);
}
BENCHMARK(BM_TurboCache_Update)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_StdCache_Update(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeStdCache(n);
    BM_Update(state, cache, keys);
}
BENCHMARK(BM_StdCache_Update)->Arg(1000)->Arg(10000)->Arg(100000);

/// ---------------------------------------------------------------------------
/// Benchmark: Erase all keys
/// ---------------------------------------------------------------------------
template<typename CacheType>
static void BM_EraseAll(benchmark::State& state, CacheType& cache,
                        const std::vector<std::string>& keys) {
    const size_t n = keys.size();
    for (auto _ : state) {
        // Insert all keys
        for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
        // Erase all
        for (const auto& k : keys) cache.erase(k);
        benchmark::DoNotOptimize(cache);
    }
    state.SetItemsProcessed(state.iterations() * n * 2);
}

static void BM_TurboCache_EraseAll(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeTurboCache(n * 2);
    BM_EraseAll(state, cache, keys);
}
BENCHMARK(BM_TurboCache_EraseAll)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_StdCache_EraseAll(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeStdCache(n * 2);
    BM_EraseAll(state, cache, keys);
}
BENCHMARK(BM_StdCache_EraseAll)->Arg(1000)->Arg(10000)->Arg(100000);

/// ---------------------------------------------------------------------------
/// Benchmark: Touch (refresh LRU order) without data access
/// ---------------------------------------------------------------------------
template<typename CacheType>
static void BM_Touch(benchmark::State& state, CacheType& cache,
                     const std::vector<std::string>& keys) {
    const size_t n = keys.size();
    for (size_t i = 0; i < n; ++i) cache.insert(keys[i], static_cast<ValueType>(i));
    size_t idx = 0;
    for (auto _ : state) {
        cache.touch(keys[idx % n]);
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_TurboCache_Touch(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeTurboCache(n);
    BM_Touch(state, cache, keys);
}
BENCHMARK(BM_TurboCache_Touch)->Arg(1000)->Arg(10000)->Arg(100000);

static void BM_StdCache_Touch(benchmark::State& state) {
    const size_t n = state.range(0);
    auto keys = GenerateKeys(n);
    auto cache = MakeStdCache(n);
    BM_Touch(state, cache, keys);
}
BENCHMARK(BM_StdCache_Touch)->Arg(1000)->Arg(10000)->Arg(100000);

BENCHMARK_MAIN();