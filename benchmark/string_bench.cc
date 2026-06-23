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
#include <random>
#include <string>
#include <fermat/container/string.h>

using KStr = fermat::KString;
using StdStr = std::string;

/// Generates a random string of given length using a fixed seed for reproducibility.
static std::string RandomString(size_t len) {
    static const char kAlphanum[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(len);
    std::mt19937 rng(12345);  /// Fixed seed
    std::uniform_int_distribution<size_t> dist(0, sizeof(kAlphanum) - 2);
    for (size_t i = 0; i < len; ++i) {
        result.push_back(kAlphanum[dist(rng)]);
    }
    return result;
}

/// Pre‑generated string samples of different length categories.
static const size_t kShortLen = 32;      /// <= 64 bytes, tiny pool
static const size_t kMediumLen = 200;    /// <= 256 bytes, small pool
static const size_t kLongLen = 2048;     /// > 1024 bytes, fallback to malloc

static const std::string kShortSrc = RandomString(kShortLen);
static const std::string kMediumSrc = RandomString(kMediumLen);
static const std::string kLongSrc = RandomString(kLongLen);

// ==================================================================
// Construction from const char* and size
// ==================================================================

static void BM_Construct_Short_KString(benchmark::State& state) {
    for (auto _ : state) {
        KStr s(kShortSrc.data(), kShortSrc.size());
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Construct_Short_KString);

static void BM_Construct_Short_StdString(benchmark::State& state) {
    for (auto _ : state) {
        StdStr s(kShortSrc.data(), kShortSrc.size());
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Construct_Short_StdString);

static void BM_Construct_Medium_KString(benchmark::State& state) {
    for (auto _ : state) {
        KStr s(kMediumSrc.data(), kMediumSrc.size());
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Construct_Medium_KString);

static void BM_Construct_Medium_StdString(benchmark::State& state) {
    for (auto _ : state) {
        StdStr s(kMediumSrc.data(), kMediumSrc.size());
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Construct_Medium_StdString);

static void BM_Construct_Long_KString(benchmark::State& state) {
    for (auto _ : state) {
        KStr s(kLongSrc.data(), kLongSrc.size());
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Construct_Long_KString);

static void BM_Construct_Long_StdString(benchmark::State& state) {
    for (auto _ : state) {
        StdStr s(kLongSrc.data(), kLongSrc.size());
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Construct_Long_StdString);

// ==================================================================
// Copy construction
// ==================================================================

static void BM_Copy_Short_KString(benchmark::State& state) {
    KStr src(kShortSrc.data(), kShortSrc.size());
    for (auto _ : state) {
        KStr s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Copy_Short_KString);

static void BM_Copy_Short_StdString(benchmark::State& state) {
    StdStr src(kShortSrc.data(), kShortSrc.size());
    for (auto _ : state) {
        StdStr s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Copy_Short_StdString);

static void BM_Copy_Medium_KString(benchmark::State& state) {
    KStr src(kMediumSrc.data(), kMediumSrc.size());
    for (auto _ : state) {
        KStr s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Copy_Medium_KString);

static void BM_Copy_Medium_StdString(benchmark::State& state) {
    StdStr src(kMediumSrc.data(), kMediumSrc.size());
    for (auto _ : state) {
        StdStr s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Copy_Medium_StdString);

static void BM_Copy_Long_KString(benchmark::State& state) {
    KStr src(kLongSrc.data(), kLongSrc.size());
    for (auto _ : state) {
        KStr s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Copy_Long_KString);

static void BM_Copy_Long_StdString(benchmark::State& state) {
    StdStr src(kLongSrc.data(), kLongSrc.size());
    for (auto _ : state) {
        StdStr s(src);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Copy_Long_StdString);

// ==================================================================
// Move construction
// ==================================================================

static void BM_Move_Short_KString(benchmark::State& state) {
    for (auto _ : state) {
        KStr src(kShortSrc.data(), kShortSrc.size());
        KStr s(std::move(src));
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Move_Short_KString);

static void BM_Move_Short_StdString(benchmark::State& state) {
    for (auto _ : state) {
        StdStr src(kShortSrc.data(), kShortSrc.size());
        StdStr s(std::move(src));
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Move_Short_StdString);

static void BM_Move_Medium_KString(benchmark::State& state) {
    for (auto _ : state) {
        KStr src(kMediumSrc.data(), kMediumSrc.size());
        KStr s(std::move(src));
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Move_Medium_KString);

static void BM_Move_Medium_StdString(benchmark::State& state) {
    for (auto _ : state) {
        StdStr src(kMediumSrc.data(), kMediumSrc.size());
        StdStr s(std::move(src));
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Move_Medium_StdString);

static void BM_Move_Long_KString(benchmark::State& state) {
    for (auto _ : state) {
        KStr src(kLongSrc.data(), kLongSrc.size());
        KStr s(std::move(src));
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Move_Long_KString);

static void BM_Move_Long_StdString(benchmark::State& state) {
    for (auto _ : state) {
        StdStr src(kLongSrc.data(), kLongSrc.size());
        StdStr s(std::move(src));
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_Move_Long_StdString);

// ==================================================================
// Assignment (copy)
// ==================================================================

static void BM_Assign_Short_KString(benchmark::State& state) {
    KStr src(kShortSrc.data(), kShortSrc.size());
    KStr dst;
    for (auto _ : state) {
        dst = src;
        benchmark::DoNotOptimize(dst);
    }
}
BENCHMARK(BM_Assign_Short_KString);

static void BM_Assign_Short_StdString(benchmark::State& state) {
    StdStr src(kShortSrc.data(), kShortSrc.size());
    StdStr dst;
    for (auto _ : state) {
        dst = src;
        benchmark::DoNotOptimize(dst);
    }
}
BENCHMARK(BM_Assign_Short_StdString);

// Medium and long are similar – omitted for brevity, add if needed.

// ==================================================================
// Append (operator+=)
// ==================================================================

static void BM_Append_Short_KString(benchmark::State& state) {
    KStr s;
    for (auto _ : state) {
        s += kShortSrc;
        benchmark::DoNotOptimize(s);
        s.clear();
    }
}
BENCHMARK(BM_Append_Short_KString);

static void BM_Append_Short_StdString(benchmark::State& state) {
    StdStr s;
    for (auto _ : state) {
        s += kShortSrc;
        benchmark::DoNotOptimize(s);
        s.clear();
    }
}
BENCHMARK(BM_Append_Short_StdString);

static void BM_Append_Medium_KString(benchmark::State& state) {
    KStr s;
    for (auto _ : state) {
        s += kMediumSrc;
        benchmark::DoNotOptimize(s);
        s.clear();
    }
}
BENCHMARK(BM_Append_Medium_KString);

static void BM_Append_Medium_StdString(benchmark::State& state) {
    StdStr s;
    for (auto _ : state) {
        s += kMediumSrc;
        benchmark::DoNotOptimize(s);
        s.clear();
    }
}
BENCHMARK(BM_Append_Medium_StdString);

static void BM_Append_Long_KString(benchmark::State& state) {
    KStr s;
    for (auto _ : state) {
        s += kLongSrc;
        benchmark::DoNotOptimize(s);
        s.clear();
    }
}
BENCHMARK(BM_Append_Long_KString);

static void BM_Append_Long_StdString(benchmark::State& state) {
    StdStr s;
    for (auto _ : state) {
        s += kLongSrc;
        benchmark::DoNotOptimize(s);
        s.clear();
    }
}
BENCHMARK(BM_Append_Long_StdString);

// ==================================================================
// Find (substring search)
// ==================================================================

static void BM_Find_Short_KString(benchmark::State& state) {
    KStr s(kShortSrc.data(), kShortSrc.size());
    const std::string pattern = kShortSrc.substr(0, 5);
    for (auto _ : state) {
        auto pos = s.find(pattern);
        benchmark::DoNotOptimize(pos);
    }
}
BENCHMARK(BM_Find_Short_KString);

static void BM_Find_Short_StdString(benchmark::State& state) {
    StdStr s(kShortSrc.data(), kShortSrc.size());
    const std::string pattern = kShortSrc.substr(0, 5);
    for (auto _ : state) {
        auto pos = s.find(pattern);
        benchmark::DoNotOptimize(pos);
    }
}
BENCHMARK(BM_Find_Short_StdString);

static void BM_Find_Long_KString(benchmark::State& state) {
    KStr s(kLongSrc.data(), kLongSrc.size());
    const std::string pattern = kLongSrc.substr(kLongLen / 2, 10);
    for (auto _ : state) {
        auto pos = s.find(pattern);
        benchmark::DoNotOptimize(pos);
    }
}
BENCHMARK(BM_Find_Long_KString);

static void BM_Find_Long_StdString(benchmark::State& state) {
    StdStr s(kLongSrc.data(), kLongSrc.size());
    const std::string pattern = kLongSrc.substr(kLongLen / 2, 10);
    for (auto _ : state) {
        auto pos = s.find(pattern);
        benchmark::DoNotOptimize(pos);
    }
}
BENCHMARK(BM_Find_Long_StdString);

// ==================================================================
// Comparison (operator==)
// ==================================================================

static void BM_Compare_Equal_Short_KString(benchmark::State& state) {
    KStr a(kShortSrc.data(), kShortSrc.size());
    KStr b(kShortSrc.data(), kShortSrc.size());
    for (auto _ : state) {
        bool eq = (a == b);
        benchmark::DoNotOptimize(eq);
    }
}
BENCHMARK(BM_Compare_Equal_Short_KString);

static void BM_Compare_Equal_Short_StdString(benchmark::State& state) {
    StdStr a(kShortSrc.data(), kShortSrc.size());
    StdStr b(kShortSrc.data(), kShortSrc.size());
    for (auto _ : state) {
        bool eq = (a == b);
        benchmark::DoNotOptimize(eq);
    }
}
BENCHMARK(BM_Compare_Equal_Short_StdString);

static void BM_Compare_NotEqual_Medium_KString(benchmark::State& state) {
    KStr a(kMediumSrc.data(), kMediumSrc.size());
    KStr b(kMediumSrc.data(), kMediumSrc.size());
    b[0] = 'X';
    for (auto _ : state) {
        bool eq = (a == b);
        benchmark::DoNotOptimize(eq);
    }
}
BENCHMARK(BM_Compare_NotEqual_Medium_KString);

static void BM_Compare_NotEqual_Medium_StdString(benchmark::State& state) {
    StdStr a(kMediumSrc.data(), kMediumSrc.size());
    StdStr b(kMediumSrc.data(), kMediumSrc.size());
    b[0] = 'X';
    for (auto _ : state) {
        bool eq = (a == b);
        benchmark::DoNotOptimize(eq);
    }
}
BENCHMARK(BM_Compare_NotEqual_Medium_StdString);

// ==================================================================
// Hashing (std::hash)
// ==================================================================

static void BM_Hash_Short_KString(benchmark::State& state) {
    KStr s(kShortSrc.data(), kShortSrc.size());
    std::hash<KStr> h;
    for (auto _ : state) {
        auto val = h(s);
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_Hash_Short_KString);

static void BM_Hash_Short_StdString(benchmark::State& state) {
    StdStr s(kShortSrc.data(), kShortSrc.size());
    std::hash<StdStr> h;
    for (auto _ : state) {
        auto val = h(s);
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_Hash_Short_StdString);

static void BM_Hash_Long_KString(benchmark::State& state) {
    KStr s(kLongSrc.data(), kLongSrc.size());
    std::hash<KStr> h;
    for (auto _ : state) {
        auto val = h(s);
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_Hash_Long_KString);

static void BM_Hash_Long_StdString(benchmark::State& state) {
    StdStr s(kLongSrc.data(), kLongSrc.size());
    std::hash<StdStr> h;
    for (auto _ : state) {
        auto val = h(s);
        benchmark::DoNotOptimize(val);
    }
}
BENCHMARK(BM_Hash_Long_StdString);

// ==================================================================
// Main entry point
// ==================================================================
BENCHMARK_MAIN();