// basic_iterator_test.cc
// Google Test conversion of range-v3 basic_iterator test.
// All comments in English.

#include <gtest/gtest.h>
#include <cstring>
#include <tuple>

#include <fermat/meta/meta.h>

#include <fermat/iterator/basic_iterator.h>
#include <fermat/utility/common_tuple.h>

// ------------------------------------------------------------
// Helper traits (from test_utils.hpp)
// ------------------------------------------------------------
template<typename T>
using iter_cat_t = typename T::iterator_category;
template<typename T>
using has_iter_cat = meta::is_trait<meta::defer<iter_cat_t, T> >;

// ------------------------------------------------------------
// Helper to ignore unused variables
// ------------------------------------------------------------
template<typename... T>
void ignore_unused(T &&...) {
}

// ------------------------------------------------------------
// test_weak_input
// ------------------------------------------------------------
namespace test_weak_input {
    template<typename I>
    struct cursor {
        I it_;

        struct mixin : fermat::ranges::basic_mixin<cursor> {
            mixin() = default;

            explicit mixin(cursor &&cur) : fermat::ranges::basic_mixin<cursor>(static_cast<cursor &&>(cur)) {
            }

            explicit mixin(cursor const &cur) : fermat::ranges::basic_mixin<cursor>(cur) {
            }

            mixin(I i) : mixin(cursor{i}) {
            }
        };

        cursor() = default;

        explicit cursor(I i) : it_(i) {
        }

        CPP_template(class J)(requires fermat::ranges::convertible_to<J, I>)
        cursor(cursor<J> that) : it_(std::move(that.it_)) {
        }

        auto read() const -> decltype(*it_) { return *it_; }
        void next() { ++it_; }
    };

    static_assert(fermat::ranges::detail::input_cursor<cursor<char *> >);
    static_assert(!fermat::ranges::detail::sentinel_for_cursor<cursor<char *>, cursor<char *> >);

    template<class I>
    using iterator = fermat::ranges::basic_iterator<cursor<I> >;
    static_assert(fermat::ranges::indirectly_readable<iterator<char *> >);
    static_assert(fermat::ranges::input_iterator<iterator<char *> >);

    // has_iter_cat checks – fixed with the global trait
    static_assert(!has_iter_cat<iterator<char *> >::value, "");
    static_assert(!has_iter_cat<std::iterator_traits<iterator<char *> > >::value, "");
    static_assert(std::is_same<iterator<char *>::iterator_concept, std::input_iterator_tag>::value, "");
    static_assert(!fermat::ranges::equality_comparable<iterator<char *> >, "");

    void test() {
        using I = iterator<char const *>;
        static_assert(std::is_same<std::iterator_traits<I>::pointer, char const *>{});
        static char const sz[] = "hello world";
        I i{sz};
        EXPECT_EQ(*i, 'h');
        EXPECT_EQ(&*i, i.operator->());
        ++i;
        EXPECT_EQ(*i, 'e');
        EXPECT_EQ(&*i, i.operator->());
    }
} // namespace test_weak_input

// ------------------------------------------------------------
// test_random_access
// ------------------------------------------------------------
namespace test_random_access {
    template<typename I>
    struct cursor {
        I it_;

        struct mixin : fermat::ranges::basic_mixin<cursor> {
            mixin() = default;

            explicit mixin(cursor &&cur) : fermat::ranges::basic_mixin<cursor>(static_cast<cursor &&>(cur)) {
            }

            explicit mixin(cursor const &cur) : fermat::ranges::basic_mixin<cursor>(cur) {
            }

            mixin(I i) : mixin(cursor{i}) {
            }
        };

        cursor() = default;

        explicit cursor(I i) : it_(i) {
        }

        CPP_template(class J)(requires fermat::ranges::convertible_to<J, I>)
        cursor(cursor<J> that) : it_(std::move(that.it_)) {
        }

        auto read() const -> decltype(*it_) { return *it_; }
        CPP_template(class J)(requires fermat::ranges::sentinel_for<J, I>)
        bool equal(cursor<J> const &that) const { return that.it_ == it_; }

