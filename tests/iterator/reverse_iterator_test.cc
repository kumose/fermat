// reverse_iterator_gtest.cpp
// Google Test conversion of range-v3 reverse_iterator test.
// All comments in English.

#include <gtest/gtest.h>

#include <fermat/range/access.h>
#include <fermat/iterator/reverse_iterator.h>
#include <fermat/view/iota.h>

// ------------------------------------------------------------
// Minimal bidirectional iterator (only ++, --, ==, !=, *)
// ------------------------------------------------------------
template<typename T>
class BidiPtr {
    T* ptr_;
public:
    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept   = std::bidirectional_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    BidiPtr() = default;
    explicit BidiPtr(T* p) : ptr_(p) {}

    // Conversion constructor: allow BidiPtr<U> -> BidiPtr<T> if U* convertible to T*
    template<typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    BidiPtr(const BidiPtr<U>& other) : ptr_(other.base()) {}

    reference operator*() const { return *ptr_; }
    pointer   operator->() const { return ptr_; }

    BidiPtr& operator++() { ++ptr_; return *this; }
    BidiPtr operator++(int) { auto tmp = *this; ++ptr_; return tmp; }
    BidiPtr& operator--() { --ptr_; return *this; }
    BidiPtr operator--(int) { auto tmp = *this; --ptr_; return tmp; }

    friend bool operator==(const BidiPtr& a, const BidiPtr& b) { return a.ptr_ == b.ptr_; }
    friend bool operator!=(const BidiPtr& a, const BidiPtr& b) { return !(a == b); }

    T* base() const { return ptr_; }
};

// ------------------------------------------------------------
// Minimal random access iterator (full random access)
// ------------------------------------------------------------
template<typename T>
class RAPtr {
    T* ptr_;
public:
    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept   = std::random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    RAPtr() = default;
    explicit RAPtr(T* p) : ptr_(p) {}

    // Conversion constructor
    template<typename U, typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
    RAPtr(const RAPtr<U>& other) : ptr_(other.base()) {}

    reference operator*() const { return *ptr_; }
    pointer   operator->() const { return ptr_; }

    RAPtr& operator++() { ++ptr_; return *this; }
    RAPtr operator++(int) { auto tmp = *this; ++ptr_; return tmp; }
    RAPtr& operator--() { --ptr_; return *this; }
    RAPtr operator--(int) { auto tmp = *this; --ptr_; return tmp; }

    RAPtr& operator+=(difference_type n) { ptr_ += n; return *this; }
    RAPtr& operator-=(difference_type n) { ptr_ -= n; return *this; }

    friend RAPtr operator+(RAPtr it, difference_type n) { it += n; return it; }
    friend RAPtr operator+(difference_type n, RAPtr it) { it += n; return it; }
    friend RAPtr operator-(RAPtr it, difference_type n) { it -= n; return it; }
    friend difference_type operator-(const RAPtr& a, const RAPtr& b) { return a.ptr_ - b.ptr_; }

    reference operator[](difference_type n) const { return *(ptr_ + n); }

    friend bool operator==(const RAPtr& a, const RAPtr& b) { return a.ptr_ == b.ptr_; }
    friend bool operator!=(const RAPtr& a, const RAPtr& b) { return !(a == b); }
    friend bool operator<(const RAPtr& a, const RAPtr& b) { return a.ptr_ < b.ptr_; }
    friend bool operator>(const RAPtr& a, const RAPtr& b) { return b < a; }
    friend bool operator<=(const RAPtr& a, const RAPtr& b) { return !(b < a); }
    friend bool operator>=(const RAPtr& a, const RAPtr& b) { return !(a < b); }

    T* base() const { return ptr_; }
};

