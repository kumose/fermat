/// Range v3 set_union algorithm test converted to Google Test
/// Original copyright Eric Niebler and LLVM, see source for details.
/// This single file supports all iterator combinations via macros:
/// compile with -DSET_UNION_1, -DSET_UNION_2, ..., -DSET_UNION_6

#include <gtest/gtest.h>
#include <algorithm>
#include <functional>
#include <vector>
#include <initializer_list>

#include <range/v3/algorithm/set_algorithm.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/algorithm/lexicographical_compare.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/counted.hpp>

/// -------------------------------------------------------------------------
/// Minimal test iterators (replaces ../test_iterators.hpp)
/// -------------------------------------------------------------------------

/// InputIterator adaptor
template<typename It>
class InputIterator {
    It it_;
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = typename std::iterator_traits<It>::value_type;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::iterator_traits<It>::pointer;
    using reference = typename std::iterator_traits<It>::reference;

    InputIterator() = default;
    explicit InputIterator(It it) : it_(it) {}
    reference operator*() const { return *it_; }
    pointer operator->() const { return &*it_; }
    InputIterator& operator++() { ++it_; return *this; }
    InputIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    friend bool operator==(const InputIterator& a, const InputIterator& b) { return a.it_ == b.it_; }
    friend bool operator!=(const InputIterator& a, const InputIterator& b) { return !(a == b); }
    It base() const { return it_; }
};

/// ForwardIterator adaptor
template<typename It>
class ForwardIterator : public InputIterator<It> {
public:
    using iterator_category = std::forward_iterator_tag;
    ForwardIterator() = default;
    explicit ForwardIterator(It it) : InputIterator<It>(it) {}
    ForwardIterator& operator++() { InputIterator<It>::operator++(); return *this; }
    ForwardIterator operator++(int) { auto tmp = *this; ++*this; return tmp; }
};

/// BidirectionalIterator adaptor
template<typename It>
class BidirectionalIterator : public ForwardIterator<It> {
public:
    using iterator_category = std::bidirectional_iterator_tag;
    BidirectionalIterator() = default;
    explicit BidirectionalIterator(It it) : ForwardIterator<It>(it) {}
    BidirectionalIterator& operator--() {
        auto it = this->base();
        --it;
        *this = BidirectionalIterator(it);
        return *this;
    }
    BidirectionalIterator operator--(int) { auto tmp = *this; --*this; return tmp; }
};

/// RandomAccessIterator adaptor
template<typename It>
class RandomAccessIterator : public BidirectionalIterator<It> {
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    RandomAccessIterator() = default;
    explicit RandomAccessIterator(It it) : BidirectionalIterator<It>(it) {}
    RandomAccessIterator& operator+=(difference_type n) {
        auto it = this->base();
        it += n;
        *this = RandomAccessIterator(it);
        return *this;
    }
    RandomAccessIterator& operator-=(difference_type n) { return *this += -n; }
    friend RandomAccessIterator operator+(RandomAccessIterator it, difference_type n) { it += n; return it; }
    friend RandomAccessIterator operator+(difference_type n, RandomAccessIterator it) { return it + n; }
    friend RandomAccessIterator operator-(RandomAccessIterator it, difference_type n) { it -= n; return it; }
    friend difference_type operator-(const RandomAccessIterator& a, const RandomAccessIterator& b) {
        return a.base() - b.base();
    }
    reference operator[](difference_type n) const { return *(this->base() + n); }
    friend bool operator<(const RandomAccessIterator& a, const RandomAccessIterator& b) {
        return a.base() < b.base();
    }
    friend bool operator>(const RandomAccessIterator& a, const RandomAccessIterator& b) { return b < a; }
    friend bool operator<=(const RandomAccessIterator& a, const RandomAccessIterator& b) { return !(b < a); }
    friend bool operator>=(const RandomAccessIterator& a, const RandomAccessIterator& b) { return !(a < b); }
};

