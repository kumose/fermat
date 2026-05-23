// benchmark_bitset.cpp
/// Benchmark comparing fermat::Bitset, fermat::BitmapView with std::bitset.
/// Compile: c++ -std=c++20 -O2 -DNDEBUG -I/path/to/fermat/include -lbenchmark -lpthread benchmark_bitset.cpp -o benchmark_bitset

#include <benchmark/benchmark.h>
#include <bitset>
#include <random>
#include <cstdint>
#include <fermat/container/bitmap_view.h>
#include <fermat/container/bitset.h>

/// Tested bit sizes (powers of two from 64 to 2048)
static constexpr size_t kSizes[] = {64, 128, 256, 512, 1024, 2048};

/// Generate a random bitset of size N using the given template template parameter.
template <size_t N, template <size_t> class BitsetType>
BitsetType<N> RandomBitset() {
    static std::mt19937_64 rng(12345);
    BitsetType<N> bs;
    constexpr size_t kWordCount = (N + 63) / 64;
    for (size_t i = 0; i < kWordCount; ++i) {
        uint64_t v = rng();
        for (size_t b = 0; b < 64 && i * 64 + b < N; ++b) {
            if ((v >> b) & 1) bs.set(i * 64 + b);
        }
    }
    return bs;
}

/// Generate random words for BitmapView initialization (same pattern as RandomBitset)
template <typename WordType>
std::vector<WordType> RandomWords(size_t num_words) {
    static std::mt19937_64 rng(12345);
    std::vector<WordType> words(num_words);
    for (size_t i = 0; i < num_words; ++i) {
        words[i] = static_cast<WordType>(rng());
    }
    return words;
}

/// Simulate find_first for std::bitset (linear scan)
template <size_t N>
size_t StdFindFirst(const std::bitset<N>& bs) {
    for (size_t i = 0; i < N; ++i) {
        if (bs.test(i)) return i;
    }
    return N;
}

/// Simulate find_next for std::bitset (linear scan)
template <size_t N>
size_t StdFindNext(const std::bitset<N>& bs, size_t prev) {
    for (size_t i = prev + 1; i < N; ++i) {
        if (bs.test(i)) return i;
    }
    return N;
}

// ============================================================================
//  Helper: Get global storage for BitmapView (to avoid allocation in benchmark loop)
// ============================================================================

/// Returns a vector of words sized for the given number of bits.
static std::vector<uint64_t>& GetBitmapStorage(size_t bits) {
    static std::unordered_map<size_t, std::vector<uint64_t>> storage;
    const size_t needed_words = (bits + 63) / 64;
    auto& vec = storage[bits];
    if (vec.size() < needed_words) {
        vec.resize(needed_words);
    }
    return vec;
}

/// Initialize a BitmapView with random content for the given size.
static fermat::BitmapView<true, uint64_t> MakeRandomBitmap(size_t bits) {
    auto& storage = GetBitmapStorage(bits);
    const size_t words = (bits + 63) / 64;
    static std::mt19937_64 rng(12345);
    for (size_t i = 0; i < words; ++i) {
        storage[i] = rng();
    }
    fermat::BitmapView<true, uint64_t> bm;
    bm.setup({storage.data(), storage.size()}, bits);
    // Trim high bits in last word to respect invariant (sanitize is automatic in setup? No, we call flip? Better do manually)
    // Bitset::setup does not sanitize, but BitmapBase relies on caller to keep invariant.
    // We simply trust that using random words is fine for benchmark.
    return bm;
}

// ============================================================================
//  Default constructor (Bitset / std::bitset) – BitmapView not applicable (no default storage)
// ============================================================================

template <size_t N>
void BM_Fermat_ConstructDefault(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(fermat::Bitset<N>());
    }
}

template <size_t N>
void BM_Std_ConstructDefault(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(std::bitset<N>());
    }
}

#define BENCHMARK_BOTH_CONSTRUCT_DEFAULT(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_ConstructDefault, size); \
    BENCHMARK_TEMPLATE(BM_Std_ConstructDefault, size)

