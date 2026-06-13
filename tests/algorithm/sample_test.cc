#include <gtest/gtest.h>
#include <array>
#include <random>
#include <fermat/algorithm/sample.h>
#include <fermat/algorithm/equal.h>
#include <fermat/numeric/iota.h>
#include <fermat/view/subrange.h>

/// Helper: check that [first, mid) is a prefix of [first, last)
/// (i.e., mid lies within [first, last])
template<typename I, typename S>
bool in_sequence(I first, I mid, S last) {
    for (; first != mid; ++first) {
        if (first == last) return false;
    }
    for (; first != last; ++first) {}
    return true;
}

TEST(SampleTest, BasicWithGenerator) {
    constexpr std::size_t N = 100;
    constexpr std::size_t K = 10;
    std::array<int, N> src;
    ranges::iota(src, 0);
    std::array<int, K> a{}, b{}, c{};
    std::minstd_rand g1, g2 = g1;

    // iterator pair, output iterator
    auto res = ranges::sample(src.data(), src.data() + N,
                              a.begin(), K, g1);
    EXPECT_TRUE(in_sequence(src.data(), res.in, src.data() + N));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, c));

    // range as input, output iterator
    res = ranges::sample(src.begin(), src.end(), b.begin(), K, g1);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_FALSE(ranges::equal(a, b));
    EXPECT_FALSE(ranges::equal(b, c));

    // same generator repeats previous result
    res = ranges::sample(src.begin(), src.end(), c.begin(), K, g2);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, c.end());
    EXPECT_TRUE(ranges::equal(a, c));
}

TEST(SampleTest, RangeVersionWithGenerator) {
    constexpr std::size_t N = 100;
    constexpr std::size_t K = 10;
    std::array<int, N> src;
    ranges::iota(src, 0);
    std::array<int, K> a{}, b{}, c{};
    std::minstd_rand g1, g2 = g1;
    auto rng = ranges::make_subrange(src.data(), src.data() + N);

    // rng as input, output iterator
    auto res = ranges::sample(rng, a.begin(), K, g1);
    EXPECT_TRUE(in_sequence(rng.begin(), res.in, rng.end()));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, b));

    // whole array as range, output iterator
    res = ranges::sample(src, b.begin(), K, g2);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_TRUE(ranges::equal(a, b));

    res = ranges::sample(src, b.begin(), K, g1);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_FALSE(ranges::equal(a, b));
    EXPECT_FALSE(ranges::equal(b, c));

    a.fill(0);
    res = ranges::sample(std::move(rng), a.begin(), K, g1);
    EXPECT_TRUE(in_sequence(src.data(), res.in, src.data() + N));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, c));
}

TEST(SampleTest, WithoutGenerator) {
    constexpr std::size_t N = 100;
    constexpr std::size_t K = 10;
    std::array<int, N> src;
    ranges::iota(src, 0);
    std::array<int, K> a{}, b{}, c{};

    // iterator pair, output iterator
    auto res = ranges::sample(src.data(), src.data() + N, a.begin(), K);
    EXPECT_TRUE(in_sequence(src.data(), res.in, src.data() + N));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, b));

    // range, output iterator
    res = ranges::sample(src, b.begin(), K);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_FALSE(ranges::equal(b, c));
    EXPECT_FALSE(ranges::equal(a, b));
}

TEST(SampleTest, ContainerOutput) {
    constexpr std::size_t N = 100;
    constexpr std::size_t K = 10;
    std::array<int, N> src;
    ranges::iota(src, 0);
    std::array<int, K> a{}, b{}, c{};
    std::minstd_rand g1, g2 = g1;

    // output container (range), with generator
    auto res = ranges::sample(src.data(), src.data() + N, a, g1);
    EXPECT_TRUE(in_sequence(src.data(), res.in, src.data() + N));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, c));

    res = ranges::sample(src.begin(), src.end(), b, g1);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_FALSE(ranges::equal(a, b));
    EXPECT_FALSE(ranges::equal(b, c));

    res = ranges::sample(src.begin(), src.end(), c, g2);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, c.end());
    EXPECT_TRUE(ranges::equal(a, c));
}

TEST(SampleTest, ContainerOutputRangeVersion) {
    constexpr std::size_t N = 100;
    constexpr std::size_t K = 10;
    std::array<int, N> src;
    ranges::iota(src, 0);
    std::array<int, K> a{}, b{}, c{};
    std::minstd_rand g1, g2 = g1;
    auto rng = ranges::make_subrange(src.data(), src.data() + N);

    // rng as input, output container
    auto res = ranges::sample(rng, a, g1);
    EXPECT_TRUE(in_sequence(src.data(), res.in, src.data() + N));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, b));

    res = ranges::sample(src, b, g2);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_TRUE(ranges::equal(a, b));

    res = ranges::sample(src, b, g1);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_FALSE(ranges::equal(a, b));
    EXPECT_FALSE(ranges::equal(b, c));

    a.fill(0);
    res = ranges::sample(std::move(rng), a, g1);
    EXPECT_TRUE(in_sequence(src.data(), res.in, src.data() + N));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, c));
}

TEST(SampleTest, ContainerOutputWithoutGenerator) {
    constexpr std::size_t N = 100;
    constexpr std::size_t K = 10;
    std::array<int, N> src;
    ranges::iota(src, 0);
    std::array<int, K> a{}, b{}, c{};

    // iterator pair, output container
    auto res = ranges::sample(src.data(), src.data() + N, a);
    EXPECT_TRUE(in_sequence(src.data(), res.in, src.data() + N));
    EXPECT_EQ(res.out, a.end());
    EXPECT_FALSE(ranges::equal(a, b));

    // range, output container
    res = ranges::sample(src, b);
    EXPECT_TRUE(in_sequence(src.begin(), res.in, src.end()));
    EXPECT_EQ(res.out, b.end());
    EXPECT_FALSE(ranges::equal(b, c));
    EXPECT_FALSE(ranges::equal(a, b));
}

TEST(SampleTest, ShortInput) {
    int data[] = {0, 1, 2, 3};
    int sample[2];
    std::minstd_rand g;

    // with generator
    auto res = ranges::sample(data, sample, g);
    EXPECT_TRUE(in_sequence(std::begin(data), res.in, std::end(data)));
    EXPECT_EQ(res.out, std::end(sample));

    // without generator
    res = ranges::sample(data, sample);
    EXPECT_TRUE(in_sequence(std::begin(data), res.in, std::end(data)));
    EXPECT_EQ(res.out, std::end(sample));

    // request more than available
    res = ranges::sample(data, data + 2, sample, 9999);
    EXPECT_EQ(res.in, data + 2);
    EXPECT_EQ(res.out, sample + 2);
}
