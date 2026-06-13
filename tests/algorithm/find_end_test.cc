#include <gtest/gtest.h>
#include <array>
#include <fermat/algorithm/find_end.h>
#include <fermat/view/subrange.h>

namespace {
    struct count_equal {
        static unsigned count;
        template<class T>
        bool operator()(const T& x, const T& y) {
            ++count;
            return x == y;
        }
    };
    unsigned count_equal::count = 0;

    struct S { int i_; };
} // namespace

// constexpr test
constexpr bool test_constexpr() {
    using namespace fermat::ranges;
    int ia[] = {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 0, 1, 2, 3, 0, 1, 2, 0, 1, 0};
    auto ia_b = begin(ia);
    auto ia_e = end(ia);
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int b[] = {0};
    int c[] = {0, 1};
    int d[] = {0, 1, 2};
    int e[] = {0, 1, 2, 3};
    int f[] = {0, 1, 2, 3, 4};
    int g[] = {0, 1, 2, 3, 4, 5};
    int h[] = {0, 1, 2, 3, 4, 5, 6};
    bool ok = true;
    ok = ok && (find_end(ia_b, ia_e, begin(b), b + 1).begin() == ia + sa - 1);
    ok = ok && (find_end(ia_b, ia_e, begin(c), c + 2).begin() == ia + 18);
    ok = ok && (find_end(ia_b, ia_e, begin(d), d + 3).begin() == ia + 15);
    ok = ok && (find_end(ia_b, ia_e, begin(e), e + 4).begin() == ia + 11);
    ok = ok && (find_end(ia_b, ia_e, begin(f), f + 5).begin() == ia + 6);
    ok = ok && (find_end(ia_b, ia_e, begin(g), g + 6).begin() == ia);
    ok = ok && (find_end(ia_b, ia_e, begin(h), h + 7).begin() == ia + sa);
    ok = ok && (find_end(ia_b, ia_e, begin(b), b).begin() == ia + sa);
    ok = ok && (find_end(ia_b, ia_b, begin(b), b + 1).begin() == ia);
    return ok;
}
static_assert(test_constexpr(), "");

TEST(FindEndTest, Basic) {
    using namespace fermat::ranges;
    int ia[] = {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 0, 1, 2, 3, 0, 1, 2, 0, 1, 0};
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int b[] = {0};
    int c[] = {0, 1};
    int d[] = {0, 1, 2};
    int e[] = {0, 1, 2, 3};
    int f[] = {0, 1, 2, 3, 4};
    int g[] = {0, 1, 2, 3, 4, 5};
    int h[] = {0, 1, 2, 3, 4, 5, 6};

    // iterator pairs
    auto res = find_end(ia, ia + sa, b, b + 1);
    EXPECT_EQ(res.begin(), ia + sa - 1);
    EXPECT_EQ(res.end(), ia + sa);

    res = find_end(ia, ia + sa, c, c + 2);
    EXPECT_EQ(res.begin(), ia + 18);
    EXPECT_EQ(res.end(), ia + 20);

    res = find_end(ia, ia + sa, d, d + 3);
    EXPECT_EQ(res.begin(), ia + 15);
    EXPECT_EQ(res.end(), ia + 18);

    res = find_end(ia, ia + sa, e, e + 4);
    EXPECT_EQ(res.begin(), ia + 11);
    EXPECT_EQ(res.end(), ia + 15);

    res = find_end(ia, ia + sa, f, f + 5);
    EXPECT_EQ(res.begin(), ia + 6);
    EXPECT_EQ(res.end(), ia + 11);

    res = find_end(ia, ia + sa, g, g + 6);
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia + 6);

    res = find_end(ia, ia + sa, h, h + 7);
    EXPECT_EQ(res.begin(), ia + sa);
    EXPECT_EQ(res.end(), ia + sa);

    res = find_end(ia, ia + sa, b, b);
    EXPECT_EQ(res.begin(), ia + sa);
    EXPECT_EQ(res.end(), ia + sa);

    res = find_end(ia, ia, b, b + 1);
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(res.end(), ia);

    // subrange version
    auto ir = make_subrange(ia, ia + sa);
    res = find_end(ir, make_subrange(b, b + 1));
    EXPECT_EQ(res.begin(), ia + sa - 1);
    res = find_end(ir, make_subrange(c, c + 2));
    EXPECT_EQ(res.begin(), ia + 18);
    res = find_end(ir, make_subrange(d, d + 3));
    EXPECT_EQ(res.begin(), ia + 15);
    res = find_end(ir, make_subrange(e, e + 4));
    EXPECT_EQ(res.begin(), ia + 11);
    res = find_end(ir, make_subrange(f, f + 5));
    EXPECT_EQ(res.begin(), ia + 6);
    res = find_end(ir, make_subrange(g, g + 6));
    EXPECT_EQ(res.begin(), ia);
    res = find_end(ir, make_subrange(h, h + 7));
    EXPECT_EQ(res.begin(), ia + sa);
    res = find_end(ir, make_subrange(b, b));
    EXPECT_EQ(res.begin(), ia + sa);
}