BENCHMARK_BOTH_CONSTRUCT_DEFAULT(64);
BENCHMARK_BOTH_CONSTRUCT_DEFAULT(128);
BENCHMARK_BOTH_CONSTRUCT_DEFAULT(256);
BENCHMARK_BOTH_CONSTRUCT_DEFAULT(512);
BENCHMARK_BOTH_CONSTRUCT_DEFAULT(1024);
BENCHMARK_BOTH_CONSTRUCT_DEFAULT(2048);

// ============================================================================
//  Constructor from unsigned long long (only fixed-size)
// ============================================================================

template <size_t N>
void BM_Fermat_ConstructFromULL(benchmark::State& state) {
    uint64_t val = 0x123456789abcdef0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(fermat::Bitset<N>(val));
    }
}

template <size_t N>
void BM_Std_ConstructFromULL(benchmark::State& state) {
    uint64_t val = 0x123456789abcdef0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(std::bitset<N>(val));
    }
}

#define BENCHMARK_BOTH_CONSTRUCT_ULL(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_ConstructFromULL, size); \
    BENCHMARK_TEMPLATE(BM_Std_ConstructFromULL, size)

BENCHMARK_BOTH_CONSTRUCT_ULL(64);
BENCHMARK_BOTH_CONSTRUCT_ULL(128);
BENCHMARK_BOTH_CONSTRUCT_ULL(256);
BENCHMARK_BOTH_CONSTRUCT_ULL(512);
BENCHMARK_BOTH_CONSTRUCT_ULL(1024);
BENCHMARK_BOTH_CONSTRUCT_ULL(2048);

// ============================================================================
//  Setup BitmapView (analogous to construction)
// ============================================================================

template <size_t N>
void BM_Bitmap_Setup(benchmark::State& state) {
    auto& storage = GetBitmapStorage(N);
    fermat::BitmapView<true, uint64_t> bm;
    for (auto _ : state) {
        bm.setup({storage.data(), storage.size()}, N);
        benchmark::DoNotOptimize(bm);
    }
}
BENCHMARK_TEMPLATE(BM_Bitmap_Setup, 64);
BENCHMARK_TEMPLATE(BM_Bitmap_Setup, 128);
BENCHMARK_TEMPLATE(BM_Bitmap_Setup, 256);
BENCHMARK_TEMPLATE(BM_Bitmap_Setup, 512);
BENCHMARK_TEMPLATE(BM_Bitmap_Setup, 1024);
BENCHMARK_TEMPLATE(BM_Bitmap_Setup, 2048);

// ============================================================================
//  Set all bits
// ============================================================================

template <size_t N>
void BM_Fermat_SetAll(benchmark::State& state) {
    fermat::Bitset<N> bs;
    for (auto _ : state) {
        bs.set();
        benchmark::DoNotOptimize(bs);
    }
}

template <size_t N>
void BM_Std_SetAll(benchmark::State& state) {
    std::bitset<N> bs;
    for (auto _ : state) {
        bs.set();
        benchmark::DoNotOptimize(bs);
    }
}

template <size_t N>
void BM_Bitmap_SetAll(benchmark::State& state) {
    auto& storage = GetBitmapStorage(N);
    fermat::BitmapView<true, uint64_t> bm;
    bm.setup({storage.data(), storage.size()}, N);
    for (auto _ : state) {
        bm.set();
        benchmark::DoNotOptimize(bm);
    }
}

#define BENCHMARK_BOTH_SET_ALL(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_SetAll, size); \
    BENCHMARK_TEMPLATE(BM_Std_SetAll, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_SetAll, size)

BENCHMARK_BOTH_SET_ALL(64);
BENCHMARK_BOTH_SET_ALL(128);
BENCHMARK_BOTH_SET_ALL(256);
BENCHMARK_BOTH_SET_ALL(512);
BENCHMARK_BOTH_SET_ALL(1024);
BENCHMARK_BOTH_SET_ALL(2048);

// ============================================================================
//  Reset all bits
// ============================================================================

