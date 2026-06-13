/// subrange_gtest.cpp
/// Google Test conversion of range-v3 subrange test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.
/// No main() function – entry point expected to be provided externally.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <utility>

#include <fermat/range/access.h>                 /// fermat::ranges::begin, fermat::ranges::end, fermat::ranges::size
#include <fermat/view/subrange.h>          /// fermat::ranges::subrange, fermat::ranges::borrowed_subrange_t, fermat::ranges::dangling
#include <fermat/view/all.h>               /// views::all
#include <fermat/view/ref.h>               /// views::ref_view
#include <fermat/iterator/unreachable_sentinel.h> /// fermat::ranges::unreachable_sentinel_t

/// Helper function to test borrowed_subrange_t (same as original).
template<typename Rng>
fermat::ranges::borrowed_subrange_t<Rng> algorithm(Rng&& rng) {
    return {fermat::ranges::begin(rng), fermat::ranges::end(rng)};
}

struct Base {};
struct Derived : Base {};

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------

TEST(SubrangeTest, BorrowedSubrangeType) {
    using namespace fermat::ranges;

    // lvalue arrays -> subrange<int*>
    static_assert(std::is_same<subrange<int*>,
                decltype(::algorithm(std::declval<int(&)[42]>()))>::value, "");
    // lvalue vector -> subrange<std::vector<int>::iterator>
    std::vector<int> vi{1,2,3,4};
    static_assert(std::is_same<subrange<std::vector<int>::iterator>,
                decltype(::algorithm(vi))>::value, "");

    // subrange and ref_view are ReferenceableRanges
    static_assert(std::is_same<subrange<int*>,
                decltype(::algorithm(std::declval<subrange<int*>>()))>::value, "");
    static_assert(std::is_same<subrange<int*>,
                decltype(::algorithm(std::declval<ref_view<int[42]>>()))>::value, "");

    // non-ReferenceableRange rvalue ranges dangle
    static_assert(std::is_same<dangling,
                decltype(::algorithm(std::declval<std::vector<int>>()))>::value, "");
    static_assert(std::is_same<dangling,
                decltype(::algorithm(std::move(vi)))>::value, "");
}

TEST(SubrangeTest, SlicingConversionsNotAllowed) {
    using namespace fermat::ranges;

    static_assert(std::is_constructible<subrange<Base*, Base*>, Base*, Base*>::value, "");
    static_assert(!std::is_constructible<subrange<Base*, Base*>, Derived*, Derived*>::value, "");
    static_assert(std::is_constructible<subrange<const Base*, const Base*>, Base*, Base*>::value, "");
    static_assert(!std::is_constructible<subrange<const Base*, const Base*>, Derived*, Derived*>::value, "");
    static_assert(!std::is_constructible<subrange<Base*, Base*>, subrange<Derived*, Derived*>>::value, "");

    static_assert(std::is_constructible<subrange<Base*, unreachable_sentinel_t>, Base*, unreachable_sentinel_t>::value, "");
    static_assert(!std::is_constructible<subrange<Base*, unreachable_sentinel_t>, Derived*, unreachable_sentinel_t>::value, "");
    static_assert(std::is_constructible<subrange<const Base*, unreachable_sentinel_t>, Base*, unreachable_sentinel_t>::value, "");
    static_assert(!std::is_constructible<subrange<const Base*, unreachable_sentinel_t>, Derived*, unreachable_sentinel_t>::value, "");
    static_assert(!std::is_constructible<subrange<Base*, unreachable_sentinel_t>, subrange<Derived*, unreachable_sentinel_t>>::value, "");

    static_assert(std::is_constructible<subrange<Base*, Base*, subrange_kind::sized>, Base*, Base*, std::size_t>::value, "");
    static_assert(!std::is_constructible<subrange<Base*, Base*, subrange_kind::sized>, Derived*, Base*, std::size_t>::value, "");
    static_assert(std::is_constructible<subrange<const Base*, const Base*, subrange_kind::sized>, Base*, const Base*, std::size_t>::value, "");
    static_assert(!std::is_constructible<subrange<const Base*, const Base*, subrange_kind::sized>, Derived*, const Base*, std::size_t>::value, "");
    static_assert(!std::is_constructible<subrange<Base*, Base*, subrange_kind::sized>, subrange<Derived*, Base*>, std::size_t>::value, "");

    static_assert(std::is_convertible<subrange<Base*, Base*>, std::pair<const Base*, const Base*>>::value, "");
    static_assert(!std::is_convertible<subrange<Derived*, Derived*>, std::pair<Base*, Base*>>::value, "");
}