/// OutputIterator adaptor
template<typename It>
class OutputIterator {
    It it_;
public:
    using iterator_category = std::output_iterator_tag;
    using value_type = void;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = void;
    OutputIterator() = default;
    explicit OutputIterator(It it) : it_(it) {}
    OutputIterator& operator*() { return *this; }
    OutputIterator& operator=(const typename std::iterator_traits<It>::value_type& val) {
        *it_ = val;
        return *this;
    }
    OutputIterator& operator++() { ++it_; return *this; }
    OutputIterator operator++(int) { auto tmp = *this; ++it_; return tmp; }
    It base() const { return it_; }
};

/// Helper to get base pointer (raw pointer extraction)
template<typename Iter>
auto base(Iter i) -> decltype(i.base()) { return i.base(); }
template<typename T>
T* base(T* p) { return p; }

/// -------------------------------------------------------------------------
/// Test data and template test function (identical to original)
/// -------------------------------------------------------------------------

template<class Iter1, class Iter2, class OutIter>
void test() {
    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    static const int sa = sizeof(ia)/sizeof(ia[0]);
    int ib[] = {2, 4, 4, 6};
    static const int sb = sizeof(ib)/sizeof(ib[0]);
    int ic[20];
    int ir[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6};
    static const int sr = sizeof(ir)/sizeof(ir[0]);

    using R = ranges::set_union_result<Iter1, Iter2, OutIter>;

    auto checker = [&](R res) {
        EXPECT_EQ((base(res.out) - ic), sr);
        EXPECT_FALSE(ranges::lexicographical_compare(ic, base(res.out), ir, ir+sr));
        ranges::fill(ic, 0);
    };

    // Default comparison
    checker(ranges::set_union(Iter1(ia), Iter1(ia+sa),
                              Iter2(ib), Iter2(ib+sb),
                              OutIter(ic)));
    checker(ranges::set_union(Iter1(ib), Iter1(ib+sb),
                              Iter2(ia), Iter2(ia+sa),
                              OutIter(ic)));
    // With comparator
    checker(ranges::set_union(Iter1(ia), Iter1(ia+sa),
                              Iter2(ib), Iter2(ib+sb),
                              OutIter(ic), std::less<int>()));
    checker(ranges::set_union(Iter1(ib), Iter1(ib+sb),
                              Iter2(ia), Iter2(ia+sa),
                              OutIter(ic), std::less<int>()));
}

/// -------------------------------------------------------------------------
/// Projection test structures (used in SET_UNION_6)
/// -------------------------------------------------------------------------
struct S { int i; };
struct T { int j; };
struct U {
    int k;
    U& operator=(S s) { k = s.i; return *this; }
    U& operator=(T t) { k = t.j; return *this; }
};