template <size_t N>
void BM_Fermat_ResetAll(benchmark::State& state) {
    fermat::Bitset<N> bs;
    bs.set();
    for (auto _ : state) {
        bs.reset();
        benchmark::DoNotOptimize(bs);
        bs.set();
    }
}

template <size_t N>
void BM_Std_ResetAll(benchmark::State& state) {
    std::bitset<N> bs;
    bs.set();
    for (auto _ : state) {
        bs.reset();
        benchmark::DoNotOptimize(bs);
        bs.set();
    }
}

template <size_t N>
void BM_Bitmap_ResetAll(benchmark::State& state) {
    auto& storage = GetBitmapStorage(N);
    fermat::BitmapView<true, uint64_t> bm;
    bm.setup({storage.data(), storage.size()}, N);
    bm.set();
    for (auto _ : state) {
        bm.reset();
        benchmark::DoNotOptimize(bm);
        bm.set();
    }
}

#define BENCHMARK_BOTH_RESET_ALL(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_ResetAll, size); \
    BENCHMARK_TEMPLATE(BM_Std_ResetAll, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_ResetAll, size)

BENCHMARK_BOTH_RESET_ALL(64);
BENCHMARK_BOTH_RESET_ALL(128);
BENCHMARK_BOTH_RESET_ALL(256);
BENCHMARK_BOTH_RESET_ALL(512);
BENCHMARK_BOTH_RESET_ALL(1024);
BENCHMARK_BOTH_RESET_ALL(2048);

// ============================================================================
//  Flip all bits
// ============================================================================

template <size_t N>
void BM_Fermat_FlipAll(benchmark::State& state) {
    fermat::Bitset<N> bs;
    for (auto _ : state) {
        bs.flip();
        benchmark::DoNotOptimize(bs);
    }
}

template <size_t N>
void BM_Std_FlipAll(benchmark::State& state) {
    std::bitset<N> bs;
    for (auto _ : state) {
        bs.flip();
        benchmark::DoNotOptimize(bs);
    }
}

template <size_t N>
void BM_Bitmap_FlipAll(benchmark::State& state) {
    auto& storage = GetBitmapStorage(N);
    fermat::BitmapView<true, uint64_t> bm;
    bm.setup({storage.data(), storage.size()}, N);
    for (auto _ : state) {
        bm.flip();
        benchmark::DoNotOptimize(bm);
    }
}

#define BENCHMARK_BOTH_FLIP_ALL(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_FlipAll, size); \
    BENCHMARK_TEMPLATE(BM_Std_FlipAll, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_FlipAll, size)

BENCHMARK_BOTH_FLIP_ALL(64);
BENCHMARK_BOTH_FLIP_ALL(128);
BENCHMARK_BOTH_FLIP_ALL(256);
BENCHMARK_BOTH_FLIP_ALL(512);
BENCHMARK_BOTH_FLIP_ALL(1024);
BENCHMARK_BOTH_FLIP_ALL(2048);

// ============================================================================
//  Single bit set/reset/test (sequential)
// ============================================================================

template <size_t N>
void BM_Fermat_SingleBitOps(benchmark::State& state) {
    fermat::Bitset<N> bs;
    for (auto _ : state) {
        for (size_t i = 0; i < N; ++i) {
            bs.set(i);
            bs.test(i);
            bs.reset(i);
        }
        benchmark::DoNotOptimize(bs);
    }
}

template <size_t N>
void BM_Std_SingleBitOps(benchmark::State& state) {
    std::bitset<N> bs;
    for (auto _ : state) {
        for (size_t i = 0; i < N; ++i) {
            bs.set(i);
            bs.test(i);
            bs.reset(i);
        }
        benchmark::DoNotOptimize(bs);
    }
}

template <size_t N>
void BM_Bitmap_SingleBitOps(benchmark::State& state) {
    auto& storage = GetBitmapStorage(N);
    fermat::BitmapView<true, uint64_t> bm;
    bm.setup({storage.data(), storage.size()}, N);
    for (auto _ : state) {
        for (size_t i = 0; i < N; ++i) {
            bm.set(i);
            bm.test(i);
            bm.reset(i);
        }
        benchmark::DoNotOptimize(bm);
    }
}

