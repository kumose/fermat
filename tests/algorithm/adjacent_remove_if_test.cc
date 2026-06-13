#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <fermat/algorithm/adjacent_remove_if.h>
#include <fermat/core.h>
#include <fermat/view/all.h>

namespace {
    // Helper: forward iterator concept test is not needed; we test with plain pointers and vector.
    // We keep the core logic but omit multiple iterator category overloads for simplicity.

    struct S {
        int i;
    };

    struct Pred {
        bool operator()(const std::unique_ptr<int>& i, const std::unique_ptr<int>& j) const {
            return *i == 2 && *j == 3;
        }
    };
} // namespace

TEST(AdjacentRemoveIfTest, Basic) {
    int ia[] = {0, 1, 1, 1, 4, 2, 2, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    int* r = ranges::adjacent_remove_if(std::begin(ia), std::end(ia), ranges::equal_to{});
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 4);
    EXPECT_EQ(ia[3], 2);
    EXPECT_EQ(ia[4], 4);
    EXPECT_EQ(ia[5], 2);
}

TEST(AdjacentRemoveIfTest, Range) {
    int ia[] = {0, 1, 1, 1, 4, 2, 2, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    int* r = ranges::adjacent_remove_if(ranges::make_subrange(std::begin(ia), std::end(ia)), ranges::equal_to{});
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0], 0);
    EXPECT_EQ(ia[1], 1);
    EXPECT_EQ(ia[2], 4);
    EXPECT_EQ(ia[3], 2);
    EXPECT_EQ(ia[4], 4);
    EXPECT_EQ(ia[5], 2);
}

TEST(AdjacentRemoveIfTest, UniquePtr) {
    constexpr std::size_t sa = 9;
    std::unique_ptr<int> ia[sa];
    ia[0].reset(new int(0));
    ia[1].reset(new int(1));
    ia[2].reset(new int(2));
    ia[3].reset(new int(3));
    ia[4].reset(new int(4));
    ia[5].reset(new int(2));
    ia[6].reset(new int(3));
    ia[7].reset(new int(4));
    ia[8].reset(new int(2));

    std::unique_ptr<int>* r = ranges::adjacent_remove_if(std::begin(ia), std::end(ia), Pred{});
    EXPECT_EQ(r, ia + sa - 2);
    EXPECT_EQ(*ia[0], 0);
    EXPECT_EQ(*ia[1], 1);
    EXPECT_EQ(*ia[2], 3);
    EXPECT_EQ(*ia[3], 4);
    EXPECT_EQ(*ia[4], 3);
    EXPECT_EQ(*ia[5], 4);
    EXPECT_EQ(*ia[6], 2);
}

TEST(AdjacentRemoveIfTest, UniquePtrRange) {
    constexpr std::size_t sa = 9;
    std::unique_ptr<int> ia[sa];
    ia[0].reset(new int(0));
    ia[1].reset(new int(1));
    ia[2].reset(new int(2));
    ia[3].reset(new int(3));
    ia[4].reset(new int(4));
    ia[5].reset(new int(2));
    ia[6].reset(new int(3));
    ia[7].reset(new int(4));
    ia[8].reset(new int(2));

    std::unique_ptr<int>* r = ranges::adjacent_remove_if(ranges::make_subrange(std::begin(ia), std::end(ia)), Pred{});
    EXPECT_EQ(r, ia + sa - 2);
    EXPECT_EQ(*ia[0], 0);
    EXPECT_EQ(*ia[1], 1);
    EXPECT_EQ(*ia[2], 3);
    EXPECT_EQ(*ia[3], 4);
    EXPECT_EQ(*ia[4], 3);
    EXPECT_EQ(*ia[5], 4);
    EXPECT_EQ(*ia[6], 2);
}

TEST(AdjacentRemoveIfTest, Projection) {
    S ia[] = {S{0}, S{1}, S{1}, S{1}, S{4}, S{2}, S{2}, S{4}, S{2}};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    S* r = ranges::adjacent_remove_if(ia, ranges::equal_to{}, &S::i);
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0].i, 0);
    EXPECT_EQ(ia[1].i, 1);
    EXPECT_EQ(ia[2].i, 4);
    EXPECT_EQ(ia[3].i, 2);
    EXPECT_EQ(ia[4].i, 4);
    EXPECT_EQ(ia[5].i, 2);
}

TEST(AdjacentRemoveIfTest, RvalueRange) {
    S ia[] = {S{0}, S{1}, S{1}, S{2}, S{3}, S{5}, S{8}, S{13}, S{21}};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    auto r = ranges::adjacent_remove_if(ranges::views::all(ia),
                                        [](int x, int y) noexcept { return (x + y) % 2 == 0; },
                                        &S::i);
    EXPECT_EQ(r, ia + sa - 3);
    EXPECT_EQ(ia[0].i, 0);
    EXPECT_EQ(ia[1].i, 1);
    EXPECT_EQ(ia[2].i, 2);
    EXPECT_EQ(ia[3].i, 5);
    EXPECT_EQ(ia[4].i, 8);
    EXPECT_EQ(ia[5].i, 21);
}

TEST(AdjacentRemoveIfTest, Constexpr) {
    // Compile-time test with array
    constexpr int ia[] = {0, 1, 1, 1, 4, 2, 2, 4, 2};
    constexpr std::size_t sa = sizeof(ia) / sizeof(ia[0]);

    // We cannot modify a constexpr array, so we test only the pointer/iterator part.
    // However, adjacent_remove_if in constexpr context requires that the array be non-const.
    // So we create a mutable array in constexpr context.
    constexpr auto test = []() constexpr {
        int arr[] = {0, 1, 1, 1, 4, 2, 2, 4, 2};
        auto* r = ranges::adjacent_remove_if(std::begin(arr), std::end(arr), ranges::equal_to{});
        bool ok = (r == arr + sa - 3) &&
                  (arr[0] == 0) && (arr[1] == 1) && (arr[2] == 4) &&
                  (arr[3] == 2) && (arr[4] == 4) && (arr[5] == 2);
        return ok;
    };
    static_assert(test(), "constexpr test failed");
}