/// -------------------------------------------------------------------------
/// Constexpr test (same as original)
/// -------------------------------------------------------------------------
constexpr bool test_constexpr() {
    using namespace ranges;
    int ia[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    int ib[] = {2, 4, 4, 6};
    int ic[20] = {0};
    int ir[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6};
    const int sr = sizeof(ir) / sizeof(ir[0]);

    auto res1 = set_union(ia, ib, ic, std::less<int>{});
    if (res1.out - ic != sr) return false;
    if (lexicographical_compare(ic, res1.out, ir, ir + sr, std::less<int>{})) return false;
    fill(ic, 0);

    auto res2 = set_union(ib, ia, ic, std::less<int>{});
    if (res2.out - ic != sr) return false;
    if (lexicographical_compare(ic, res2.out, ir, ir + sr, std::less<int>{})) return false;
    return true;
}

/// -------------------------------------------------------------------------
/// Test groups – each group is enabled by a corresponding macro.
/// Compile with -DSET_UNION_1 to enable the first group, etc.
/// -------------------------------------------------------------------------

#ifdef SET_UNION_1
TEST(SetUnion, Input_Input_Output) { test<InputIterator<const int*>, InputIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Input_Input_Forward) { test<InputIterator<const int*>, InputIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Input_Input_Bidirectional) { test<InputIterator<const int*>, InputIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Input_Input_RandomAccess) { test<InputIterator<const int*>, InputIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Input_Input_Pointer) { test<InputIterator<const int*>, InputIterator<const int*>, int*>(); }

TEST(SetUnion, Input_Forward_Output) { test<InputIterator<const int*>, ForwardIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Input_Forward_Forward) { test<InputIterator<const int*>, ForwardIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Input_Forward_Bidirectional) { test<InputIterator<const int*>, ForwardIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Input_Forward_RandomAccess) { test<InputIterator<const int*>, ForwardIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Input_Forward_Pointer) { test<InputIterator<const int*>, ForwardIterator<const int*>, int*>(); }

TEST(SetUnion, Input_Bidirectional_Output) { test<InputIterator<const int*>, BidirectionalIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Input_Bidirectional_Forward) { test<InputIterator<const int*>, BidirectionalIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Input_Bidirectional_Bidirectional) { test<InputIterator<const int*>, BidirectionalIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Input_Bidirectional_RandomAccess) { test<InputIterator<const int*>, BidirectionalIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Input_Bidirectional_Pointer) { test<InputIterator<const int*>, BidirectionalIterator<const int*>, int*>(); }

TEST(SetUnion, Input_RandomAccess_Output) { test<InputIterator<const int*>, RandomAccessIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Input_RandomAccess_Forward) { test<InputIterator<const int*>, RandomAccessIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Input_RandomAccess_Bidirectional) { test<InputIterator<const int*>, RandomAccessIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Input_RandomAccess_RandomAccess) { test<InputIterator<const int*>, RandomAccessIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Input_RandomAccess_Pointer) { test<InputIterator<const int*>, RandomAccessIterator<const int*>, int*>(); }

TEST(SetUnion, Input_Pointer_Output) { test<InputIterator<const int*>, const int*, OutputIterator<int*>>(); }
TEST(SetUnion, Input_Pointer_Forward) { test<InputIterator<const int*>, const int*, ForwardIterator<int*>>(); }
TEST(SetUnion, Input_Pointer_Bidirectional) { test<InputIterator<const int*>, const int*, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Input_Pointer_RandomAccess) { test<InputIterator<const int*>, const int*, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Input_Pointer_Pointer) { test<InputIterator<const int*>, const int*, int*>(); }
#endif

#ifdef SET_UNION_2
TEST(SetUnion, Forward_Input_Output) { test<ForwardIterator<const int*>, InputIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Forward_Input_Forward) { test<ForwardIterator<const int*>, InputIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Forward_Input_Bidirectional) { test<ForwardIterator<const int*>, InputIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Forward_Input_RandomAccess) { test<ForwardIterator<const int*>, InputIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Forward_Input_Pointer) { test<ForwardIterator<const int*>, InputIterator<const int*>, int*>(); }

TEST(SetUnion, Forward_Forward_Output) { test<ForwardIterator<const int*>, ForwardIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Forward_Forward_Forward) { test<ForwardIterator<const int*>, ForwardIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Forward_Forward_Bidirectional) { test<ForwardIterator<const int*>, ForwardIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Forward_Forward_RandomAccess) { test<ForwardIterator<const int*>, ForwardIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Forward_Forward_Pointer) { test<ForwardIterator<const int*>, ForwardIterator<const int*>, int*>(); }

TEST(SetUnion, Forward_Bidirectional_Output) { test<ForwardIterator<const int*>, BidirectionalIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Forward_Bidirectional_Forward) { test<ForwardIterator<const int*>, BidirectionalIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Forward_Bidirectional_Bidirectional) { test<ForwardIterator<const int*>, BidirectionalIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Forward_Bidirectional_RandomAccess) { test<ForwardIterator<const int*>, BidirectionalIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Forward_Bidirectional_Pointer) { test<ForwardIterator<const int*>, BidirectionalIterator<const int*>, int*>(); }

TEST(SetUnion, Forward_RandomAccess_Output) { test<ForwardIterator<const int*>, RandomAccessIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Forward_RandomAccess_Forward) { test<ForwardIterator<const int*>, RandomAccessIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Forward_RandomAccess_Bidirectional) { test<ForwardIterator<const int*>, RandomAccessIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Forward_RandomAccess_RandomAccess) { test<ForwardIterator<const int*>, RandomAccessIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Forward_RandomAccess_Pointer) { test<ForwardIterator<const int*>, RandomAccessIterator<const int*>, int*>(); }

TEST(SetUnion, Forward_Pointer_Output) { test<ForwardIterator<const int*>, const int*, OutputIterator<int*>>(); }
TEST(SetUnion, Forward_Pointer_Forward) { test<ForwardIterator<const int*>, const int*, ForwardIterator<int*>>(); }
TEST(SetUnion, Forward_Pointer_Bidirectional) { test<ForwardIterator<const int*>, const int*, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Forward_Pointer_RandomAccess) { test<ForwardIterator<const int*>, const int*, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Forward_Pointer_Pointer) { test<ForwardIterator<const int*>, const int*, int*>(); }
#endif

#ifdef SET_UNION_3
TEST(SetUnion, Bidirectional_Input_Output) { test<BidirectionalIterator<const int*>, InputIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Input_Forward) { test<BidirectionalIterator<const int*>, InputIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Input_Bidirectional) { test<BidirectionalIterator<const int*>, InputIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Input_RandomAccess) { test<BidirectionalIterator<const int*>, InputIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Input_Pointer) { test<BidirectionalIterator<const int*>, InputIterator<const int*>, int*>(); }

TEST(SetUnion, Bidirectional_Forward_Output) { test<BidirectionalIterator<const int*>, ForwardIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Forward_Forward) { test<BidirectionalIterator<const int*>, ForwardIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Forward_Bidirectional) { test<BidirectionalIterator<const int*>, ForwardIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Forward_RandomAccess) { test<BidirectionalIterator<const int*>, ForwardIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Forward_Pointer) { test<BidirectionalIterator<const int*>, ForwardIterator<const int*>, int*>(); }

TEST(SetUnion, Bidirectional_Bidirectional_Output) { test<BidirectionalIterator<const int*>, BidirectionalIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Bidirectional_Forward) { test<BidirectionalIterator<const int*>, BidirectionalIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Bidirectional_Bidirectional) { test<BidirectionalIterator<const int*>, BidirectionalIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Bidirectional_RandomAccess) { test<BidirectionalIterator<const int*>, BidirectionalIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Bidirectional_Pointer) { test<BidirectionalIterator<const int*>, BidirectionalIterator<const int*>, int*>(); }

TEST(SetUnion, Bidirectional_RandomAccess_Output) { test<BidirectionalIterator<const int*>, RandomAccessIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Bidirectional_RandomAccess_Forward) { test<BidirectionalIterator<const int*>, RandomAccessIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Bidirectional_RandomAccess_Bidirectional) { test<BidirectionalIterator<const int*>, RandomAccessIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Bidirectional_RandomAccess_RandomAccess) { test<BidirectionalIterator<const int*>, RandomAccessIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Bidirectional_RandomAccess_Pointer) { test<BidirectionalIterator<const int*>, RandomAccessIterator<const int*>, int*>(); }

TEST(SetUnion, Bidirectional_Pointer_Output) { test<BidirectionalIterator<const int*>, const int*, OutputIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Pointer_Forward) { test<BidirectionalIterator<const int*>, const int*, ForwardIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Pointer_Bidirectional) { test<BidirectionalIterator<const int*>, const int*, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Pointer_RandomAccess) { test<BidirectionalIterator<const int*>, const int*, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Bidirectional_Pointer_Pointer) { test<BidirectionalIterator<const int*>, const int*, int*>(); }
#endif

#ifdef SET_UNION_4
TEST(SetUnion, RandomAccess_Input_Output) { test<RandomAccessIterator<const int*>, InputIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Input_Forward) { test<RandomAccessIterator<const int*>, InputIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Input_Bidirectional) { test<RandomAccessIterator<const int*>, InputIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Input_RandomAccess) { test<RandomAccessIterator<const int*>, InputIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Input_Pointer) { test<RandomAccessIterator<const int*>, InputIterator<const int*>, int*>(); }

TEST(SetUnion, RandomAccess_Forward_Output) { test<RandomAccessIterator<const int*>, ForwardIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Forward_Forward) { test<RandomAccessIterator<const int*>, ForwardIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Forward_Bidirectional) { test<RandomAccessIterator<const int*>, ForwardIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Forward_RandomAccess) { test<RandomAccessIterator<const int*>, ForwardIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Forward_Pointer) { test<RandomAccessIterator<const int*>, ForwardIterator<const int*>, int*>(); }

TEST(SetUnion, RandomAccess_Bidirectional_Output) { test<RandomAccessIterator<const int*>, BidirectionalIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Bidirectional_Forward) { test<RandomAccessIterator<const int*>, BidirectionalIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Bidirectional_Bidirectional) { test<RandomAccessIterator<const int*>, BidirectionalIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Bidirectional_RandomAccess) { test<RandomAccessIterator<const int*>, BidirectionalIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Bidirectional_Pointer) { test<RandomAccessIterator<const int*>, BidirectionalIterator<const int*>, int*>(); }

TEST(SetUnion, RandomAccess_RandomAccess_Output) { test<RandomAccessIterator<const int*>, RandomAccessIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, RandomAccess_RandomAccess_Forward) { test<RandomAccessIterator<const int*>, RandomAccessIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, RandomAccess_RandomAccess_Bidirectional) { test<RandomAccessIterator<const int*>, RandomAccessIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, RandomAccess_RandomAccess_RandomAccess) { test<RandomAccessIterator<const int*>, RandomAccessIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, RandomAccess_RandomAccess_Pointer) { test<RandomAccessIterator<const int*>, RandomAccessIterator<const int*>, int*>(); }

TEST(SetUnion, RandomAccess_Pointer_Output) { test<RandomAccessIterator<const int*>, const int*, OutputIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Pointer_Forward) { test<RandomAccessIterator<const int*>, const int*, ForwardIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Pointer_Bidirectional) { test<RandomAccessIterator<const int*>, const int*, BidirectionalIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Pointer_RandomAccess) { test<RandomAccessIterator<const int*>, const int*, RandomAccessIterator<int*>>(); }
TEST(SetUnion, RandomAccess_Pointer_Pointer) { test<RandomAccessIterator<const int*>, const int*, int*>(); }
#endif

#ifdef SET_UNION_5
TEST(SetUnion, Pointer_Input_Output) { test<const int*, InputIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Pointer_Input_Forward) { test<const int*, InputIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Pointer_Input_Bidirectional) { test<const int*, InputIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Pointer_Input_RandomAccess) { test<const int*, InputIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Pointer_Input_Pointer) { test<const int*, InputIterator<const int*>, int*>(); }

TEST(SetUnion, Pointer_Forward_Output) { test<const int*, ForwardIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Pointer_Forward_Forward) { test<const int*, ForwardIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Pointer_Forward_Bidirectional) { test<const int*, ForwardIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Pointer_Forward_RandomAccess) { test<const int*, ForwardIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Pointer_Forward_Pointer) { test<const int*, ForwardIterator<const int*>, int*>(); }

TEST(SetUnion, Pointer_Bidirectional_Output) { test<const int*, BidirectionalIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Pointer_Bidirectional_Forward) { test<const int*, BidirectionalIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Pointer_Bidirectional_Bidirectional) { test<const int*, BidirectionalIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Pointer_Bidirectional_RandomAccess) { test<const int*, BidirectionalIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Pointer_Bidirectional_Pointer) { test<const int*, BidirectionalIterator<const int*>, int*>(); }

TEST(SetUnion, Pointer_RandomAccess_Output) { test<const int*, RandomAccessIterator<const int*>, OutputIterator<int*>>(); }
TEST(SetUnion, Pointer_RandomAccess_Forward) { test<const int*, RandomAccessIterator<const int*>, ForwardIterator<int*>>(); }
TEST(SetUnion, Pointer_RandomAccess_Bidirectional) { test<const int*, RandomAccessIterator<const int*>, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Pointer_RandomAccess_RandomAccess) { test<const int*, RandomAccessIterator<const int*>, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Pointer_RandomAccess_Pointer) { test<const int*, RandomAccessIterator<const int*>, int*>(); }

TEST(SetUnion, Pointer_Pointer_Output) { test<const int*, const int*, OutputIterator<int*>>(); }
TEST(SetUnion, Pointer_Pointer_Forward) { test<const int*, const int*, ForwardIterator<int*>>(); }
TEST(SetUnion, Pointer_Pointer_Bidirectional) { test<const int*, const int*, BidirectionalIterator<int*>>(); }
TEST(SetUnion, Pointer_Pointer_RandomAccess) { test<const int*, const int*, RandomAccessIterator<int*>>(); }
TEST(SetUnion, Pointer_Pointer_Pointer) { test<const int*, const int*, int*>(); }
#endif

#ifdef SET_UNION_6
/// Projection tests
TEST(SetUnion, Projection) {
    S ia[] = {S{1}, S{2}, S{2}, S{3}, S{3}, S{3}, S{4}, S{4}, S{4}, S{4}};
    T ib[] = {T{2}, T{4}, T{4}, T{6}};
    U ic[20];
    int ir[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6};
    static const int sr = sizeof(ir)/sizeof(ir[0]);

    using R = ranges::set_union_result<S*, T*, U*>;
    R res = ranges::set_union(ia, ib, ic, std::less<int>(), &S::i, &T::j);
    EXPECT_EQ((res.out - ic), sr);
    EXPECT_FALSE(ranges::lexicographical_compare(ic, res.out, ir, ir+sr, std::less<int>(), &U::k));
    ranges::fill(ic, U{0});

    using R2 = ranges::set_union_result<T*, S*, U*>;
    R2 res2 = ranges::set_union(ib, ia, ic, std::less<int>(), &T::j, &S::i);
    EXPECT_EQ((res2.out - ic), sr);
    EXPECT_FALSE(ranges::lexicographical_compare(ic, res2.out, ir, ir+sr, std::less<int>(), &U::k));
}

/// Rvalue ranges tests (dangling check)
TEST(SetUnion, RvalueRanges) {
    S ia[] = {S{1}, S{2}, S{2}, S{3}, S{3}, S{3}, S{4}, S{4}, S{4}, S{4}};
    T ib[] = {T{2}, T{4}, T{4}, T{6}};
    U ic[20];
    int ir[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 6};
    static const int sr = sizeof(ir)/sizeof(ir[0]);

    auto res1 = ranges::set_union(std::move(ia), ranges::views::all(ib), ic, std::less<int>(), &S::i, &T::j);
    /// res1.in1 is dangling; only check output
    EXPECT_EQ((res1.out - ic), sr);
    EXPECT_FALSE(ranges::lexicographical_compare(ic, res1.out, ir, ir+sr, std::less<int>(), &U::k));

    ranges::fill(ic, U{0});
    auto res2 = ranges::set_union(std::move(ib), ranges::views::all(ia), ic, std::less<int>(), &T::j, &S::i);
    EXPECT_EQ((res2.out - ic), sr);
    EXPECT_FALSE(ranges::lexicographical_compare(ic, res2.out, ir, ir+sr, std::less<int>(), &U::k));

    std::vector<S> vec_ia{S{1}, S{2}, S{2}, S{3}, S{3}, S{3}, S{4}, S{4}, S{4}, S{4}};
    std::vector<T> vec_ib{T{2}, T{4}, T{4}, T{6}};
    ranges::fill(ic, U{0});
    auto res3 = ranges::set_union(std::move(vec_ia), ranges::views::all(vec_ib), ic, std::less<int>(), &S::i, &T::j);
    EXPECT_EQ((res3.out - ic), sr);
    EXPECT_FALSE(ranges::lexicographical_compare(ic, res3.out, ir, ir+sr, std::less<int>(), &U::k));
}

TEST(SetUnion, Constexpr) {
    static_assert(test_constexpr(), "set_union constexpr test failed");
    EXPECT_TRUE(test_constexpr());
}
#endif
