/// compressed_pair_gtest.cpp
/// Google Test conversion of range-v3 compressed_pair test.
/// All comments in English, using /// Doxygen style.

#include <gtest/gtest.h>
#include <fermat/utility/compressed_pair.h>   /// Assume Fermat provides compressed_pair
#include <fermat/utility/box.h>

/// Helper macro for empty base optimization (compiler-specific).
/// Fermat may define RANGES_EMPTY_BASES similarly; if not, define it empty.
#ifndef RANGES_EMPTY_BASES
#define RANGES_EMPTY_BASES __attribute__((__empty_bases__))
#endif

using namespace ranges;   /// Use Fermat's ranges namespace (assuming it contains compressed_pair)

/// Test for issue #1093: compressed_pair should be empty‑base‑optimized.
void test_1093() {
    struct Op {};
    struct Op2 {};

    struct payload { void* v; };
    struct base_adaptor {};

    struct RANGES_EMPTY_BASES A : base_adaptor, private ranges::box<Op, A> {};
    struct RANGES_EMPTY_BASES B : base_adaptor, private ranges::box<Op2, B> {};

    using P  = compressed_pair<A, payload>;
    using P2 = compressed_pair<B, P>;

    EXPECT_EQ(sizeof(P), sizeof(payload));
    EXPECT_EQ(sizeof(P2), sizeof(P));
}

TEST(CompressedPairTest, Issue1093) {
    test_1093();
}
