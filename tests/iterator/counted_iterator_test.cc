// counted_iterator_gtest.cpp
// Google Test conversion of range-v3 counted_iterator test.
// All comments in English.

#include <gtest/gtest.h>

#include <fermat/iterator/counted_iterator.h>
#include <fermat/iterator/default_sentinel.h>

// iterator that models input_iterator (has void-returning postfix increment operator)
struct Iterator {
    using value_type = int;
    using difference_type = int;

    int counter = 0;

    int operator*() const { return counter; }
    Iterator& operator++() { ++counter; return *this; }
    void operator++(int) { ++counter; }
    bool operator==(const Iterator& rhs) const { return counter == rhs.counter; }
    bool operator!=(const Iterator& rhs) const { return !(*this == rhs); }
};

static_assert(fermat::ranges::input_iterator<Iterator>, "Iterator must model input_iterator");

TEST(CountedIteratorTest, Basic) {
    auto cnt = fermat::ranges::counted_iterator<Iterator>(Iterator(), 1);
    EXPECT_EQ(*cnt, 0);
    cnt++;
    EXPECT_EQ(cnt, fermat::ranges::default_sentinel);
}
