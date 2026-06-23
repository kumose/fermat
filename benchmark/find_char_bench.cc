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
#include <xsimd/xsimd.hpp>
#include <string_view>
#include <string>
#include <random>

constexpr size_t kNPos = static_cast<size_t>(-1);

// SIMD version using xsimd (corrected mask handling)
size_t find_in_range_simd(std::string_view s, char c) {
    const size_t len = s.size();
    if (len == 0) return kNPos;

    using batch_type = xsimd::batch<char, xsimd::default_arch>;
    const size_t batch_size = batch_type::size;
    const batch_type target = batch_type(c);
    const char* data = s.data();
    size_t i = 0;

    // Optional prefix alignment (kept from original)
    const uintptr_t addr = reinterpret_cast<uintptr_t>(data);
    const size_t align_mask = batch_size - 1;
    const size_t first_aligned = ((addr + batch_size - 1) & ~align_mask) - addr;
    size_t prefix = std::min(first_aligned, len);
    for (i = 0; i < prefix; ++i) {
        if (data[i] == c) return i;
    }

    for (; i + batch_size <= len; i += batch_size) {
        batch_type vec = xsimd::load_aligned(data + i);
        auto mask = xsimd::eq(vec, target);
        uint64_t bits = mask.mask();   // correct API
        if (bits) {
            int offset = __builtin_ctzll(bits);
            return i + offset;
        }
    }

    for (; i < len; ++i) {
        if (data[i] == c) return i;
    }
    return kNPos;
}

// STL baseline
size_t find_in_range_stl(std::string_view s, char c) {
    return s.find(c);
}

static void BM_FindSimd(benchmark::State& state) {
    const size_t len = state.range(0);
    const char target = 'x';
    std::string data(len, 'a');
    // Place target at a random position (but deterministic for reproducibility)
    size_t pos = len / 2;
    if (len > 0) data[pos] = target;

    for (auto _ : state) {
        size_t res = find_in_range_simd(data, target);
        benchmark::DoNotOptimize(res);
    }
    state.SetBytesProcessed(state.iterations() * len);
}

static void BM_FindStl(benchmark::State& state) {
    const size_t len = state.range(0);
    const char target = 'x';
    std::string data(len, 'a');
    size_t pos = len / 2;
    if (len > 0) data[pos] = target;

    for (auto _ : state) {
        size_t res = find_in_range_stl(data, target);
        benchmark::DoNotOptimize(res);
    }
    state.SetBytesProcessed(state.iterations() * len);
}

BENCHMARK(BM_FindSimd)->Range(8, 1 << 20)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_FindStl)->Range(8, 1 << 20)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();