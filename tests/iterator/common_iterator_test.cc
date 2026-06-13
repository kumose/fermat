// common_iterator_gtest.cpp
// Google Test conversion of range-v3 common_iterator test.
// All comments in English.

#include <gtest/gtest.h>
#include <algorithm>
#include <numeric>
#include <iterator>

#include <fermat/iterator/common_iterator.h>
#include <fermat/iterator/unreachable_sentinel.h>

// ------------------------------------------------------------
// Minimal forward iterator (only ++, ==, !=, *)
// ------------------------------------------------------------
template<typename T>
class ForwardPtr {
    T *ptr_;

public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    ForwardPtr() = default;

    explicit ForwardPtr(T *p) : ptr_(p) {
    }

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    ForwardPtr &operator++() {
        ++ptr_;
        return *this;
    }

    ForwardPtr operator++(int) {
        auto tmp = *this;
        ++ptr_;
        return tmp;
    }

    friend bool operator==(const ForwardPtr &a, const ForwardPtr &b) { return a.ptr_ == b.ptr_; }
    friend bool operator!=(const ForwardPtr &a, const ForwardPtr &b) { return !(a == b); }

    T *base() const { return ptr_; }
};

// ------------------------------------------------------------
// Minimal bidirectional iterator (from reverse_iterator_test)
// ------------------------------------------------------------
template<typename T>
class BidiPtr {
    T *ptr_;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    BidiPtr() = default;

    explicit BidiPtr(T *p) : ptr_(p) {
    }

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    BidiPtr &operator++() {
        ++ptr_;
        return *this;
    }

    BidiPtr operator++(int) {
        auto tmp = *this;
        ++ptr_;
        return tmp;
    }

    BidiPtr &operator--() {
        --ptr_;
        return *this;
    }

    BidiPtr operator--(int) {
        auto tmp = *this;
        --ptr_;
        return tmp;
    }

    friend bool operator==(const BidiPtr &a, const BidiPtr &b) { return a.ptr_ == b.ptr_; }
    friend bool operator!=(const BidiPtr &a, const BidiPtr &b) { return !(a == b); }

    T *base() const { return ptr_; }
};

// ------------------------------------------------------------
// Minimal random access iterator (from reverse_iterator_test)
// ------------------------------------------------------------
template<typename T>
class RAPtr {
    T *ptr_;

public:
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T &;

    RAPtr() = default;

    explicit RAPtr(T *p) : ptr_(p) {
    }

    reference operator*() const { return *ptr_; }
    pointer operator->() const { return ptr_; }

    RAPtr &operator++() {
        ++ptr_;
        return *this;
    }

    RAPtr operator++(int) {
        auto tmp = *this;
        ++ptr_;
        return tmp;
    }

    RAPtr &operator--() {
        --ptr_;
        return *this;
    }

    RAPtr operator--(int) {
        auto tmp = *this;
        --ptr_;
        return tmp;
    }

    RAPtr &operator+=(difference_type n) {
        ptr_ += n;
        return *this;
    }

    RAPtr &operator-=(difference_type n) {
        ptr_ -= n;
        return *this;
    }

    friend RAPtr operator+(RAPtr it, difference_type n) {
        it += n;
        return it;
    }

    friend RAPtr operator+(difference_type n, RAPtr it) {
        it += n;
        return it;
    }

    friend RAPtr operator-(RAPtr it, difference_type n) {
        it -= n;
        return it;
    }

    friend difference_type operator-(const RAPtr &a, const RAPtr &b) { return a.ptr_ - b.ptr_; }

    reference operator[](difference_type n) const { return *(ptr_ + n); }

    friend bool operator==(const RAPtr &a, const RAPtr &b) { return a.ptr_ == b.ptr_; }
    friend bool operator!=(const RAPtr &a, const RAPtr &b) { return !(a == b); }
    friend bool operator<(const RAPtr &a, const RAPtr &b) { return a.ptr_ < b.ptr_; }
    friend bool operator>(const RAPtr &a, const RAPtr &b) { return b < a; }
    friend bool operator<=(const RAPtr &a, const RAPtr &b) { return !(b < a); }
    friend bool operator>=(const RAPtr &a, const RAPtr &b) { return !(a < b); }

    T *base() const { return ptr_; }
};

// ------------------------------------------------------------
// Sentinel with optional sized sentinel capability
// ------------------------------------------------------------
template<typename It, bool IsSized = false>
class Sentinel {
    typename It::pointer ptr_; // store raw pointer from iterator's base
public:
    Sentinel() = default;

    explicit Sentinel(It it) : ptr_(it.base()) {
    }

    explicit Sentinel(typename It::pointer p) : ptr_(p) {
    }

    // Comparison with any of the three iterator types (using base pointer)
    template<typename Iter>
    friend bool operator==(const Iter &i, const Sentinel &s) {
        return i.base() == s.ptr_;
    }

    template<typename Iter>
    friend bool operator==(const Sentinel &s, const Iter &i) {
        return i == s;
    }

    template<typename Iter>
    friend bool operator!=(const Iter &i, const Sentinel &s) {
        return !(i == s);
    }

    template<typename Iter>
    friend bool operator!=(const Sentinel &s, const Iter &i) {
        return !(i == s);
    }

    // Sized sentinel support (only for random access iterators when IsSized true)
    template<bool B = IsSized>
    friend typename std::enable_if<B, std::ptrdiff_t>::type
    operator-(const RAPtr<typename It::value_type> &i, const Sentinel &s) {
        return i.base() - s.ptr_;
    }