        void next() { ++it_; }
        void prev() { --it_; }
        void advance(fermat::ranges::iter_difference_t<I> n) { it_ += n; }
        CPP_template(class J)(requires fermat::ranges::sized_sentinel_for<J, I>)
        fermat::ranges::iter_difference_t<I> distance_to(cursor<J> const &that) const { return that.it_ - it_; }
    };

    static_assert(fermat::ranges::detail::random_access_cursor<cursor<char *> >);

    template<class I>
    using iterator = fermat::ranges::basic_iterator<cursor<I> >;
    static_assert(std::is_same<iterator<char *>::iterator_category, std::random_access_iterator_tag>::value, "");

    void test() {
        using namespace fermat::ranges;
        iterator<char *> a(nullptr);
        iterator<char const *> b(nullptr);
        iterator<char const *> c(a);
        static_assert(std::is_same<std::iterator_traits<iterator<char *> >::pointer, char *>{});
        b = a;
        bool d = a == b;
        d = (a != b);
        ignore_unused(d, a < b, a <= b, a > b, a >= b, (a - b), (b - a), (a - a), (b - b));
    }
} // namespace test_random_access

// ------------------------------------------------------------
// test_weak_output
// ------------------------------------------------------------
namespace test_weak_output {
    template<typename I>
    struct cursor {
        struct mixin : fermat::ranges::basic_mixin<cursor> {
            mixin() = default;

            explicit mixin(cursor &&cur) : fermat::ranges::basic_mixin<cursor>(static_cast<cursor &&>(cur)) {
            }

            explicit mixin(cursor const &cur) : fermat::ranges::basic_mixin<cursor>(cur) {
            }

            explicit mixin(I i) : mixin(cursor{i}) {
            }
        };

        cursor() = default;

        explicit cursor(I i) : it_(i) {
        }

        void write(fermat::ranges::iter_value_t<I> v) const { *it_ = v; }
        void next() { ++it_; }

    private:
        I it_;
    };

    static_assert(fermat::ranges::detail::output_cursor<cursor<char *>, char>);
    static_assert(!fermat::ranges::detail::sentinel_for_cursor<cursor<char *>, cursor<char *> >);

    template<class I>
    using iterator = fermat::ranges::basic_iterator<cursor<I> >;
    static_assert(fermat::ranges::output_iterator<iterator<char *>, char>);
    static_assert(!fermat::ranges::equality_comparable<iterator<char *> >);

    void test() {
        char buf[10];
        iterator<char *> i(buf);
        *i = 'h';
        ++i;
        *i = 'e';
        ++i;
        *i = 'l';
        ++i;
        *i = 'l';
        ++i;
        *i = 'o';
        ++i;
        *i = '\0';
        EXPECT_EQ(std::strcmp(buf, "hello"), 0);
    }
} // namespace test_weak_output

// ------------------------------------------------------------
// test_output
// ------------------------------------------------------------
namespace test_output {
    template<typename I>
    struct cursor {
        I it_;

        struct mixin : fermat::ranges::basic_mixin<cursor> {
            mixin() = default;

            explicit mixin(cursor &&cur) : fermat::ranges::basic_mixin<cursor>(static_cast<cursor &&>(cur)) {
            }

            explicit mixin(cursor const &cur) : fermat::ranges::basic_mixin<cursor>(cur) {
            }

            mixin(I i) : mixin(cursor{i}) {
            }
        };

        cursor() = default;

        explicit cursor(I i) : it_(i) {
        }

        CPP_template(class J)(requires fermat::ranges::convertible_to<J, I>)
        cursor(cursor<J> that) : it_(std::move(that.it_)) {
        }

        using value_type = fermat::ranges::iter_value_t<I>;
        value_type read() const { return *it_; }
        void write(value_type v) const { *it_ = v; }
        I arrow() const { return it_; }
        void next() { ++it_; }
        bool equal(cursor const &that) const { return it_ == that.it_; }
    };

    static_assert(fermat::ranges::detail::output_cursor<cursor<char *>, char>);
    static_assert(fermat::ranges::detail::forward_cursor<cursor<char *> >);