TEST(FindEndTest, WithPredicate) {
    using namespace fermat::ranges;
    int ia[] = {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 0, 1, 2, 3, 0, 1, 2, 0, 1, 0};
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int b[] = {0};
    int c[] = {0, 1};
    int d[] = {0, 1, 2};
    int e[] = {0, 1, 2, 3};
    int f[] = {0, 1, 2, 3, 4};
    int g[] = {0, 1, 2, 3, 4, 5};
    int h[] = {0, 1, 2, 3, 4, 5, 6};

    count_equal::count = 0;
    auto res = find_end(ia, ia + sa, b, b + 1, count_equal());
    EXPECT_EQ(res.begin(), ia + sa - 1);
    EXPECT_LE(count_equal::count, 1 * (sa - 1 + 1));

    count_equal::count = 0;
    res = find_end(ia, ia + sa, c, c + 2, count_equal());
    EXPECT_EQ(res.begin(), ia + 18);
    EXPECT_LE(count_equal::count, 2 * (sa - 2 + 1));

    count_equal::count = 0;
    res = find_end(ia, ia + sa, d, d + 3, count_equal());
    EXPECT_EQ(res.begin(), ia + 15);
    EXPECT_LE(count_equal::count, 3 * (sa - 3 + 1));

    count_equal::count = 0;
    res = find_end(ia, ia + sa, e, e + 4, count_equal());
    EXPECT_EQ(res.begin(), ia + 11);
    EXPECT_LE(count_equal::count, 4 * (sa - 4 + 1));

    count_equal::count = 0;
    res = find_end(ia, ia + sa, f, f + 5, count_equal());
    EXPECT_EQ(res.begin(), ia + 6);
    EXPECT_LE(count_equal::count, 5 * (sa - 5 + 1));

    count_equal::count = 0;
    res = find_end(ia, ia + sa, g, g + 6, count_equal());
    EXPECT_EQ(res.begin(), ia);
    EXPECT_LE(count_equal::count, 6 * (sa - 6 + 1));

    count_equal::count = 0;
    res = find_end(ia, ia + sa, h, h + 7, count_equal());
    EXPECT_EQ(res.begin(), ia + sa);
    EXPECT_LE(count_equal::count, 7 * (sa - 7 + 1));

    count_equal::count = 0;
    res = find_end(ia, ia + sa, b, b, count_equal());
    EXPECT_EQ(res.begin(), ia + sa);
    EXPECT_EQ(count_equal::count, 0u);

    count_equal::count = 0;
    res = find_end(ia, ia, b, b + 1, count_equal());
    EXPECT_EQ(res.begin(), ia);
    EXPECT_EQ(count_equal::count, 0u);

    // subrange version with predicate
    auto ir = make_subrange(ia, ia + sa);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(b, b + 1), count_equal());
    EXPECT_EQ(res.begin(), ia + sa - 1);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(c, c + 2), count_equal());
    EXPECT_EQ(res.begin(), ia + 18);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(d, d + 3), count_equal());
    EXPECT_EQ(res.begin(), ia + 15);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(e, e + 4), count_equal());
    EXPECT_EQ(res.begin(), ia + 11);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(f, f + 5), count_equal());
    EXPECT_EQ(res.begin(), ia + 6);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(g, g + 6), count_equal());
    EXPECT_EQ(res.begin(), ia);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(h, h + 7), count_equal());
    EXPECT_EQ(res.begin(), ia + sa);
    count_equal::count = 0;
    res = find_end(ir, make_subrange(b, b), count_equal());
    EXPECT_EQ(res.begin(), ia + sa);
}

TEST(FindEndTest, WithProjection) {
    using namespace fermat::ranges;
    S ia[] = {{0}, {1}, {2}, {3}, {4}, {5}, {0}, {1}, {2}, {3}, {4}, {0}, {1}, {2}, {3}, {0}, {1}, {2}, {0}, {1}, {0}};
    constexpr auto sa = sizeof(ia) / sizeof(ia[0]);
    int b[] = {0};
    int c[] = {0, 1};
    int d[] = {0, 1, 2};
    int e[] = {0, 1, 2, 3};
    int f[] = {0, 1, 2, 3, 4};
    int g[] = {0, 1, 2, 3, 4, 5};
    int h[] = {0, 1, 2, 3, 4, 5, 6};

    auto res = find_end(ia, ia + sa, b, b + 1, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + sa - 1);

    res = find_end(ia, ia + sa, c, c + 2, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 18);

    res = find_end(ia, ia + sa, d, d + 3, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 15);

    res = find_end(ia, ia + sa, e, e + 4, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 11);

    res = find_end(ia, ia + sa, f, f + 5, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 6);

    res = find_end(ia, ia + sa, g, g + 6, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia);

    res = find_end(ia, ia + sa, h, h + 7, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + sa);

    res = find_end(ia, ia + sa, b, b, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + sa);

    res = find_end(ia, ia, b, b + 1, equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia);

    // subrange version
    auto ir = make_subrange(ia, ia + sa);
    res = find_end(ir, make_subrange(b, b + 1), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + sa - 1);
    res = find_end(ir, make_subrange(c, c + 2), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 18);
    res = find_end(ir, make_subrange(d, d + 3), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 15);
    res = find_end(ir, make_subrange(e, e + 4), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 11);
    res = find_end(ir, make_subrange(f, f + 5), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + 6);
    res = find_end(ir, make_subrange(g, g + 6), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia);
    res = find_end(ir, make_subrange(h, h + 7), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + sa);
    res = find_end(ir, make_subrange(b, b), equal_to(), &S::i_);
    EXPECT_EQ(res.begin(), ia + sa);
}
