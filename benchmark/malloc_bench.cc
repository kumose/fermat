/// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.

#include <benchmark/benchmark.h>
#include <cstdlib>      // malloc, free, aligned_alloc, posix_memalign
#include <cstring>      // memset
#include <memory>

#define HAS_MIMALLOC
#ifdef HAS_MIMALLOC
#include <mimalloc.h>
#endif

namespace {

/// Helper: allocate, touch (memset), and free.
template <typename AllocFunc, typename FreeFunc>
void BM_AllocTouchFree(benchmark::State& state, AllocFunc alloc, FreeFunc free, size_t size) {
    for (auto _ : state) {
        void* ptr = alloc(size);
        if (ptr) {
            std::memset(ptr, 0, size);          ///< Touch memory to force physical allocation
            benchmark::DoNotOptimize(ptr);
            benchmark::ClobberMemory();
        }
        free(ptr);
    }
    state.SetBytesProcessed(state.iterations() * size);
}

// ------------------------------------------------------------------------------
/// Define benchmark macros for each allocator and size
// ------------------------------------------------------------------------------

/// For malloc / free
#define BENCH_MALLOC(size) \
    static void BM_Malloc_##size(benchmark::State& state) { \
        BM_AllocTouchFree(state, [](size_t s) { return malloc(s); }, free, size); \
    } \
    BENCHMARK(BM_Malloc_##size);

/// For mi_malloc / mi_free
#ifdef HAS_MIMALLOC
#define BENCH_MIMALLOC(size) \
    static void BM_MiMalloc_##size(benchmark::State& state) { \
        BM_AllocTouchFree(state, [](size_t s) { return mi_malloc(s); }, mi_free, size); \
    } \
    BENCHMARK(BM_MiMalloc_##size);
#endif

/// For mi_malloc_aligned (alignment = 64)
#ifdef HAS_MIMALLOC
#define BENCH_MIMALLOC_ALIGNED(size) \
    static void BM_MiMallocAligned_##size(benchmark::State& state) { \
        const size_t alignment = 64; \
        BM_AllocTouchFree(state, [alignment](size_t s) { return mi_malloc_aligned(s, alignment); }, mi_free, size); \
    } \
    BENCHMARK(BM_MiMallocAligned_##size);
#endif

/// For aligned_alloc (alignment = 64)
#define BENCH_ALIGNED_ALLOC(size) \
    static void BM_AlignedAlloc_##size(benchmark::State& state) { \
        const size_t alignment = 64; \
        size_t rounded = ((size + alignment - 1) / alignment) * alignment; \
        BM_AllocTouchFree(state, [alignment](size_t s) { return aligned_alloc(alignment, s); }, free, rounded); \
    } \
    BENCHMARK(BM_AlignedAlloc_##size);

/// For posix_memalign (alignment = 64)
#define BENCH_POSIX_MEMALIGN(size) \
    static void BM_PosixMemalign_##size(benchmark::State& state) { \
        const size_t alignment = 64; \
        BM_AllocTouchFree(state, [alignment](size_t s) { \
            void* ptr = nullptr; \
            int ret = posix_memalign(&ptr, alignment, s); \
            return (ret == 0) ? ptr : nullptr; \
        }, free, size); \
    } \
    BENCHMARK(BM_PosixMemalign_##size);

// ------------------------------------------------------------------------------
/// List of sizes: powers of two from 4 bytes to 128 MB
// ------------------------------------------------------------------------------

constexpr size_t kSizes[] = {
    4, 8, 16, 32, 64, 128, 256, 512,
    1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072,
    262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216,
    33554432, 67108864, 134217728   // up to 128 MB
};

// Helper macro to iterate over all sizes
#define FOREACH_SIZE(MACRO) \
    MACRO(4) MACRO(8) MACRO(16) MACRO(32) MACRO(64) MACRO(128) MACRO(256) MACRO(512) \
    MACRO(1024) MACRO(2048) MACRO(4096) MACRO(8192) MACRO(16384) MACRO(32768) MACRO(65536) MACRO(131072) \
    MACRO(262144) MACRO(524288) MACRO(1048576) MACRO(2097152) MACRO(4194304) MACRO(8388608) MACRO(16777216) \
    MACRO(33554432) MACRO(67108864) MACRO(134217728)

/// Generate all benchmarks
FOREACH_SIZE(BENCH_MALLOC)
FOREACH_SIZE(BENCH_ALIGNED_ALLOC)
FOREACH_SIZE(BENCH_POSIX_MEMALIGN)

#ifdef HAS_MIMALLOC
FOREACH_SIZE(BENCH_MIMALLOC)
FOREACH_SIZE(BENCH_MIMALLOC_ALIGNED)   // Added: aligned version of mimalloc
#endif

} // namespace

BENCHMARK_MAIN();