#define BENCHMARK_BOTH_SINGLE_BIT(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_SingleBitOps, size); \
    BENCHMARK_TEMPLATE(BM_Std_SingleBitOps, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_SingleBitOps, size)

BENCHMARK_BOTH_SINGLE_BIT(64);
BENCHMARK_BOTH_SINGLE_BIT(128);
BENCHMARK_BOTH_SINGLE_BIT(256);
BENCHMARK_BOTH_SINGLE_BIT(512);
BENCHMARK_BOTH_SINGLE_BIT(1024);
BENCHMARK_BOTH_SINGLE_BIT(2048);

// ============================================================================
//  Count bits set
// ============================================================================

template <size_t N>
void BM_Fermat_Count(benchmark::State& state) {
    auto bs = RandomBitset<N, fermat::Bitset>();
    for (auto _ : state) {
        auto c = bs.count();
        benchmark::DoNotOptimize(c);
    }
}

template <size_t N>
void BM_Std_Count(benchmark::State& state) {
    auto bs = RandomBitset<N, std::bitset>();
    for (auto _ : state) {
        auto c = bs.count();
        benchmark::DoNotOptimize(c);
    }
}

template <size_t N>
void BM_Bitmap_Count(benchmark::State& state) {
    auto bm = MakeRandomBitmap(N);
    for (auto _ : state) {
        auto c = bm.count();
        benchmark::DoNotOptimize(c);
    }
}

#define BENCHMARK_BOTH_COUNT(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_Count, size); \
    BENCHMARK_TEMPLATE(BM_Std_Count, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_Count, size)

BENCHMARK_BOTH_COUNT(64);
BENCHMARK_BOTH_COUNT(128);
BENCHMARK_BOTH_COUNT(256);
BENCHMARK_BOTH_COUNT(512);
BENCHMARK_BOTH_COUNT(1024);
BENCHMARK_BOTH_COUNT(2048);

// ============================================================================
//  any(), all(), none()
// ============================================================================

template <size_t N>
void BM_Fermat_AnyAllNone(benchmark::State& state) {
    auto bs = RandomBitset<N, fermat::Bitset>();
    for (auto _ : state) {
        benchmark::DoNotOptimize(bs.any());
        benchmark::DoNotOptimize(bs.all());
        benchmark::DoNotOptimize(bs.none());
    }
}

template <size_t N>
void BM_Std_AnyAllNone(benchmark::State& state) {
    auto bs = RandomBitset<N, std::bitset>();
    for (auto _ : state) {
        benchmark::DoNotOptimize(bs.any());
        benchmark::DoNotOptimize(bs.all());
        benchmark::DoNotOptimize(bs.none());
    }
}

template <size_t N>
void BM_Bitmap_AnyAllNone(benchmark::State& state) {
    auto bm = MakeRandomBitmap(N);
    for (auto _ : state) {
        benchmark::DoNotOptimize(bm.any());
        benchmark::DoNotOptimize(bm.all());
        benchmark::DoNotOptimize(bm.none());
    }
}

#define BENCHMARK_BOTH_ANY_ALL_NONE(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_AnyAllNone, size); \
    BENCHMARK_TEMPLATE(BM_Std_AnyAllNone, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_AnyAllNone, size)

BENCHMARK_BOTH_ANY_ALL_NONE(64);
BENCHMARK_BOTH_ANY_ALL_NONE(128);
BENCHMARK_BOTH_ANY_ALL_NONE(256);
BENCHMARK_BOTH_ANY_ALL_NONE(512);
BENCHMARK_BOTH_ANY_ALL_NONE(1024);
BENCHMARK_BOTH_ANY_ALL_NONE(2048);

// ============================================================================
//  Bitwise AND, OR, XOR
// ============================================================================

template <size_t N>
void BM_Fermat_BitwiseOps(benchmark::State& state) {
    auto a = RandomBitset<N, fermat::Bitset>();
    auto b = RandomBitset<N, fermat::Bitset>();
    for (auto _ : state) {
        auto c = a; c &= b; benchmark::DoNotOptimize(c);
        c = a; c |= b; benchmark::DoNotOptimize(c);
        c = a; c ^= b; benchmark::DoNotOptimize(c);
    }
}

