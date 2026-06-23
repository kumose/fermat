#include <gtest/gtest.h>
#include <vector>
#include <array>
#include <initializer_list>
#include <fermat/algorithm/partition.h>
#include <fermat/view/subrange.h>

/// predicate for testing
struct is_odd {
    constexpr bool operator()(const int& i) const { return i & 1; }
};

/// test partition with iterator pairs (raw pointers)
void test_iter() {
    int ia[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    // mixed
    int* r = fermat::ranges::partition(ia, ia + sa, is_odd{});
    EXPECT_EQ(r, ia + 5);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // empty
    r = fermat::ranges::partition(ia, ia, is_odd{});
    EXPECT_EQ(r, ia);

    // all false
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i;
    r = fermat::ranges::partition(ia, ia + sa, is_odd{});
    EXPECT_EQ(r, ia);

    // all true
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i + 1;
    r = fermat::ranges::partition(ia, ia + sa, is_odd{});
    EXPECT_EQ(r, ia + sa);

    // all true but last
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i + 1;
    ia[sa - 1] = 10;
    r = fermat::ranges::partition(ia, ia + sa, is_odd{});
    EXPECT_EQ(r, ia + sa - 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // all true but first
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i + 1;
    ia[0] = 10;
    r = fermat::ranges::partition(ia, ia + sa, is_odd{});
    EXPECT_EQ(r, ia + sa - 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // all false but last
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i;
    ia[sa - 1] = 11;
    r = fermat::ranges::partition(ia, ia + sa, is_odd{});
    EXPECT_EQ(r, ia + 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // all false but first
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i;
    ia[0] = 11;
    r = fermat::ranges::partition(ia, ia + sa, is_odd{});
    EXPECT_EQ(r, ia + 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));
}

/// test partition with range (using subrange)
void test_range() {
    int ia[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    // mixed
    auto r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia + sa), is_odd{});
    EXPECT_EQ(r, ia + 5);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // empty
    r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia), is_odd{});
    EXPECT_EQ(r, ia);

    // all false
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i;
    r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia + sa), is_odd{});
    EXPECT_EQ(r, ia);

    // all true
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i + 1;
    r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia + sa), is_odd{});
    EXPECT_EQ(r, ia + sa);

    // all true but last
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i + 1;
    ia[sa - 1] = 10;
    r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia + sa), is_odd{});
    EXPECT_EQ(r, ia + sa - 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // all true but first
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i + 1;
    ia[0] = 10;
    r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia + sa), is_odd{});
    EXPECT_EQ(r, ia + sa - 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // all false but last
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i;
    ia[sa - 1] = 11;
    r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia + sa), is_odd{});
    EXPECT_EQ(r, ia + 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));

    // all false but first
    for (std::size_t i = 0; i < sa; ++i) ia[i] = 2 * i;
    ia[0] = 11;
    r = fermat::ranges::partition(fermat::ranges::make_subrange(ia, ia + sa), is_odd{});
    EXPECT_EQ(r, ia + 1);
    for (int* i = ia; i < r; ++i) EXPECT_TRUE(is_odd{}(*i));
    for (int* i = r; i < ia + sa; ++i) EXPECT_FALSE(is_odd{}(*i));
}

/// projection test
struct S { int i; };

TEST(PartitionTest, IteratorPair) {
    test_iter();
}

TEST(PartitionTest, Range) {
    test_range();
}

TEST(PartitionTest, Projection) {
    S arr[] = {S{1}, S{2}, S{3}, S{4}, S{5}, S{6}, S{7}, S{8}, S{9}};
    constexpr std::size_t sa = sizeof(arr) / sizeof(arr[0]);
    S* r = fermat::ranges::partition(arr, is_odd{}, &S::i);
    EXPECT_EQ(r, arr + 5);
    for (S* i = arr; i < r; ++i) EXPECT_TRUE(is_odd{}(i->i));
    for (S* i = r; i < arr + sa; ++i) EXPECT_FALSE(is_odd{}(i->i));
}

/// rvalue range test (compile only, we just check dangling is not required)
TEST(PartitionTest, RvalueRange) {
    S arr[] = {S{1}, S{2}, S{3}, S{4}, S{5}, S{6}, S{7}, S{8}, S{9}};
    auto r = fermat::ranges::partition(std::move(arr), is_odd{}, &S::i);
    // r is a dangling iterator; no runtime check needed.
    (void)r;
    std::vector<S> vec(std::begin(arr), std::end(arr));
    auto r3 = fermat::ranges::partition(std::move(vec), is_odd{}, &S::i);
    (void)r3;
}

/// constexpr test
TEST(PartitionTest, Constexpr) {
    constexpr auto test = []() constexpr {
        std::array<int, 9> arr{1, 2, 3, 4, 5, 6, 7, 8, 9};
        int* r = fermat::ranges::partition(arr.begin(), arr.end(), is_odd{});
        bool ok = (r == arr.begin() + 5);
        for (int* i = arr.begin(); i < r; ++i) ok = ok && (is_odd{})(*i);
        for (int* i = r; i < arr.end(); ++i) ok = ok && !(is_odd{})(*i);
        return ok;
    };
    static_assert(test(), "");
}