// ------------------------------------------------------------
// Test helper functions (identical to original)
// ------------------------------------------------------------
template<class It> void test() { ranges::reverse_iterator<It>{}; }
template<class It> void test2(It i) {
    ranges::reverse_iterator<It> r(i);
    EXPECT_EQ(r.base(), i);
}
template<class It, class U> void test3(U u) {
    const ranges::reverse_iterator<U> r2(u);
    ranges::reverse_iterator<It> r1 = ranges::reverse_iterator<It>(r2);
    EXPECT_EQ(r1.base(), u);
}
struct Base {};
struct Derived : Base {};
template<class It> void test4(It i) {
    const ranges::reverse_iterator<It> r = ranges::make_reverse_iterator(i);
    EXPECT_EQ(r.base(), i);
}
template<class It> void test5(It l, It r, bool x) {
    const ranges::reverse_iterator<It> r1(l);
    const ranges::reverse_iterator<It> r2(r);
    EXPECT_EQ((r1 != r2), x);
}
template<class It> void test6(It i, It x) {
    ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It> rr = r++;
    EXPECT_EQ(r.base(), x);
    EXPECT_EQ(rr.base(), i);
}
template<class It> void test7(It i, It x) {
    ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It>& rr = ++r;
    EXPECT_EQ(r.base(), x);
    EXPECT_EQ(&rr, &r);
}
template<class It> void test8(It i, ranges::iter_difference_t<It> n, It x) {
    const ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It> rr = r + n;
    EXPECT_EQ(rr.base(), x);
}
template<class It> void test9(It i, ranges::iter_difference_t<It> n, It x) {
    ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It>& rr = r += n;
    EXPECT_EQ(r.base(), x);
    EXPECT_EQ(&rr, &r);
}
template<class It> void test10(It i, It x) {
    ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It> rr = r--;
    EXPECT_EQ(r.base(), x);
    EXPECT_EQ(rr.base(), i);
}
template<class It> void test11(It i, It x) {
    ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It>& rr = --r;
    EXPECT_EQ(r.base(), x);
    EXPECT_EQ(&rr, &r);
}
template<class It> void test12(It i, ranges::iter_difference_t<It> n, It x) {
    const ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It> rr = r - n;
    EXPECT_EQ(rr.base(), x);
}
template<class It> void test13(It i, ranges::iter_difference_t<It> n, It x) {
    ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It>& rr = r -= n;
    EXPECT_EQ(r.base(), x);
    EXPECT_EQ(&rr, &r);
}
class A {
    int data_ = 1;
public:
    friend bool operator==(const A& x, const A& y) { return x.data_ == y.data_; }
};
template<class It> void test14(It i, ranges::iter_value_t<It> x) {
    ranges::reverse_iterator<It> r(i);
    EXPECT_EQ(*r, x);
}
template<class It, class U> void test15(U u) {
    const ranges::reverse_iterator<U> r2(u);
    ranges::reverse_iterator<It> r1;
    ranges::reverse_iterator<It>& rr = r1 = r2;
    EXPECT_EQ(r1.base(), u);
    EXPECT_EQ(&rr, &r1);
}
template<class It> void test16(It l, It r, bool x) {
    const ranges::reverse_iterator<It> r1(l);
    const ranges::reverse_iterator<It> r2(r);
    EXPECT_EQ((r1 == r2), x);
}
template<class It1, class It2> void test17(It1 l, It2 r, std::ptrdiff_t x) {
    const ranges::reverse_iterator<It1> r1(l);
    const ranges::reverse_iterator<It2> r2(r);
    EXPECT_EQ((r1 - r2), x);
}
template<class It> void test18(It l, It r, bool x) {
    const ranges::reverse_iterator<It> r1(l);
    const ranges::reverse_iterator<It> r2(r);
    EXPECT_EQ((r1 > r2), x);
}
template<class It> void test19(It l, It r, bool x) {
    const ranges::reverse_iterator<It> r1(l);
    const ranges::reverse_iterator<It> r2(r);
    EXPECT_EQ((r1 >= r2), x);
}
template<class It> void test20(It i, ranges::iter_difference_t<It> n, ranges::iter_value_t<It> x) {
    const ranges::reverse_iterator<It> r(i);
    ranges::iter_value_t<It> rr = r[n];
    EXPECT_EQ(rr, x);
}
template<class It> void test21(It l, It r, bool x) {
    const ranges::reverse_iterator<It> r1(l);
    const ranges::reverse_iterator<It> r2(r);
    EXPECT_EQ((r1 < r2), x);
}
template<class It> void test22(It l, It r, bool x) {
    const ranges::reverse_iterator<It> r1(l);
    const ranges::reverse_iterator<It> r2(r);
    EXPECT_EQ((r1 < r2), x);
}
template<class It> void test23(It l, It r, bool x) {
    const ranges::reverse_iterator<It> r1(l);
    const ranges::reverse_iterator<It> r2(r);
    EXPECT_EQ((r1 <= r2), x);
}
class B {
    int data_ = 1;
public:
    int get() const { return data_; }
    friend bool operator==(const B& x, const B& y) { return x.data_ == y.data_; }
};
template<class It> void test24(It i, ranges::iter_value_t<It> x) {
    ranges::reverse_iterator<It> r(i);
    EXPECT_EQ((*r).get(), x.get());
}
class C {
    int data_;
public:
    C(int d = 1) : data_(d) {}
    int get() const { return data_; }
    friend bool operator==(const C& x, const C& y) { return x.data_ == y.data_; }
    const C* operator&() const { return nullptr; }
    C* operator&() { return nullptr; }
};
template<class It> void test25(It i, ranges::iter_difference_t<It> n, It x) {
    const ranges::reverse_iterator<It> r(i);
    ranges::reverse_iterator<It> rr = n + r;
    EXPECT_EQ(rr.base(), x);
}