template <size_t N>
void BM_Std_BitwiseOps(benchmark::State& state) {
    auto a = RandomBitset<N, std::bitset>();
    auto b = RandomBitset<N, std::bitset>();
    for (auto _ : state) {
        auto c = a; c &= b; benchmark::DoNotOptimize(c);
        c = a; c |= b; benchmark::DoNotOptimize(c);
        c = a; c ^= b; benchmark::DoNotOptimize(c);
    }
}

template <size_t N>
void BM_Bitmap_BitwiseOps(benchmark::State& state) {
    auto a = MakeRandomBitmap(N);
    auto b = MakeRandomBitmap(N);
    for (auto _ : state) {
        auto c = a; c &= b; benchmark::DoNotOptimize(c);
        c = a; c |= b; benchmark::DoNotOptimize(c);
        c = a; c ^= b; benchmark::DoNotOptimize(c);
    }
}

#define BENCHMARK_BOTH_BITWISE(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_BitwiseOps, size); \
    BENCHMARK_TEMPLATE(BM_Std_BitwiseOps, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_BitwiseOps, size)

BENCHMARK_BOTH_BITWISE(64);
BENCHMARK_BOTH_BITWISE(128);
BENCHMARK_BOTH_BITWISE(256);
BENCHMARK_BOTH_BITWISE(512);
BENCHMARK_BOTH_BITWISE(1024);
BENCHMARK_BOTH_BITWISE(2048);

// ============================================================================
//  Left shift
// ============================================================================

template <size_t N>
void BM_Fermat_ShiftLeft(benchmark::State& state) {
    auto bs = RandomBitset<N, fermat::Bitset>();
    for (auto _ : state) {
        auto bs2 = bs;
        bs2 <<= 3;
        benchmark::DoNotOptimize(bs2);
    }
}

template <size_t N>
void BM_Std_ShiftLeft(benchmark::State& state) {
    auto bs = RandomBitset<N, std::bitset>();
    for (auto _ : state) {
        auto bs2 = bs;
        bs2 <<= 3;
        benchmark::DoNotOptimize(bs2);
    }
}

template <size_t N>
void BM_Bitmap_ShiftLeft(benchmark::State& state) {
    auto bm = MakeRandomBitmap(N);
    for (auto _ : state) {
        auto bm2 = bm;
        bm2 <<= 3;
        benchmark::DoNotOptimize(bm2);
    }
}

#define BENCHMARK_BOTH_SHIFT_LEFT(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_ShiftLeft, size); \
    BENCHMARK_TEMPLATE(BM_Std_ShiftLeft, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_ShiftLeft, size)

BENCHMARK_BOTH_SHIFT_LEFT(64);
BENCHMARK_BOTH_SHIFT_LEFT(128);
BENCHMARK_BOTH_SHIFT_LEFT(256);
BENCHMARK_BOTH_SHIFT_LEFT(512);
BENCHMARK_BOTH_SHIFT_LEFT(1024);
BENCHMARK_BOTH_SHIFT_LEFT(2048);

// ============================================================================
//  Right shift
// ============================================================================

template <size_t N>
void BM_Fermat_ShiftRight(benchmark::State& state) {
    auto bs = RandomBitset<N, fermat::Bitset>();
    for (auto _ : state) {
        auto bs2 = bs;
        bs2 >>= 3;
        benchmark::DoNotOptimize(bs2);
    }
}

template <size_t N>
void BM_Std_ShiftRight(benchmark::State& state) {
    auto bs = RandomBitset<N, std::bitset>();
    for (auto _ : state) {
        auto bs2 = bs;
        bs2 >>= 3;
        benchmark::DoNotOptimize(bs2);
    }
}

template <size_t N>
void BM_Bitmap_ShiftRight(benchmark::State& state) {
    auto bm = MakeRandomBitmap(N);
    for (auto _ : state) {
        auto bm2 = bm;
        bm2 >>= 3;
        benchmark::DoNotOptimize(bm2);
    }
}

