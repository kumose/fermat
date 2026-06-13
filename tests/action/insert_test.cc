#include <gtest/gtest.h>
#include <set>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/take.h>
#include <fermat/view/for_each.h>
#include <fermat/view/ref.h>
#include <fermat/action/insert.h>
#include <fermat/range/conversion.h>

namespace {
    template<typename T>
    struct vector_like : std::vector<T> {
        using std::vector<T>::vector;
        using typename std::vector<T>::size_type;

        size_type last_reservation{};
        size_type reservation_count{};

        void reserve(size_type n) {
            std::vector<T>::reserve(n);
            last_reservation = n;
            ++reservation_count;
        }
    };
} // namespace

TEST(ActionInsertTest, Vector) {
    using namespace fermat::ranges;

    std::vector<int> v;
    auto i = insert(v, v.begin(), 42);
    EXPECT_EQ(i, v.begin());
    EXPECT_TRUE((v == std::vector<int>{42}));

    insert(v, v.end(), {1, 2, 3});
    EXPECT_TRUE((v == std::vector<int>{42, 1, 2, 3}));

    insert(v, v.begin(), views::ints | views::take(3));
    EXPECT_TRUE((v == std::vector<int>{0, 1, 2, 42, 1, 2, 3}));

    int rg[] = {9, 8, 7};
    insert(v, v.begin() + 3, rg);
    EXPECT_TRUE((v == std::vector<int>{0, 1, 2, 9, 8, 7, 42, 1, 2, 3}));

    insert(v, v.begin() + 1, rg);
    EXPECT_TRUE((v == std::vector<int>{0, 9, 8, 7, 1, 2, 9, 8, 7, 42, 1, 2, 3}));
}

TEST(ActionInsertTest, Set) {
    using namespace fermat::ranges;

    std::set<int> s;
    insert(s, views::ints | views::take(10) |
              views::for_each([](int i) { return yield_if(i % 2 == 0, i); }));
    EXPECT_TRUE((s == std::set<int>{0, 2, 4, 6, 8}));

    auto j = insert(s, 10);
    EXPECT_EQ(j.first, prev(s.end()));
    EXPECT_TRUE(j.second);
    EXPECT_TRUE((s == std::set<int>{0, 2, 4, 6, 8, 10}));

    insert(views::ref(s), 12);
    EXPECT_TRUE((s == std::set<int>{0, 2, 4, 6, 8, 10, 12}));
}

TEST(ActionInsertTest, VectorLikeReservation) {
    using namespace fermat::ranges;

    const std::size_t N = 1024;
    vector_like<int> vl;
    insert(vl, vl.end(), views::iota(0, int{N}));
    EXPECT_EQ(vl.reservation_count, 1u);
    EXPECT_EQ(vl.last_reservation, N);

    auto r = views::iota(0, int{2 * N});
    insert(vl, vl.begin() + 42, begin(r), end(r));
    EXPECT_EQ(vl.reservation_count, 2u);
    EXPECT_EQ(vl.last_reservation, 3 * N);

    int i = 42;
    insert(vl, vl.end(), &i, &i + 1);
    EXPECT_EQ(vl.reservation_count, 3u);
    EXPECT_GT(vl.last_reservation, 3 * N + 1);
}