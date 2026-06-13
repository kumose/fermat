#include <gtest/gtest.h>
#include <vector>
#include <random>
#include <fermat/action/unstable_remove_if.h>
#include <fermat/action/remove_if.h>
#include <fermat/action/sort.h>
#include <fermat/range/conversion.h>

using namespace ranges;

// ---------- helper for fuzzy test ----------
namespace {
    class fuzzy_test_fn {
        int size;
#if defined(__GLIBCXX__) && defined(RANGES_WORKAROUND_VALGRIND_RDRAND)
        std::random_device rd{"/dev/urandom"};
#else
        std::random_device rd;
#endif
        std::mt19937 eng{rd()};
        std::uniform_int_distribution<int> distr;

    public:
        explicit fuzzy_test_fn(int sz)
          : size(sz)
          , distr{0, sz}
        {}

        void operator()() {
            struct Int {
                int value;
                explicit Int(int v) : value(v) {}
                Int(Int const &) = default;
                Int(Int&& other) noexcept : value(0) { *this = std::move(other); }
                Int& operator=(Int const &) = default;
                Int& operator=(Int&& other) noexcept {
                    const int sentinel = -1;
                    EXPECT_NE(other.value, sentinel);
                    value = other.value;
                    other.value = sentinel;
                    return *this;
                }
                bool operator==(Int const &other) const { return value == other.value; }
                bool operator!=(Int const &other) const { return value != other.value; }
                bool operator<(Int const &other) const { return value < other.value; }
                bool operator>(Int const &other) const { return value > other.value; }
                bool operator<=(Int const &other) const { return value <= other.value; }
                bool operator>=(Int const &other) const { return value >= other.value; }
            };

            std::vector<Int> ordered_list, unordered_list;
            for (int i = 0; i < size; ++i) {
                ordered_list.emplace_back(i);
                unordered_list.emplace_back(i);
            }

            const int erase_count = distr(eng);
            for (int i = 0; i < erase_count; ++i) {
                const int value = distr(eng);
                const auto pred = [value](Int j) { return j.value == value; };
                unordered_list |= actions::unstable_remove_if(pred);
                ordered_list |= actions::remove_if(pred);
            }

            unordered_list |= actions::sort;
            EXPECT_EQ(ordered_list, unordered_list);
        }
    };
} // namespace

// ---------- logic tests ----------
TEST(ActionUnstableRemoveIfTest, Empty) {
    std::vector<int> vec;
    vec |= actions::unstable_remove_if([](int) { return true; });
    EXPECT_TRUE(vec.empty());
}

TEST(ActionUnstableRemoveIfTest, AllStay) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int) { return false; });
    EXPECT_TRUE((vec == std::vector<int>{1,2,3,4,5}));
}

TEST(ActionUnstableRemoveIfTest, AllRemove) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int) { return true; });
    EXPECT_TRUE(vec.empty());
}

TEST(ActionUnstableRemoveIfTest, RemoveMiddle) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i == 2; });
    EXPECT_TRUE((vec == std::vector<int>{1,5,3,4}));
}

TEST(ActionUnstableRemoveIfTest, RemoveFirst) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i == 1; });
    EXPECT_TRUE((vec == std::vector<int>{5,2,3,4}));
}

TEST(ActionUnstableRemoveIfTest, RemoveLast) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i == 5; });
    EXPECT_TRUE((vec == std::vector<int>{1,2,3,4}));
}

TEST(ActionUnstableRemoveIfTest, RemoveGroupMiddle) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i == 2 || i == 3 || i == 4; });
    EXPECT_TRUE((vec == std::vector<int>{1,5}));
}

TEST(ActionUnstableRemoveIfTest, RemoveGroupBegin) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i == 1 || i == 2 || i == 3; });
    EXPECT_TRUE((vec == std::vector<int>{5,4}));
}

TEST(ActionUnstableRemoveIfTest, RemoveGroupEnd) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i == 3 || i == 4 || i == 5; });
    EXPECT_TRUE((vec == std::vector<int>{1,2}));
}

TEST(ActionUnstableRemoveIfTest, RemainsOneMiddle) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i != 3; });
    EXPECT_TRUE((vec == std::vector<int>{3}));
}

TEST(ActionUnstableRemoveIfTest, RemainsGroupMiddle) {
    std::vector<int> vec = {1,2,3,4,5};
    vec |= actions::unstable_remove_if([](int i) { return i != 3 && i != 4; });
    EXPECT_TRUE((vec == std::vector<int>{4,3}));
}

// ---------- predicate call count tests ----------
TEST(ActionUnstableRemoveIfTest, PredicateCallCount) {
    auto is_zero_count_invocations = [counter = 0](int i) mutable -> bool {
        ++counter;
        if (i == 0) return true;
        return false;
    };

    {
        std::vector<int> vec{0};
        int cnt = 0;
        auto pred = [&cnt](int i) { ++cnt; return i == 0; };
        vec |= actions::unstable_remove_if(pred);
        EXPECT_EQ(cnt, 1);
    }
    {
        std::vector<int> vec{1,1,1};
        int cnt = 0;
        auto pred = [&cnt](int i) { ++cnt; return i == 0; };
        vec |= actions::unstable_remove_if(pred);
        EXPECT_EQ(cnt, 3);
    }
    {
        std::vector<int> vec{1,0};
        int cnt = 0;
        auto pred = [&cnt](int i) { ++cnt; return i == 0; };
        vec |= actions::unstable_remove_if(pred);
        EXPECT_EQ(cnt, 2);
    }
    {
        std::vector<int> vec{1,2,0};
        int cnt = 0;
        auto pred = [&cnt](int i) { ++cnt; return i == 0; };
        vec |= actions::unstable_remove_if(pred);
        EXPECT_EQ(cnt, 3);
    }
    {
        std::vector<int> vec{0,0,0,0};
        int cnt = 0;
        auto pred = [&cnt](int i) { ++cnt; return i == 0; };
        vec |= actions::unstable_remove_if(pred);
        EXPECT_EQ(cnt, 4);
    }
    {
        std::vector<int> vec{1,2,3,0,0,0,0,4,5};
        int cnt = 0;
        auto pred = [&cnt](int i) { ++cnt; return i == 0; };
        vec |= actions::unstable_remove_if(pred);
        EXPECT_EQ(cnt, 9);
    }
}

// ---------- fuzzy test ----------
TEST(ActionUnstableRemoveIfTest, Fuzzy) {
    const int size = 100;
    const int repeats = 1000;
    fuzzy_test_fn fuzzy_test(size);
    for (int i = 0; i < repeats; ++i) {
        fuzzy_test();
    }
}