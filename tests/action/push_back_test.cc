#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/take.h>
#include <fermat/view/for_each.h>
#include <fermat/action/push_back.h>
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

TEST(ActionPushBackTest, DirectCall) {
    using namespace fermat::ranges;

    {
        std::vector<int> v;
        push_back(v, {1, 2, 3});
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3}));

        push_back(v, views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12}));

        push_back(v, views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12}));

        int rg[] = {9, 8, 7};
        push_back(v, rg);
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12, 9, 8, 7}));
        push_back(v, rg);
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12, 9, 8, 7, 9, 8, 7}));
    }

    {
        std::list<int> s;
        push_back(s, views::ints | views::take(10) |
                     views::for_each([](int i) { return yield_if(i % 2 == 0, i); }));
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8}));
        push_back(s, 10);
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8, 10}));
    }
}

TEST(ActionPushBackTest, PipeWithMove) {
    using namespace fermat::ranges;

    {
        std::vector<int> v;
        v = std::move(v) | push_back({1, 2, 3});
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3}));

        v = std::move(v) | push_back(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12}));

        v = std::move(v) | push_back(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12}));

        int rg[] = {9, 8, 7};
        v = std::move(v) | push_back(rg);
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12, 9, 8, 7}));
        v = std::move(v) | push_back(rg);
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12, 9, 8, 7, 9, 8, 7}));
    }

    {
        std::list<int> s;
        s = std::move(s) | push_back(views::ints | views::take(10) |
                                     views::for_each([](int i) { return yield_if(i % 2 == 0, i); }));
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8}));
        s = std::move(s) | push_back(10);
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8, 10}));
    }
}

TEST(ActionPushBackTest, InPlacePipe) {
    using namespace fermat::ranges;

    {
        std::vector<int> v;
        v |= push_back({1, 2, 3});
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3}));

        v |= push_back(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12}));

        v |= push_back(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12}));

        int rg[] = {9, 8, 7};
        v |= push_back(rg);
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12, 9, 8, 7}));
        v |= push_back(rg);
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3, 10, 11, 12, 10, 11, 12, 9, 8, 7, 9, 8, 7}));
    }

    {
        std::list<int> s;
        s |= push_back(views::ints | views::take(10) |
                       views::for_each([](int i) { return yield_if(i % 2 == 0, i); }));
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8}));
        s |= push_back(10);
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8, 10}));
    }
}