/// adjacent_remove_if_gtest.cpp
/// Google Test conversion of range-v3 adjacent_remove_if test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <vector>
#include <iterator>
#include <functional>

#include <fermat/range/access.h>
#include <fermat/iterator/operations.h>
#include <fermat/iterator/insert_iterators.h>
#include <fermat/algorithm/copy.h>
#include <fermat/utility/copy.h>
#include <fermat/view/adjacent_remove_if.h>
#include <fermat/view/iota.h>
#include <fermat/view/counted.h>

/// Minimal forward iterator for testing purposes.
template<typename T>
class ForwardIterator {
    T* ptr_;
public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept   = std::forward_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    ForwardIterator() = default;
    explicit ForwardIterator(T* p) : ptr_(p) {}

    reference operator*() const { return *ptr_; }
    pointer   operator->() const { return ptr_; }

    ForwardIterator& operator++() { ++ptr_; return *this; }
    ForwardIterator operator++(int) { auto tmp = *this; ++ptr_; return tmp; }

    friend bool operator==(const ForwardIterator& a, const ForwardIterator& b) { return a.ptr_ == b.ptr_; }
    friend bool operator!=(const ForwardIterator& a, const ForwardIterator& b) { return !(a == b); }

    T* base() const { return ptr_; }
};

/// Helper to compare a range with an initializer list.
template<typename Rng, typename T>
void check_equal(Rng&& rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const& val : expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

/// Helper to check that a type is exactly T (for has_type macro replacement).
template<typename T, typename U>
void has_type(U&&) {
    static_assert(std::is_same<T, U>::value, "");
}

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(AdjacentRemoveIfTest, RawArrayWithEqualTo) {
    using namespace fermat::ranges;

    int const rgi[] = {1, 1, 1, 2, 3, 4, 4};
    std::vector<int> out;

    auto rng = rgi | views::adjacent_remove_if(std::equal_to<int>{});
    has_type<int const &>(*begin(rng));
    static_assert(view_<decltype(rng)>, "");
    static_assert(common_range<decltype(rng)>, "");
    static_assert(!sized_range<decltype(rng)>, "");
    static_assert(bidirectional_iterator<decltype(begin(rng))>, "");
    static_assert(!random_access_iterator<decltype(begin(rng))>, "");
    static_assert(output_iterator<decltype(fermat::ranges::back_inserter(out)), int>, "");
    static_assert(!equality_comparable<decltype(fermat::ranges::back_inserter(out))>, "");

    copy(rng, fermat::ranges::back_inserter(out));
    check_equal(out, {1, 2, 3, 4});
}

TEST(AdjacentRemoveIfTest, CountedViewWithLambda) {
    using namespace fermat::ranges;

    int const rgi[] = {1, 1, 1, 2, 3, 4, 4};
    auto rng2 = views::counted(rgi, 7)
                | views::adjacent_remove_if([](int i, int j) { return i == j; });
    has_type<int const &>(*begin(rng2));
    static_assert(view_<decltype(rng2)>, "");
    static_assert(forward_range<decltype(rng2)>, "");
    static_assert(common_range<decltype(rng2)>, "");
    static_assert(!sized_range<decltype(rng2)>, "");
    static_assert(bidirectional_iterator<decltype(begin(rng2))>, "");
    static_assert(!random_access_iterator<decltype(begin(rng2))>, "");
    check_equal(rng2, {1, 2, 3, 4});
}

TEST(AdjacentRemoveIfTest, ForwardIteratorWithEqualTo) {
    using namespace fermat::ranges;

    int const rgi[] = {1, 1, 1, 2, 3, 4, 4};
    auto rng3 = views::counted(ForwardIterator<int const>(rgi), 7)
                | views::adjacent_remove_if(std::equal_to<int>{});
    has_type<int const &>(*begin(rng3));
    static_assert(view_<decltype(rng3)>, "");
    static_assert(forward_range<decltype(rng3)>, "");
    static_assert(!common_range<decltype(rng3)>, "");
    static_assert(!sized_range<decltype(rng3)>, "");
    static_assert(forward_iterator<decltype(begin(rng3))>, "");
    static_assert(!bidirectional_iterator<decltype(begin(rng3))>, "");
    check_equal(rng3, {1, 2, 3, 4});
}

TEST(AdjacentRemoveIfTest, ForwardIteratorAlwaysTrue) {
    using namespace fermat::ranges;

    int const rgi[] = {1, 1, 1, 2, 3, 4, 4};
    auto rng4 = views::counted(ForwardIterator<int const>(rgi), 7)
                | views::adjacent_remove_if([](int, int) { return true; });
    has_type<int const &>(*begin(rng4));
    EXPECT_EQ(*begin(rng4), 4);
    static_assert(view_<decltype(rng4)>, "");
    static_assert(forward_range<decltype(rng4)>, "");
    static_assert(!common_range<decltype(rng4)>, "");
    static_assert(!sized_range<decltype(rng4)>, "");
    static_assert(forward_iterator<decltype(begin(rng4))>, "");
    static_assert(!bidirectional_iterator<decltype(begin(rng4))>, "");
    check_equal(rng4, {4});
}

TEST(AdjacentRemoveIfTest, IotaWithCustomPredicate) {
    using namespace fermat::ranges;

    auto is_odd_then_even = [](int i, int j) { return 1 == i%2 && 0 == j%2; };
    auto rng5 = views::iota(0, 11) | views::adjacent_remove_if(is_odd_then_even);
    has_type<int>(*begin(rng5));
    static_assert(view_<decltype(rng5)>, "");
    static_assert(forward_range<decltype(rng5)>, "");
    static_assert(common_range<decltype(rng5)>, "");
    static_assert(!sized_range<decltype(rng5)>, "");
    static_assert(bidirectional_iterator<decltype(begin(rng5))>, "");
    static_assert(!random_access_iterator<decltype(begin(rng5))>, "");
    check_equal(rng5, {0, 2, 4, 6, 8, 10});
}

TEST(AdjacentRemoveIfTest, BidirectionalTraversalConsistency) {
    using namespace fermat::ranges;

    int const rgi[] = {1, 1, 1, 2, 3, 4, 4};
    auto rng = views::adjacent_remove_if(rgi, std::equal_to<int>{});
    std::vector<int const*> pointers;
    for (auto& i : rng)
        pointers.push_back(&i);

    auto pos = fermat::ranges::end(rng);
    for (auto i = pointers.size(); i != 0; ) {
        EXPECT_NE(pos, fermat::ranges::begin(rng));
        EXPECT_EQ(&*--pos, pointers[--i]);
    }
    EXPECT_EQ(pos, fermat::ranges::begin(rng));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
