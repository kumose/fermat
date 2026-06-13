#include <gtest/gtest.h>
#include <vector>
#include <initializer_list>
#include <fermat/core.h>
#include <fermat/algorithm/any_of.h>

namespace {
    constexpr bool even(int n) { return n % 2 == 0; }

    struct S {
        bool test;
        S(bool p) : test(p) {}
        bool p() const { return test; }
    };
} // namespace

TEST(AnyOfTest, VectorIterators) {
    std::vector<int> all_even{0, 2, 4, 6};
    std::vector<int> one_even{1, 3, 4, 7};
    std::vector<int> none_even{1, 3, 5, 7};

    EXPECT_TRUE(ranges::any_of(all_even.begin(), all_even.end(), even));
    EXPECT_TRUE(ranges::any_of(one_even.begin(), one_even.end(), even));
    EXPECT_FALSE(ranges::any_of(none_even.begin(), none_even.end(), even));
}

TEST(AnyOfTest, VectorRange) {
    std::vector<int> all_even{0, 2, 4, 6};
    std::vector<int> one_even{1, 3, 4, 7};
    std::vector<int> none_even{1, 3, 5, 7};

    EXPECT_TRUE(ranges::any_of(all_even, even));
    EXPECT_TRUE(ranges::any_of(one_even, even));
    EXPECT_FALSE(ranges::any_of(none_even, even));
}

TEST(AnyOfTest, InitializerList) {
    using ILI = std::initializer_list<int>;
    EXPECT_TRUE(ranges::any_of(ILI{0, 2, 4, 6}, [](int n) { return n % 2 == 0; }));
    EXPECT_TRUE(ranges::any_of(ILI{1, 3, 4, 7}, [](int n) { return n % 2 == 0; }));
    EXPECT_FALSE(ranges::any_of(ILI{1, 3, 5, 7}, [](int n) { return n % 2 == 0; }));
}

TEST(AnyOfTest, StructSVectorIterators) {
    std::vector<S> all_true{S(true), S(true), S(true)};
    std::vector<S> one_true{S(false), S(false), S(true)};
    std::vector<S> none_true{S(false), S(false), S(false)};

    EXPECT_TRUE(ranges::any_of(all_true.begin(), all_true.end(), &S::p));
    EXPECT_TRUE(ranges::any_of(one_true.begin(), one_true.end(), &S::p));
    EXPECT_FALSE(ranges::any_of(none_true.begin(), none_true.end(), &S::p));
}

TEST(AnyOfTest, StructSVectorRange) {
    std::vector<S> all_true{S(true), S(true), S(true)};
    std::vector<S> one_true{S(false), S(false), S(true)};
    std::vector<S> none_true{S(false), S(false), S(false)};

    EXPECT_TRUE(ranges::any_of(all_true, &S::p));
    EXPECT_TRUE(ranges::any_of(one_true, &S::p));
    EXPECT_FALSE(ranges::any_of(none_true, &S::p));
}

TEST(AnyOfTest, StructSInitializerList) {
    using ILS = std::initializer_list<S>;
    EXPECT_TRUE(ranges::any_of(ILS{S(true), S(true), S(true)}, &S::p));
    EXPECT_TRUE(ranges::any_of(ILS{S(false), S(true), S(false)}, &S::p));
    EXPECT_FALSE(ranges::any_of(ILS{S(false), S(false), S(false)}, &S::p));
}

TEST(AnyOfTest, Constexpr) {
    constexpr auto test = []() constexpr {
        auto check = [](std::initializer_list<int> il) constexpr {
            return ranges::any_of(il, even);
        };
        bool ok = check({0, 2, 4, 6}) && check({1, 3, 4, 7}) && !check({1, 3, 5, 7});
        return ok;
    };
    static_assert(test(), "constexpr any_of test failed");
}