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
#include <turbo/container/flat_hash_map.h>
#include <fermat/memory/allocator.h>
#include <random>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

// Generate random alphanumeric string
static std::string random_string(size_t len, std::mt19937_64& rng) {
    static const char alphanum[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<size_t> dist(0, sizeof(alphanum) - 2);
    std::string result;
    result.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(alphanum[dist(rng)]);
    }
    return result;
}

struct TestData {
    std::vector<std::string> keys;
    std::vector<std::string> values;
    size_t count;
};

static TestData generate_test_data(size_t target_raw_bytes = 1024 * 1024 * 1024) {
    const size_t avg_string_len = 64;
    size_t approx_pairs = target_raw_bytes / (2 * avg_string_len);
    size_t n = std::min<size_t>(approx_pairs, 2'000'000);
    TestData data;
    data.count = n;
    data.keys.reserve(n);
    data.values.reserve(n);
    std::mt19937_64 rng(12345);
    for (size_t i = 0; i < n; ++i) {
        std::string key = random_string(avg_string_len / 2, rng) + std::to_string(i);
        std::string val = random_string(avg_string_len / 2, rng);
        data.keys.push_back(std::move(key));
        data.values.push_back(std::move(val));
    }
    return data;
}

static const TestData& get_test_data() {
    static TestData data = generate_test_data();
    return data;
}

// Benchmark: Insert only
template<typename MapType>
static void BM_Insert(benchmark::State& state) {
    const auto& data = get_test_data();
    for (auto _ : state) {
        MapType map;
        for (size_t i = 0; i < data.count; ++i) {
            map[data.keys[i]] = data.values[i];
        }
        benchmark::DoNotOptimize(map);
    }
    state.SetItemsProcessed(state.iterations() * data.count);
}

// Benchmark: Mixed operations
template<typename MapType>
static void BM_Mixed(benchmark::State& state) {
    const auto& data = get_test_data();
    std::vector<size_t> indices(data.count);
    std::iota(indices.begin(), indices.end(), 0);
    std::mt19937_64 rng(56789);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (auto _ : state) {
        MapType map;
        // Insert all
        for (size_t idx : indices) {
            map[data.keys[idx]] = data.values[idx];
        }
        // Find all
        size_t found = 0;
        for (size_t idx : indices) {
            auto it = map.find(data.keys[idx]);
            if (it != map.end() && it->second == data.values[idx]) ++found;
        }
        benchmark::DoNotOptimize(found);
        // Erase all
        for (size_t idx : indices) {
            map.erase(data.keys[idx]);
        }
        benchmark::DoNotOptimize(map.empty());
    }
    state.SetItemsProcessed(state.iterations() * (3 * data.count));
}


BENCHMARK_MAIN();