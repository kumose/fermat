#include <gtest/gtest.h>
#include <cstring>
#include <algorithm>
#include <vector>
#include <array>
#include <initializer_list>
#include <fermat/algorithm/permutation.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/subrange.h>

// Helper: factorial
int factorial(int x) {
    int r = 1;
    for (; x; --x) r *= x;
    return r;
}

// Test next_permutation on a mutable array (raw pointer version)
void test_permutation_on_array(int* first, int* last) {
    int n = static_cast<int>(last - first);
    int count = 0;
    bool has_next;
    std::vector<int> prev(n);
    do {
        std::copy(first, last, prev.begin());
        has_next = ranges::next_permutation(first, last);
        if (n > 1) {
            if (has_next)
                EXPECT_TRUE(std::lexicographical_compare(prev.begin(), prev.end(), first, last));
            else
                EXPECT_TRUE(std::lexicographical_compare(first, last, prev.begin(), prev.end()));
        }
        ++count;
    } while (has_next);
    EXPECT_EQ(count, factorial(n));
}

TEST(NextPermutationTest, Basic) {
    int arr[] = {1, 2, 3, 4, 5, 6};
    for (int e = 0; e <= 6; ++e) {
        test_permutation_on_array(arr, arr + e);
    }
}

TEST(NextPermutationTest, WithComparator) {
    int arr[] = {6, 5, 4, 3, 2, 1};
    const int n = sizeof(arr)/sizeof(arr[0]);
    (void)n; // suppress unused variable warning
    for (int e = 0; e <= 6; ++e) {
        int count = 0;
        bool has_next;
        std::vector<int> prev(e);
        do {
            std::copy(arr, arr + e, prev.begin());
            has_next = ranges::next_permutation(arr, arr + e, std::greater<int>());
            if (e > 1) {
                if (has_next)
                    EXPECT_TRUE(std::lexicographical_compare(prev.begin(), prev.end(), arr, arr + e, std::greater<int>()));
                else
                    EXPECT_TRUE(std::lexicographical_compare(arr, arr + e, prev.begin(), prev.end(), std::greater<int>()));
            }
            ++count;
        } while (has_next);
        EXPECT_EQ(count, factorial(e));
    }
}

TEST(NextPermutationTest, RangeVersion) {
    int arr[] = {1, 2, 3, 4, 5, 6};
    for (int e = 0; e <= 6; ++e) {
        int count = 0;
        bool has_next;
        std::vector<int> prev(e);
        do {
            std::copy(arr, arr + e, prev.begin());
            auto rng = ranges::subrange(arr, arr + e);   // use subrange constructor
            has_next = ranges::next_permutation(rng);
            if (e > 1) {
                if (has_next)
                    EXPECT_TRUE(std::lexicographical_compare(prev.begin(), prev.end(), arr, arr + e));
                else
                    EXPECT_TRUE(std::lexicographical_compare(arr, arr + e, prev.begin(), prev.end()));
            }
            ++count;
        } while (has_next);
        EXPECT_EQ(count, factorial(e));
    }
}

TEST(NextPermutationTest, RangeWithComparator) {
    int arr[] = {6, 5, 4, 3, 2, 1};
    for (int e = 0; e <= 6; ++e) {
        int count = 0;
        bool has_next;
        std::vector<int> prev(e);
        do {
            std::copy(arr, arr + e, prev.begin());
            auto rng = ranges::subrange(arr, arr + e);
            has_next = ranges::next_permutation(rng, std::greater<int>());
            if (e > 1) {
                if (has_next)
                    EXPECT_TRUE(std::lexicographical_compare(prev.begin(), prev.end(), arr, arr + e, std::greater<int>()));
                else
                    EXPECT_TRUE(std::lexicographical_compare(arr, arr + e, prev.begin(), prev.end(), std::greater<int>()));
            }
            ++count;
        } while (has_next);
        EXPECT_EQ(count, factorial(e));
    }
}

struct c_str {
    char const* value;
    friend bool operator==(c_str a, c_str b) { return std::strcmp(a.value, b.value) == 0; }
    friend bool operator!=(c_str a, c_str b) { return !(a == b); }
};

TEST(NextPermutationTest, Projection) {
    using Pair = std::pair<int, c_str>;
    Pair arr[] = {
        {6, {"six"}}, {5,{"five"}}, {4,{"four"}}, {3,{"three"}}, {2,{"two"}}, {1,{"one"}}
    };
    auto comp = std::greater<int>();
    auto proj = &Pair::first;

    EXPECT_TRUE(ranges::next_permutation(arr, comp, proj));
    Pair expected1[] = {{6,{"six"}}, {5,{"five"}}, {4,{"four"}}, {3,{"three"}}, {1,{"one"}}, {2,{"two"}}};
    EXPECT_TRUE(ranges::equal(arr, expected1));

    EXPECT_TRUE(ranges::next_permutation(arr, comp, proj));
    Pair expected2[] = {{6,{"six"}}, {5,{"five"}}, {4,{"four"}}, {2,{"two"}}, {3,{"three"}}, {1,{"one"}}};
    EXPECT_TRUE(ranges::equal(arr, expected2));

    EXPECT_TRUE(ranges::next_permutation(arr, comp, proj));
    Pair expected3[] = {{6,{"six"}}, {5,{"five"}}, {4,{"four"}}, {2,{"two"}}, {1,{"one"}}, {3,{"three"}}};
    EXPECT_TRUE(ranges::equal(arr, expected3));
}

TEST(NextPermutationTest, Constexpr) {
    using namespace ranges;
    constexpr auto test = []() constexpr {
        int arr[] = {6,5,4,3,2,1};
        next_permutation(arr, std::greater<int>());
        bool ok = equal(arr, std::initializer_list<int>{6,5,4,3,1,2});
        next_permutation(arr, std::greater<int>());
        ok = ok && equal(arr, std::initializer_list<int>{6,5,4,2,3,1});
        next_permutation(arr, std::greater<int>());
        ok = ok && equal(arr, std::initializer_list<int>{6,5,4,2,1,3});
        return ok;
    };
    static_assert(test(), "constexpr next_permutation test failed");
}