    template<bool B = IsSized>
    friend typename std::enable_if<B, std::ptrdiff_t>::type
    operator-(const Sentinel &s, const RAPtr<typename It::value_type> &i) {
        return s.ptr_ - i.base();
    }

    typename It::pointer base() const { return ptr_; }
};

// ------------------------------------------------------------
// Helper to ignore unused variables
// ------------------------------------------------------------
template<typename... T>
void ignore_unused(T &&...) {
}

// ------------------------------------------------------------
// Test operator-> (identical to original)
// ------------------------------------------------------------
namespace {
    struct silly_arrow_cursor {
        int read() const { return 0; }

        void next() {
        }

        int arrow() const { return 42; }
    };

    int forty_two = 42;

    struct lvalue_iterator {
        using difference_type = int;
        using value_type = int;
        int &operator*() const { return forty_two; }
        lvalue_iterator &operator++() & { return *this; }
        lvalue_iterator operator++(int) & { return *this; }
    };

    struct xvalue_iterator : lvalue_iterator {
        int &&operator*() const { return std::move(forty_two); }
        xvalue_iterator &operator++() & { return *this; }
        xvalue_iterator operator++(int) & { return *this; }
    };

    struct proxy_cursor {
        int read() const { return 42; }

        void next() {
        }
    };

    void test_operator_arrow() {
        // I is a pointer type
        {
            int i = 42;
            auto ci = ranges::common_iterator<int *, ranges::unreachable_sentinel_t>{&i};
            static_assert(ranges::same_as<int *, decltype(ci.operator->())>);
            EXPECT_EQ(ci.operator->(), &i);
        }
        // the expression i.operator->() is well-formed
        {
            using I = ranges::basic_iterator<silly_arrow_cursor>;
            auto ci = ranges::common_iterator<I, ranges::unreachable_sentinel_t>{};
            static_assert(ranges::same_as<I, decltype(ci.operator->())>);
            EXPECT_EQ(ci.operator->().operator->(), 42);
        }
        // the expression *i is a glvalue [lvalue case]
        {
            auto ci = ranges::common_iterator<lvalue_iterator, ranges::unreachable_sentinel_t>{};
            static_assert(ranges::same_as<int *, decltype(ci.operator->())>);
            EXPECT_EQ(ci.operator->(), &forty_two);
        }
        // the expression *i is a glvalue [xvalue case]
        {
            auto ci = ranges::common_iterator<xvalue_iterator, ranges::unreachable_sentinel_t>{};
            static_assert(ranges::same_as<int *, decltype(ci.operator->())>);
            EXPECT_EQ(ci.operator->(), &forty_two);
        }
        // Otherwise, returns a proxy object
        {
            using I = ranges::basic_iterator<proxy_cursor>;
            auto ci = ranges::common_iterator<I, ranges::unreachable_sentinel_t>{};
            using A = decltype(ci.operator->());
            static_assert(std::is_class<A>::value);
            static_assert(!std::is_same<I, A>::value);
            EXPECT_EQ(*ci.operator->().operator->(), 42);
        }
    }
}

// ------------------------------------------------------------
// Google Test cases (all original tests preserved)
// ------------------------------------------------------------
TEST(CommonIteratorTest, Concepts) {
    // forward_iterator concept test: BidiPtr + Sentinel -> forward, not bidirectional
    using CI_forward = ranges::common_iterator<BidiPtr<const char>, Sentinel<BidiPtr<const char> > >;
    static_assert(ranges::forward_iterator<CI_forward>);
    static_assert(!ranges::bidirectional_iterator<CI_forward>);

    // common_reference test
    using CI_ref = ranges::common_iterator<BidiPtr<const char>, Sentinel<BidiPtr<const char> > >;
    static_assert(std::is_same<
        ranges::common_reference<CI_ref &, CI_ref>::type,
        CI_ref
    >::value);

    // sized sentinel tests
    // Case 1: forward iterator + sized sentinel -> not sized (ForwardPtr does not support subtraction)
    using CI_forward_sized = ranges::common_iterator<ForwardPtr<int>, Sentinel<ForwardPtr<int>, true> >;
    static_assert(!ranges::sized_sentinel_for<CI_forward_sized, CI_forward_sized>);

    // Case 2: random access iterator + sized sentinel -> sized
    using CI_random_sized = ranges::common_iterator<RAPtr<int>, Sentinel<RAPtr<int>, true> >;
    static_assert(ranges::sized_sentinel_for<CI_random_sized, CI_random_sized>);

    // Case 3: random access iterator + non-sized sentinel -> not sized
    using CI_random_unsized = ranges::common_iterator<RAPtr<int>, Sentinel<RAPtr<int>, false> >;
    static_assert(!ranges::sized_sentinel_for<CI_random_unsized, CI_random_unsized>);
}

TEST(CommonIteratorTest, Accumulate) {
    int rgi[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    using CI = ranges::common_iterator<RAPtr<int>, Sentinel<RAPtr<int> > >;
    CI first{RAPtr<int>{rgi}};
    CI last{Sentinel<RAPtr<int> >{rgi + 10}};
    int sum = std::accumulate(first, last, 0, std::plus<int>{});
    EXPECT_EQ(sum, 45);
}

TEST(CommonIteratorTest, OperatorArrow) {
    test_operator_arrow();
}
