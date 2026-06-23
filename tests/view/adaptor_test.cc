/// view_adaptor_gtest.cpp
/// Google Test conversion of range-v3 view_adaptor test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <list>
#include <vector>
#include <sstream>
#include <type_traits>

#include <fermat/range/access.h>
#include <fermat/iterator/operations.h>
#include <fermat/utility/copy.h>
#include <fermat/view/delimit.h>
#include <fermat/view/istream.h>
#include <fermat/view/all.h>

/// Helper to compare a range with an initializer list
template<typename Rng, typename T>
void check_equal(Rng &&rng, std::initializer_list<T> expected) {
    auto it = fermat::ranges::begin(rng);
    auto end = fermat::ranges::end(rng);
    for (auto const &val: expected) {
        EXPECT_NE(it, end);
        EXPECT_EQ(*it, val);
        ++it;
    }
    EXPECT_EQ(it, end);
}

// ------------------------------------------------------------------
// my_reverse_view: a view that iterates a bidirectional range backwards
// ------------------------------------------------------------------
template<typename BidiRange>
struct my_reverse_view
        : fermat::ranges::view_adaptor<my_reverse_view<BidiRange>, BidiRange> {
private:
    static_assert(fermat::ranges::bidirectional_range<BidiRange>, "BidiRange must be bidirectional");
    static_assert(fermat::ranges::common_range<BidiRange>, "BidiRange must be common");
    friend fermat::ranges::range_access;
    using base_iterator_t = fermat::ranges::iterator_t<BidiRange>;

    struct adaptor : fermat::ranges::adaptor_base {
        template<class base_mixin>
        struct mixin : base_mixin {
            mixin() = default;

            using base_mixin::base_mixin;

            int mixin_int = 120;

            int base_plus_adaptor() const {
                int y = this->get().t;
                return *this->base() + y;
            }
        };

        int t = 20;

        // Cross-wire begin and end.
        base_iterator_t begin(my_reverse_view const &rng) const {
            return fermat::ranges::end(rng.base());
        }

        base_iterator_t end(my_reverse_view const &rng) const {
            return fermat::ranges::begin(rng.base());
        }

        void next(base_iterator_t &it) const {
            --it;
        }

        void prev(base_iterator_t &it) const {
            ++it;
        }

        fermat::ranges::range_reference_t<BidiRange> read(base_iterator_t it) const {
            return *fermat::ranges::prev(it);
        }

        // Conditionally provide advance and distance_to only when BidiRange is random_access
        template<typename R = BidiRange,
            typename = std::enable_if_t<fermat::ranges::random_access_range<R> > >
        void advance(base_iterator_t &it, fermat::ranges::range_difference_t<BidiRange> n) const {
            it -= n;
        }

        template<typename R = BidiRange,
            typename = std::enable_if_t<fermat::ranges::random_access_range<R> > >
        fermat::ranges::range_difference_t<BidiRange>
        distance_to(base_iterator_t const &here, base_iterator_t const &there) const {
            return here - there;
        }
    };

    adaptor begin_adaptor() const { return {}; }
    adaptor end_adaptor() const { return {}; }

public:
    using my_reverse_view::view_adaptor::view_adaptor;
};

// ------------------------------------------------------------------
// my_delimited_range: a view that wraps delimit_view and adds a mixin
// ------------------------------------------------------------------
struct my_delimited_range
        : fermat::ranges::view_adaptor<
            my_delimited_range,
            fermat::ranges::delimit_view<fermat::ranges::istream_view<int>, int> > {
    using view_adaptor::view_adaptor;

    struct adaptor : fermat::ranges::adaptor_base {
        template<class base_mixin>
        struct mixin : base_mixin {
            mixin() = default;

            using base_mixin::base_mixin;

            int mixin_int = 120;

            int adaptor_access_test() const {
                int y = this->get().t;
                return y;
            }
        };

        int t = 20;
    };

    adaptor begin_adaptor() const { return {}; }
    adaptor end_adaptor() const { return {}; }
};

// ------------------------------------------------------------------
// Test cases
// ------------------------------------------------------------------
TEST(ViewAdaptorTest, ReverseViewWithVector) {
    std::vector<int> v{1, 2, 3, 4};
    my_reverse_view<std::vector<int> &> retro{v};

    static_assert(fermat::ranges::common_range<decltype(retro)>, "");
    static_assert(fermat::ranges::view_<decltype(retro)>, "");
    static_assert(fermat::ranges::random_access_iterator<decltype(retro.begin())>, "");

    check_equal(retro, {4, 3, 2, 1});

    EXPECT_EQ(retro.begin().mixin_int, 120);
    EXPECT_EQ(*(retro.begin() + 1).base(), 4);
    EXPECT_EQ((retro.begin() + 1).base_plus_adaptor(), 24);
}

TEST(ViewAdaptorTest, ReverseViewWithList) {
    std::list<int> l{1, 2, 3, 4};
    my_reverse_view<std::list<int> &> retro2{l};

    static_assert(fermat::ranges::common_range<decltype(retro2)>, "");
    static_assert(fermat::ranges::view_<decltype(retro2)>, "");
    static_assert(fermat::ranges::bidirectional_iterator<decltype(retro2.begin())>, "");
    static_assert(!fermat::ranges::random_access_iterator<decltype(retro2.begin())>, "");

    check_equal(retro2, {4, 3, 2, 1});
}

TEST(ViewAdaptorTest, DelimitedRange) {
    std::stringstream sinx("1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9 1 2 3 4 42 6 7 8 9 ");
    my_delimited_range r{fermat::ranges::views::delimit(fermat::ranges::istream<int>(sinx), 42)};

    static_assert(fermat::ranges::view_<decltype(r)>, "");
    static_assert(!fermat::ranges::common_range<decltype(r)>, "");
    static_assert(fermat::ranges::input_iterator<decltype(r.begin())>, "");
    static_assert(!fermat::ranges::forward_iterator<decltype(r.begin())>, "");

    check_equal(r, {
                    1, 2, 3, 4, 5, 6, 7, 8, 9,
                    1, 2, 3, 4, 5, 6, 7, 8, 9,
                    1, 2, 3, 4
                });

    EXPECT_EQ(r.end().mixin_int, 120);
    EXPECT_EQ(r.end().adaptor_access_test(), 20);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
