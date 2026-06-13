#include <gtest/gtest.h>

#include <list>
#include <map>
#include <vector>

#include <fermat/action/sort.h>
#include <fermat/core.h>
#include <fermat/range/conversion.h>
#include <fermat/view/indices.h>
#include <fermat/view/iota.h>
#include <fermat/view/take.h>
#include <fermat/view/transform.h>
#include <fermat/view/zip.h>
#include <fermat/view/reverse.h>

// Helper: vector_like with reservation tracking
template<typename T>
struct vector_like {
private:
    std::vector<T> data_;

public:
    using size_type = std::size_t;
    using allocator_type = std::allocator<T>;

    vector_like() = default;

    template<typename I>
    vector_like(I first, I last) : data_(first, last) {
    }

    template<typename I>
    void assign(I first, I last) { data_.assign(first, last); }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
    size_type size() const { return data_.size(); }
    size_type capacity() const { return data_.capacity(); }
    size_type max_size() const { return data_.max_size(); }
    auto &operator[](size_type n) { return data_[n]; }
    auto &operator[](size_type n) const { return data_[n]; }

    size_type last_reservation{};
    size_type reservation_count{};

    void reserve(size_type n) {
        data_.reserve(n);
        last_reservation = n;
        ++reservation_count;
    }
};

// Helper: map_like
template<typename K, typename V>
struct map_like : std::map<K, V> {
    template<typename Iter>
    map_like(Iter f, Iter l) : std::map<K, V>(f, l) {
    }
};

// Helper: check_equal for containers (simple)
template<typename C, typename V>
void check_equal(const C &c, const std::initializer_list<V> &expected) {
    auto it = c.begin();
    auto ex = expected.begin();
    while (it != c.end() && ex != expected.end()) {
        EXPECT_EQ(*it, *ex);
        ++it;
        ++ex;
    }
    EXPECT_TRUE(it == c.end() && ex == expected.end());
}

// Helper: check_map_equal (for map-like containers)
template<typename M, typename K, typename V>
void check_map_equal(const M &m, const std::initializer_list<std::pair<K, V> > &expected) {
    auto it = m.begin();
    auto ex = expected.begin();
    while (it != m.end() && ex != expected.end()) {
        EXPECT_EQ(it->first, ex->first);
        EXPECT_EQ(it->second, ex->second);
        ++it;
        ++ex;
    }
    EXPECT_TRUE(it == m.end() && ex == expected.end());
}

// Dummy overload for test_zip_to_map (compile-time check)
#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
template<typename Rng, typename CI = ranges::range_common_iterator_t<Rng>,
    typename = decltype(std::map{CI{}, CI{}})> // SFINAE
void test_zip_to_map(Rng &&rng, int) {
    using namespace ranges;
#ifdef RANGES_WORKAROUND_MSVC_779708
    auto m = static_cast<Rng &&>(rng) | to<std::map>();
#else
    auto m = static_cast<Rng &&>(rng) | to<std::map>;
#endif
    static_assert(std::is_same_v<decltype(m), std::map<int, int> >);
    // No runtime checks needed
}
#endif
template<typename Rng>
void test_zip_to_map(Rng &&, long) {
}

// GTest fixture not needed, just standalone tests.

TEST(RangeConversionTest, ToStdListFromIotaTransformTake) {
    using namespace ranges;
    auto lst0 = views::ints | views::transform([](int i) { return i * i; }) |
                views::take(10) | to<std::list>();
    static_assert(std::is_same_v<decltype(lst0), std::list<int> >);
    check_equal(lst0, {0, 1, 4, 9, 16, 25, 36, 49, 64, 81});
}

#ifndef RANGES_WORKAROUND_MSVC_779708
TEST(RangeConversionTest, ToStdListFromIotaTransformTakeWithoutParens) {
    using namespace ranges;
    auto lst1 = views::ints | views::transform([](int i) { return i * i; }) |
                views::take(10) | to<std::list>;
    static_assert(std::is_same_v<decltype(lst1), std::list<int> >);
    check_equal(lst1, {0, 1, 4, 9, 16, 25, 36, 49, 64, 81});
}
#endif

TEST(RangeConversionTest, ToVectorThenSort) {
    using namespace ranges;
    auto vec0 = views::ints | views::transform([](int i) { return i * i; }) |
                views::take(10) | to_vector | actions::sort(std::greater<int>{});
    static_assert(std::is_same_v<decltype(vec0), std::vector<int> >);
    check_equal(vec0, {81, 64, 49, 36, 25, 16, 9, 4, 1, 0});
}