    template<class I>
    using iterator = fermat::ranges::basic_iterator<cursor<I> >;
    static_assert(fermat::ranges::output_iterator<iterator<char *>, char>);
    static_assert(fermat::ranges::forward_iterator<iterator<char *> >);
    static_assert(std::is_same<std::iterator_traits<iterator<char *> >::pointer, char *>());

    void test() {
        char buf[10];
        iterator<char *> i(buf);
        *i = 'h';
        EXPECT_EQ(*i, 'h');
        EXPECT_EQ(*i, *i);
        ++i;
        *i = 'e';
        EXPECT_EQ('e', *i);
        ++i;
        *i = 'l';
        ++i;
        *i = 'l';
        ++i;
        *i = 'o';
        ++i;
        *i = '\0';
        EXPECT_EQ(std::strcmp(buf, "hello"), 0);
        EXPECT_EQ(i, iterator<char*>(buf + 5));
        ++i;
        EXPECT_NE(i, iterator<char*>(buf + 5));
        EXPECT_EQ(i, iterator<char*>(buf + 6));
    }
} // namespace test_output

// ------------------------------------------------------------
// test_move_only
// ------------------------------------------------------------
namespace test_move_only {
    struct MoveOnly {
        MoveOnly() = default;

        MoveOnly(MoveOnly &&) = default;

        MoveOnly(MoveOnly const &) = delete;

        MoveOnly &operator=(MoveOnly &&) = default;

        MoveOnly &operator=(MoveOnly const &) = delete;
    };

    template<typename I>
    struct zip1_cursor {
        I it_;

        struct mixin : fermat::ranges::basic_mixin<zip1_cursor> {
            mixin() = default;

            explicit mixin(zip1_cursor &&cur) : fermat::ranges::basic_mixin<zip1_cursor>(static_cast<zip1_cursor &&>(cur)) {
            }

            explicit mixin(zip1_cursor const &cur) : fermat::ranges::basic_mixin<zip1_cursor>(cur) {
            }

            mixin(I i) : mixin(zip1_cursor{i}) {
            }
        };

        zip1_cursor() = default;

        explicit zip1_cursor(I i) : it_(i) {
        }

        CPP_template(class J)(requires fermat::ranges::convertible_to<J, I>)
        zip1_cursor(zip1_cursor<J> that) : it_(std::move(that.it_)) {
        }

        using value_type = std::tuple<fermat::ranges::iter_value_t<I> >;
        using reference = fermat::ranges::common_tuple<fermat::ranges::iter_reference_t<I> >;
        using rvalue_reference = fermat::ranges::common_tuple<fermat::ranges::iter_rvalue_reference_t<I> >;
        reference read() const { return reference{*it_}; }
        rvalue_reference move() const { return rvalue_reference{fermat::ranges::iter_move(it_)}; }
        void write(reference const &v) const { reference{*it_} = v; }
        void write(value_type &&v) const { reference{*it_} = std::move(v); }
        void next() { ++it_; }
        bool equal(zip1_cursor const &that) const { return it_ == that.it_; }
    };

    static_assert(fermat::ranges::detail::output_cursor<zip1_cursor<MoveOnly *>, std::tuple<MoveOnly> &&>);
    static_assert(fermat::ranges::detail::forward_cursor<zip1_cursor<MoveOnly *> >);

    template<class I>
    using iterator = fermat::ranges::basic_iterator<zip1_cursor<I> >;
    static_assert(fermat::ranges::output_iterator<iterator<MoveOnly *>, std::tuple<MoveOnly> &&>);
    static_assert(fermat::ranges::forward_iterator<iterator<MoveOnly *> >);

    void test() {
        MoveOnly buf[10] = {};
        iterator<MoveOnly *> i(buf);
        *i = std::tuple<MoveOnly>{};
        fermat::ranges::common_tuple<MoveOnly &> x = *i;
        (void) x;
        std::tuple<MoveOnly> v = fermat::ranges::iter_move(i);
        *i = std::move(v);
    }
} // namespace test_move_only