#define BENCHMARK_BOTH_SHIFT_RIGHT(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_ShiftRight, size); \
    BENCHMARK_TEMPLATE(BM_Std_ShiftRight, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_ShiftRight, size)

BENCHMARK_BOTH_SHIFT_RIGHT(64);
BENCHMARK_BOTH_SHIFT_RIGHT(128);
BENCHMARK_BOTH_SHIFT_RIGHT(256);
BENCHMARK_BOTH_SHIFT_RIGHT(512);
BENCHMARK_BOTH_SHIFT_RIGHT(1024);
BENCHMARK_BOTH_SHIFT_RIGHT(2048);

// ============================================================================
//  find_first (fermat::Bitset/BitmapView vs linear scan std)
// ============================================================================

template <size_t N>
void BM_Fermat_FindFirst(benchmark::State& state) {
    auto bs = RandomBitset<N, fermat::Bitset>();
    for (auto _ : state) {
        auto pos = bs.find_first();
        benchmark::DoNotOptimize(pos);
    }
}

template <size_t N>
void BM_Std_FindFirst(benchmark::State& state) {
    auto bs = RandomBitset<N, std::bitset>();
    for (auto _ : state) {
        auto pos = StdFindFirst(bs);
        benchmark::DoNotOptimize(pos);
    }
}

template <size_t N>
void BM_Bitmap_FindFirst(benchmark::State& state) {
    auto bm = MakeRandomBitmap(N);
    for (auto _ : state) {
        auto pos = bm.find_first();
        benchmark::DoNotOptimize(pos);
    }
}

#define BENCHMARK_BOTH_FIND_FIRST(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_FindFirst, size); \
    BENCHMARK_TEMPLATE(BM_Std_FindFirst, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_FindFirst, size)

BENCHMARK_BOTH_FIND_FIRST(64);
BENCHMARK_BOTH_FIND_FIRST(128);
BENCHMARK_BOTH_FIND_FIRST(256);
BENCHMARK_BOTH_FIND_FIRST(512);
BENCHMARK_BOTH_FIND_FIRST(1024);
BENCHMARK_BOTH_FIND_FIRST(2048);

// ============================================================================
//  find_next (fermat::Bitset/BitmapView vs linear scan std)
// ============================================================================

template <size_t N>
void BM_Fermat_FindNext(benchmark::State& state) {
    auto bs = RandomBitset<N, fermat::Bitset>();
    size_t prev = bs.find_first();
    if (prev == N) prev = 0;
    for (auto _ : state) {
        auto pos = bs.find_next(prev);
        benchmark::DoNotOptimize(pos);
    }
}

template <size_t N>
void BM_Std_FindNext(benchmark::State& state) {
    auto bs = RandomBitset<N, std::bitset>();
    size_t prev = StdFindFirst(bs);
    if (prev == N) prev = 0;
    for (auto _ : state) {
        auto pos = StdFindNext(bs, prev);
        benchmark::DoNotOptimize(pos);
    }
}

template <size_t N>
void BM_Bitmap_FindNext(benchmark::State& state) {
    auto bm = MakeRandomBitmap(N);
    size_t prev = bm.find_first();
    if (prev == N) prev = 0;
    for (auto _ : state) {
        auto pos = bm.find_next(prev);
        benchmark::DoNotOptimize(pos);
    }
}

#define BENCHMARK_BOTH_FIND_NEXT(size)          \
    BENCHMARK_TEMPLATE(BM_Fermat_FindNext, size); \
    BENCHMARK_TEMPLATE(BM_Std_FindNext, size);    \
    BENCHMARK_TEMPLATE(BM_Bitmap_FindNext, size)

BENCHMARK_BOTH_FIND_NEXT(64);
BENCHMARK_BOTH_FIND_NEXT(128);
BENCHMARK_BOTH_FIND_NEXT(256);
BENCHMARK_BOTH_FIND_NEXT(512);
BENCHMARK_BOTH_FIND_NEXT(1024);
BENCHMARK_BOTH_FIND_NEXT(2048);

BENCHMARK_MAIN();