TEST(RangeConversionTest, ToVectorLong) {
    using namespace ranges;
    auto vec1 = views::ints | views::transform([](int i) { return i * i; }) |
                views::take(10) | to<std::vector<long> >() |
                actions::sort(std::greater<long>{});
    static_assert(std::is_same_v<decltype(vec1), std::vector<long> >);
    check_equal(vec1, {81, 64, 49, 36, 25, 16, 9, 4, 1, 0});
}

#ifndef RANGES_WORKAROUND_MSVC_779708
TEST(RangeConversionTest, ToVectorLongWithoutParens) {
    using namespace ranges;
    auto vec2 = views::ints | views::transform([](int i) { return i * i; }) |
                views::take(10) | to<std::vector<long> > |
                actions::sort(std::greater<long>{});
    static_assert(std::is_same_v<decltype(vec2), std::vector<long> >);
    check_equal(vec2, {81, 64, 49, 36, 25, 16, 9, 4, 1, 0});
}
#endif

TEST(RangeConversionTest, ToVectorLikeWithReserve) {
    using namespace ranges;
    const std::size_t N = 4096;
    auto vl = views::iota(0, int{N}) | to<vector_like<int> >();
    static_assert(std::is_same_v<decltype(vl), vector_like<int> >);
    EXPECT_EQ(vl.reservation_count, 1);
    EXPECT_EQ(vl.last_reservation, N);
}

TEST(RangeConversionTest, ZipToMap) {
    using namespace ranges;
    // Issue #1145
    auto r1 = views::indices(std::uintmax_t{100});
    auto r2 = views::zip(r1, r1);
#ifdef RANGES_WORKAROUND_MSVC_779708
    auto m = r2 | ranges::to<std::map<std::uintmax_t, std::uintmax_t> >();
#else
    auto m = r2 | ranges::to<std::map<std::uintmax_t, std::uintmax_t> >;
#endif
    static_assert(std::is_same_v<decltype(m), std::map<std::uintmax_t, std::uintmax_t> >);
    EXPECT_EQ(m.size(), 100u);
    for (std::uintmax_t i = 0; i < 100; ++i) {
        auto it = m.find(i);
        EXPECT_NE(it, m.end());
        EXPECT_EQ(it->second, i);
    }
}

TEST(RangeConversionTest, TransformRangeOfRangesToVectorOfVectors) {
    using namespace ranges;
    auto r = views::ints(1, 4) |
             views::transform([](int i) { return views::ints(i, i + 3); });
    auto m = r | ranges::to<std::vector<std::vector<int> > >();
    static_assert(std::is_same_v<decltype(m), std::vector<std::vector<int> > >);
    EXPECT_EQ(m.size(), 3u);
    check_equal(m[0], {1, 2, 3});
    check_equal(m[1], {2, 3, 4});
    check_equal(m[2], {3, 4, 5});
}

TEST(RangeConversionTest, ClosureWithAction) {
    using namespace ranges;
#ifdef RANGES_WORKAROUND_MSVC_779708
    auto closure = ranges::to<std::vector>() | actions::sort;
#else
    auto closure = ranges::to<std::vector> | actions::sort;
#endif
    auto r = views::ints(1, 4) | views::reverse;
    auto m = r | closure;
    static_assert(std::is_same_v<decltype(m), std::vector<int> >);
    EXPECT_EQ(m.size(), 3u);
    check_equal(m, {1, 2, 3});
}

TEST(RangeConversionTest, ZipToMapCompileTimeCheck) {
    // This test only checks compile-time overload resolution.
    test_zip_to_map(ranges::views::zip(ranges::views::ints, ranges::views::iota(0, 10)), 0);
}

TEST(RangeConversionTest, TransformAllOfVectorOfVectors) {
    using namespace ranges;
    std::vector<std::vector<int> > d;
    auto m = views::transform(d, views::all);
    auto v = ranges::to<std::vector<std::vector<int> > >(m);
    EXPECT_EQ(v, d);
}

TEST(RangeConversionTest, ToMapLike) {
    using namespace ranges;
    using MapType = map_like<int, int>;
    std::vector<std::pair<int, int>> v = {{1, 2}, {3, 4}};

    auto m1 = ranges::to<MapType>(v);
    auto m2 = v | ranges::to<MapType>();

    static_assert(std::is_same_v<decltype(m1), MapType>);
    static_assert(std::is_same_v<decltype(m2), MapType>);

    std::map<int, int> expected = {{1, 2}, {3, 4}};
    EXPECT_EQ((static_cast<std::map<int, int>>(m1)), expected);
    EXPECT_EQ((static_cast<std::map<int, int>>(m2)), expected);
}