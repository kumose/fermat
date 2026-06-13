#include <gtest/gtest.h>
#include <vector>
#include <initializer_list>
#include <fermat/core.h>
#include <fermat/algorithm/all_of.h>

namespace {
    constexpr bool even(int n) { return n % 2 == 0; }

    struct S {
        bool test;
        S(bool p) : test(p) {}
        bool p() const { return test; }
    };
} // namespace

TEST(AllOfTest, VectorIterators) {
    std::vector<int> all_even{0, 2, 4, 6};
    std::vector<int> one_even{1, 3, 4, 7};
    std::vector<int> none_even{1, 3, 5, 7};

    EXPECT_TRUE(ranges::all_of(all_even.begin(), all_even.end(), even));
    EXPECT_FALSE(ranges::all_of(one_even.begin(), one_even.end(), even));
    EXPECT_FALSE(ranges::all_of(none_even.begin(), none_even.end(), even));
}

TEST(AllOfTest, VectorRange) {
    std::vector<int> all_even{0, 2, 4, 6};
    std::vector<int> one_even{1, 3, 4, 7};
    std::vector<int> none_even{1, 3, 5, 7};

    EXPECT_TRUE(ranges::all_of(all_even, even));
    EXPECT_FALSE(ranges::all_of(one_even, even));
    EXPECT_FALSE(ranges::all_of(none_even, even));
}

TEST(AllOfTest, InitializerList) {
    using ILI = std::initializer_list<int>;
    EXPECT_TRUE(ranges::all_of(ILI{0, 2, 4, 6}, [](int n) { return n % 2 == 0; }));
    EXPECT_FALSE(ranges::all_of(ILI{1, 3, 4, 7}, [](int n) { return n % 2 == 0; }));
    EXPECT_FALSE(ranges::all_of(ILI{1, 3, 5, 7}, [](int n) { return n % 2 == 0; }));
}

TEST(AllOfTest, StructSVectorIterators) {
    std::vector<S> all_true{S(true), S(true), S(true)};
    std::vector<S> one_true{S(false), S(false), S(true)};
    std::vector<S> none_true{S(false), S(false), S(false)};

    EXPECT_TRUE(ranges::all_of(all_true.begin(), all_true.end(), &S::p));
    EXPECT_FALSE(ranges::all_of(one_true.begin(), one_true.end(), &S::p));
    EXPECT_FALSE(ranges::all_of(none_true.begin(), none_true.end(), &S::p));
}

TEST(AllOfTest, StructSVectorRange) {
    std::vector<S> all_true{S(true), S(true), S(true)};
    std::vector<S> one_true{S(false), S(false), S(true)};
    std::vector<S> none_true{S(false), S(false), S(false)};

    EXPECT_TRUE(ranges::all_of(all_true, &S::p));
    EXPECT_FALSE(ranges::all_of(one_true, &S::p));
    EXPECT_FALSE(ranges::all_of(none_true, &S::p));
}

TEST(AllOfTest, StructSInitializerList) {
    using ILS = std::initializer_list<S>;
    EXPECT_TRUE(ranges::all_of(ILS{S(true), S(true), S(true)}, &S::p));
    EXPECT_FALSE(ranges::all_of(ILS{S(false), S(true), S(false)}, &S::p));
    EXPECT_FALSE(ranges::all_of(ILS{S(false), S(false), S(false)}, &S::p));
}

TEST(AllOfTest, Constexpr) {
    constexpr auto test = []() constexpr {
        auto check = [](std::initializer_list<int> il) constexpr {
            return ranges::all_of(il, even);
        };
        bool ok = check({0, 2, 4, 6}) && !check({0, 2, 4, 5}) &&
                  !check({1, 3, 4, 7}) && !check({1, 3, 5, 7});
        return ok;
    };
    static_assert(test(), "constexpr all_of test failed");
}