// ------------------------------------------------------------
// Google Test cases – using BidiPtr<const char> and RAPtr<const char> etc.
// ------------------------------------------------------------
TEST(ReverseIteratorTest, Concepts) {
    static_assert(ranges::bidirectional_iterator<
                  ranges::reverse_iterator<BidiPtr<char>>>);
    static_assert(ranges::random_access_iterator<
                  ranges::reverse_iterator<RAPtr<char>>>);
}

TEST(ReverseIteratorTest, Test1) {
    test<BidiPtr<char>>();
    test<RAPtr<char>>();
    test<char*>();
    test<const char*>();
}

TEST(ReverseIteratorTest, Test2) {
    const char s[] = "123";
    test2(BidiPtr<const char>(s));
    test2(RAPtr<const char>(s));
    test2(s);
}

TEST(ReverseIteratorTest, Test3) {
    Derived d;
    test3<BidiPtr<Base>>(BidiPtr<Derived>(&d));
}

TEST(ReverseIteratorTest, Test4) {
    const char* s = "1234567890";
    RAPtr<const char> b(s);
    RAPtr<const char> e(s + 10);
    while (b != e) test4(b++);
}

TEST(ReverseIteratorTest, Test5) {
    const char* s = "1234567890";
    test5(BidiPtr<const char>(s), BidiPtr<const char>(s), false);
    test5(BidiPtr<const char>(s), BidiPtr<const char>(s+1), true);
    test5(RAPtr<const char>(s), RAPtr<const char>(s), false);
    test5(RAPtr<const char>(s), RAPtr<const char>(s+1), true);
    test5(s, s, false);
    test5(s, s+1, true);
}

TEST(ReverseIteratorTest, Test6) {
    const char* s = "123";
    test6(BidiPtr<const char>(s+1), BidiPtr<const char>(s));
    test6(RAPtr<const char>(s+1), RAPtr<const char>(s));
    test6(s+1, s);
}

TEST(ReverseIteratorTest, Test7) {
    const char* s = "123";
    test7(BidiPtr<const char>(s+1), BidiPtr<const char>(s));
    test7(RAPtr<const char>(s+1), RAPtr<const char>(s));
    test7(s+1, s);
}

TEST(ReverseIteratorTest, Test8) {
    const char* s = "1234567890";
    test8(RAPtr<const char>(s+5), 5, RAPtr<const char>(s));
    test8(s+5, 5, s);
}

TEST(ReverseIteratorTest, Test9) {
    const char* s = "1234567890";
    test9(RAPtr<const char>(s+5), 5, RAPtr<const char>(s));
    test9(s+5, 5, s);
}

TEST(ReverseIteratorTest, Test10) {
    const char* s = "123";
    test10(BidiPtr<const char>(s+1), BidiPtr<const char>(s+2));
    test10(RAPtr<const char>(s+1), RAPtr<const char>(s+2));
    test10(s+1, s+2);
}

TEST(ReverseIteratorTest, Test11) {
    const char* s = "123";
    test11(BidiPtr<const char>(s+1), BidiPtr<const char>(s+2));
    test11(RAPtr<const char>(s+1), RAPtr<const char>(s+2));
    test11(s+1, s+2);
}

TEST(ReverseIteratorTest, Test12) {
    const char* s = "1234567890";
    test12(RAPtr<const char>(s+5), 5, RAPtr<const char>(s+10));
    test12(s+5, 5, s+10);
}

TEST(ReverseIteratorTest, Test13) {
    const char* s = "1234567890";
    test13(RAPtr<const char>(s+5), 5, RAPtr<const char>(s+10));
    test13(s+5, 5, s+10);
}

TEST(ReverseIteratorTest, Test14) {
    A a;
    test14(&a + 1, A());
}

TEST(ReverseIteratorTest, Test15) {
    Derived d;
    test15<BidiPtr<Base>>(BidiPtr<Derived>(&d));
    test15<RAPtr<const Base>>(RAPtr<Derived>(&d));
    test15<Base*>(&d);
}

