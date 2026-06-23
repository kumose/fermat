#include <gtest/gtest.h>
#include <list>
#include <vector>
#include <fermat/core.h>
#include <fermat/view/iota.h>
#include <fermat/view/take.h>
#include <fermat/view/for_each.h>
#include <fermat/action/push_front.h>
#include <fermat/range/conversion.h>

TEST(ActionPushFrontTest, DirectCall) {
    using namespace fermat::ranges;

    {
        std::vector<int> v;
        push_front(v, {1, 2, 3});
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3}));

        push_front(v, views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{10, 11, 12, 1, 2, 3}));

        push_front(v, views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{10, 11, 12, 10, 11, 12, 1, 2, 3}));

        int rg[] = {9, 8, 7};
        push_front(v, rg);
        EXPECT_TRUE((v == std::vector<int>{9, 8, 7, 10, 11, 12, 10, 11, 12, 1, 2, 3}));
        push_front(v, rg);
        EXPECT_TRUE((v == std::vector<int>{9, 8, 7, 9, 8, 7, 10, 11, 12, 10, 11, 12, 1, 2, 3}));
    }

    {
        std::list<int> s;
        push_front(s, views::ints | views::take(10) |
                      views::for_each([](int i) { return yield_if(i % 2 == 0, i); }));
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8}));
        push_front(s, -2);
        EXPECT_TRUE((s == std::list<int>{-2, 0, 2, 4, 6, 8}));
    }
}

TEST(ActionPushFrontTest, PipeWithMove) {
    using namespace fermat::ranges;

    {
        std::vector<int> v;
        v = std::move(v) | push_front({1, 2, 3});
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3}));

        v = std::move(v) | push_front(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{10, 11, 12, 1, 2, 3}));

        v = std::move(v) | push_front(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{10, 11, 12, 10, 11, 12, 1, 2, 3}));

        int rg[] = {9, 8, 7};
        v = std::move(v) | push_front(rg);
        EXPECT_TRUE((v == std::vector<int>{9, 8, 7, 10, 11, 12, 10, 11, 12, 1, 2, 3}));
        v = std::move(v) | push_front(rg);
        EXPECT_TRUE((v == std::vector<int>{9, 8, 7, 9, 8, 7, 10, 11, 12, 10, 11, 12, 1, 2, 3}));
    }

    {
        std::list<int> s;
        s = std::move(s) | push_front(views::ints | views::take(10) |
                                      views::for_each([](int i) { return yield_if(i % 2 == 0, i); }));
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8}));
        s = std::move(s) | push_front(-2);
        EXPECT_TRUE((s == std::list<int>{-2, 0, 2, 4, 6, 8}));
    }
}

TEST(ActionPushFrontTest, InPlacePipe) {
    using namespace fermat::ranges;

    {
        std::vector<int> v;
        v |= push_front({1, 2, 3});
        EXPECT_TRUE((v == std::vector<int>{1, 2, 3}));

        v |= push_front(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{10, 11, 12, 1, 2, 3}));

        v |= push_front(views::iota(10) | views::take(3));
        EXPECT_TRUE((v == std::vector<int>{10, 11, 12, 10, 11, 12, 1, 2, 3}));

        int rg[] = {9, 8, 7};
        v |= push_front(rg);
        EXPECT_TRUE((v == std::vector<int>{9, 8, 7, 10, 11, 12, 10, 11, 12, 1, 2, 3}));
        v |= push_front(rg);
        EXPECT_TRUE((v == std::vector<int>{9, 8, 7, 9, 8, 7, 10, 11, 12, 10, 11, 12, 1, 2, 3}));
    }

    {
        std::list<int> s;
        s |= push_front(views::ints | views::take(10) |
                        views::for_each([](int i) { return yield_if(i % 2 == 0, i); }));
        EXPECT_TRUE((s == std::list<int>{0, 2, 4, 6, 8}));
        s |= push_front(-2);
        EXPECT_TRUE((s == std::list<int>{-2, 0, 2, 4, 6, 8}));
    }
}