// ------------------------------------------------------------
// test_forward_sized
// ------------------------------------------------------------
namespace test_forward_sized {
    template<typename I>
    struct cursor {
        I it_;

        struct mixin : fermat::ranges::basic_mixin<cursor> {
            mixin() = default;

            explicit mixin(cursor &&cur) : fermat::ranges::basic_mixin<cursor>(static_cast<cursor &&>(cur)) {
            }

            explicit mixin(cursor const &cur) : fermat::ranges::basic_mixin<cursor>(cur) {
            }

            mixin(I i) : mixin(cursor{i}) {
            }
        };

        cursor() = default;

        explicit cursor(I i) : it_(i) {
        }

        CPP_template(class J)(requires fermat::ranges::convertible_to<J, I>)
        cursor(cursor<J> that) : it_(std::move(that.it_)) {
        }

        auto read() const -> decltype(*it_) { return *it_; }
        CPP_template(class J)(requires fermat::ranges::sentinel_for<J, I>)
        bool equal(cursor<J> const &that) const { return that.it_ == it_; }

        void next() { ++it_; }
        CPP_template(class J)(requires fermat::ranges::sized_sentinel_for<J, I>)
        fermat::ranges::iter_difference_t<I> distance_to(cursor<J> const &that) const { return that.it_ - it_; }
    };

    static_assert(fermat::ranges::detail::sized_sentinel_for_cursor<cursor<char *>, cursor<char *> >);
    static_assert(fermat::ranges::detail::forward_cursor<cursor<char *> >);

    template<class I>
    using iterator = fermat::ranges::basic_iterator<cursor<I> >;
    static_assert(std::is_same<iterator<char *>::iterator_category, std::forward_iterator_tag>::value, "");

    void test() {
        using namespace fermat::ranges;
        iterator<char *> a(nullptr);
        iterator<char const *> b(nullptr);
        iterator<char const *> c(a);
        b = a;
        bool d = a == b;
        d = (a != b);
        ignore_unused(d, a < b, a <= b, a > b, a >= b, (a - b), (b - a), (a - a), (b - b));
    }
} // namespace test_forward_sized

// ------------------------------------------------------------
// test_box
// ------------------------------------------------------------
void test_box() {
    struct A : fermat::ranges::box<int> {
    };
    EXPECT_EQ(sizeof(A), sizeof(int));
    struct empty {
    };
    struct B : fermat::ranges::box<empty> {
        int i;
    };
    EXPECT_EQ(sizeof(B), sizeof(int));
    B b1, b2;
    if (fermat::ranges::detail::box_compression<empty>() == fermat::ranges::detail::box_compress::coalesce) {
        EXPECT_EQ(&b1.get(), &b2.get());
    }
    struct nontrivial {
        nontrivial() {
        }
    };
    struct C : fermat::ranges::box<nontrivial> {
        int i;
    };
    EXPECT_EQ(sizeof(C), sizeof(int));
    C c1, c2;
    EXPECT_NE(&c1.get(), &c2.get());

    {
        struct cursor {
            using value_type = int;

            cursor() {
            }

            int read() const { return 42; }

            void next() {
            }
        };
        static_assert(fermat::ranges::detail::box_compression<cursor>() == fermat::ranges::detail::box_compress::ebo);
        static_assert(fermat::ranges::same_as<int, fermat::ranges::basic_iterator<cursor>::value_type>);
    }
}

// ------------------------------------------------------------
// Google Test cases
// ------------------------------------------------------------
TEST(BasicIteratorTest, WeakInput) { test_weak_input::test(); }
TEST(BasicIteratorTest, RandomAccess) { test_random_access::test(); }
TEST(BasicIteratorTest, WeakOutput) { test_weak_output::test(); }
TEST(BasicIteratorTest, Output) { test_output::test(); }
TEST(BasicIteratorTest, MoveOnly) { test_move_only::test(); }
TEST(BasicIteratorTest, ForwardSized) { test_forward_sized::test(); }
TEST(BasicIteratorTest, Box) { test_box(); }