TEST(ReverseIteratorTest, Test16) {
    const char* s = "1234567890";
    test16(BidiPtr<const char>(s), BidiPtr<const char>(s), true);
    test16(BidiPtr<const char>(s), BidiPtr<const char>(s+1), false);
    test16(RAPtr<const char>(s), RAPtr<const char>(s), true);
    test16(RAPtr<const char>(s), RAPtr<const char>(s+1), false);
    test16(s, s, true);
    test16(s, s+1, false);
}

TEST(ReverseIteratorTest, Test17) {
    char s[3] = {0};
    test17(RAPtr<char>(s), RAPtr<char>(s), 0);
    test17(RAPtr<char>(s), RAPtr<char>(s+1), 1);
    test17(RAPtr<char>(s+1), RAPtr<char>(s), -1);
    test17(s, s, 0);
    test17(s, s+1, 1);
    test17(s+1, s, -1);
}

TEST(ReverseIteratorTest, Test18) {
    const char* s = "1234567890";
    test18(RAPtr<const char>(s), RAPtr<const char>(s), false);
    test18(RAPtr<const char>(s), RAPtr<const char>(s+1), true);
    test18(RAPtr<const char>(s+1), RAPtr<const char>(s), false);
    test18(s, s, false);
    test18(s, s+1, true);
    test18(s+1, s, false);
}

TEST(ReverseIteratorTest, Test19) {
    const char* s = "1234567890";
    test19(RAPtr<const char>(s), RAPtr<const char>(s), true);
    test19(RAPtr<const char>(s), RAPtr<const char>(s+1), true);
    test19(RAPtr<const char>(s+1), RAPtr<const char>(s), false);
    test19(s, s, true);
    test19(s, s+1, true);
    test19(s+1, s, false);
}

TEST(ReverseIteratorTest, Test20) {
    const char* s = "1234567890";
    test20(RAPtr<const char>(s+5), 4, '1');
    test20(s+5, 4, '1');
}

TEST(ReverseIteratorTest, Test21) {
    const char* s = "1234567890";
    test21(RAPtr<const char>(s), RAPtr<const char>(s), false);
    test21(RAPtr<const char>(s), RAPtr<const char>(s+1), false);
    test21(RAPtr<const char>(s+1), RAPtr<const char>(s), true);
    test21(s, s, false);
    test21(s, s+1, false);
    test21(s+1, s, true);
}

TEST(ReverseIteratorTest, Test22) {
    const char* s = "1234567890";
    test22(RAPtr<const char>(s), RAPtr<const char>(s), false);
    test22(RAPtr<const char>(s), RAPtr<const char>(s+1), false);
    test22(RAPtr<const char>(s+1), RAPtr<const char>(s), true);
    test22(s, s, false);
    test22(s, s+1, false);
    test22(s+1, s, true);
}

TEST(ReverseIteratorTest, Test23) {
    const char* s = "1234567890";
    test23(RAPtr<const char>(s), RAPtr<const char>(s), true);
    test23(RAPtr<const char>(s), RAPtr<const char>(s+1), false);
    test23(RAPtr<const char>(s+1), RAPtr<const char>(s), true);
    test23(s, s, true);
    test23(s, s+1, false);
    test23(s+1, s, true);
}

TEST(ReverseIteratorTest, Test24) {
    B a;
    test24(&a + 1, B());
}

TEST(ReverseIteratorTest, Test25) {
    C l[3] = {C(0), C(1), C(2)};
    auto ri = ranges::rbegin(l);
    EXPECT_EQ(ri->get(), 2);
    EXPECT_EQ((*ri).get(), 2);
    EXPECT_EQ(ri.operator->(), ranges::prev(ri.base()));
    ++ri;
    EXPECT_EQ(ri->get(), 1);
    EXPECT_EQ((*ri).get(), 1);
    EXPECT_EQ(ri.operator->(), ranges::prev(ri.base()));
    ++ri;
    EXPECT_EQ(ri->get(), 0);
    EXPECT_EQ((*ri).get(), 0);
    EXPECT_EQ(ri.operator->(), ranges::prev(ri.base()));
    ++ri;
    EXPECT_EQ(ri, ranges::rend(l));
}

TEST(ReverseIteratorTest, Test26) {
    const char* s = "1234567890";
    test25(RAPtr<const char>(s+5), 5, RAPtr<const char>(s));
    test25(s+5, 5, s);
}