TEST(SubrangeTest, BasicOperations) {
    using namespace fermat::ranges;

    std::vector<int> vi{1,2,3,4};
    subrange<std::vector<int>::iterator> r0{vi.begin(), vi.end()};

    static_assert(std::tuple_size<decltype(r0)>::value == 2, "");
    static_assert(std::is_same<std::vector<int>::iterator,
                std::tuple_element<0, decltype(r0)>::type>::value, "");
    static_assert(std::is_same<std::vector<int>::iterator,
                std::tuple_element<1, decltype(r0)>::type>::value, "");
    static_assert(sized_range<decltype(r0)>, "");
    EXPECT_EQ(r0.size(), 4u);
    EXPECT_EQ(r0.begin(), vi.begin());
    EXPECT_EQ(fermat::ranges::get<0>(r0), vi.begin());
    EXPECT_EQ(r0.end(), vi.end());
    EXPECT_EQ(fermat::ranges::get<1>(r0), vi.end());

    r0 = r0.next();
    EXPECT_EQ(r0.size(), 3u);
}

TEST(SubrangeTest, SizeConstructor) {
    using namespace fermat::ranges;

    std::vector<int> vi{1,2,3,4};
    subrange<std::vector<int>::iterator> rng{vi.begin(), vi.end(), fermat::ranges::size(vi)};
    EXPECT_EQ(rng.size(), 4u);
    EXPECT_EQ(rng.begin(), vi.begin());
    EXPECT_EQ(rng.end(), vi.end());
}

TEST(SubrangeTest, ConversionToPair) {
    using namespace fermat::ranges;

    std::vector<int> vi{1,2,3,4};
    subrange<std::vector<int>::iterator> r0{vi.begin()+1, vi.end()};
    std::pair<std::vector<int>::iterator, std::vector<int>::iterator> p0 = r0;
    EXPECT_EQ(p0.first, vi.begin()+1);
    EXPECT_EQ(p0.second, vi.end());
}

TEST(SubrangeTest, UnreachableSentinel) {
    using namespace fermat::ranges;

    std::vector<int> vi{1,2,3,4};
    subrange<std::vector<int>::iterator> r0{vi.begin()+1, vi.end()};
    subrange<std::vector<int>::iterator, unreachable_sentinel_t> r1{r0.begin(), {}};

    static_assert(std::tuple_size<decltype(r1)>::value == 2, "");
    static_assert(std::is_same<std::vector<int>::iterator,
                std::tuple_element<0, decltype(r1)>::type>::value, "");
    static_assert(std::is_same<unreachable_sentinel_t,
                std::tuple_element<1, decltype(r1)>::type>::value, "");
    static_assert(view_<decltype(r1)>, "");
    static_assert(!sized_range<decltype(r1)>, "");
    EXPECT_EQ(r1.begin(), vi.begin()+1);
    r1.end() = unreachable;   // no effect but compiles

    std::pair<std::vector<int>::iterator, unreachable_sentinel_t> p1 = r1;
    EXPECT_EQ(p1.first, vi.begin()+1);
}

#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
// Deduction guides tests – compile‑time only.
TEST(SubrangeTest, DeductionGuides) {
    using namespace fermat::ranges;
    std::vector<int> vi{1,2,3,4};
    std::list<int> li{1,2,3,4};

    {
        subrange s0{vi.begin(), vi.end()};
        subrange s1{li.begin(), li.end()};
        static_assert(std::is_same<decltype(s0), subrange<std::vector<int>::iterator>>::value, "");
        static_assert(std::is_same<decltype(s1), subrange<std::list<int>::iterator>>::value, "");
    }
    {
        subrange s0{vi.begin(), vi.end(), fermat::ranges::size(vi)};
        subrange s1{li.begin(), li.end(), fermat::ranges::size(li)};
        static_assert(std::is_same<decltype(s0), subrange<std::vector<int>::iterator, std::vector<int>::iterator, subrange_kind::sized>>::value, "");
        static_assert(std::is_same<decltype(s1), subrange<std::list<int>::iterator, std::list<int>::iterator, subrange_kind::sized>>::value, "");
    }
    {
        subrange s0{vi};
        subrange s1{li};
        subrange s2{views::all(vi)};
        subrange s3{views::all(li)};
        static_assert(std::is_same<decltype(s0), subrange<std::vector<int>::iterator>>::value, "");
        static_assert(std::is_same<decltype(s1), subrange<std::list<int>::iterator, std::list<int>::iterator, subrange_kind::sized>>::value, "");
        static_assert(std::is_same<decltype(s2), subrange<std::vector<int>::iterator>>::value, "");
        static_assert(std::is_same<decltype(s3), subrange<std::list<int>::iterator, std::list<int>::iterator, subrange_kind::sized>>::value, "");
    }
    // Removed undefined 'r0' line that caused compilation error.
    {
        subrange s0{vi, fermat::ranges::size(vi)};
        subrange s1{li, fermat::ranges::size(li)};
        static_assert(std::is_same<decltype(s0), subrange<std::vector<int>::iterator, std::vector<int>::iterator, subrange_kind::sized>>::value, "");
        static_assert(std::is_same<decltype(s1), subrange<std::list<int>::iterator, std::list<int>::iterator, subrange_kind::sized>>::value, "");
    }
    {
        subrange s(li.begin(), li.end());
        subrange s2 = s.next();
        EXPECT_EQ(s2.begin(), std::next(li.begin()));
        EXPECT_EQ(s2.end(), li.end());
    }
}
#endif // deduction guides
