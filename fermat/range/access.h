/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_RANGE_ACCESS_HPP
#define RANGES_V3_RANGE_ACCESS_HPP

#include <fermat/detail/config.h>

#include <functional> // for reference_wrapper (whose use with begin/end is deprecated)
#include <initializer_list>
#include <iterator>
#include <limits>
#include <utility>

#ifdef __has_include
#if __has_include(<span>) && !defined(RANGES_WORKAROUND_MSVC_UNUSABLE_SPAN)
#include <span>
#endif
#if __has_include(<string_view>)
#include <string_view>
#endif
#endif

#include <fermat/range_fwd.h>

#include <fermat/iterator/concepts.h>
#include <fermat/iterator/reverse_iterator.h>
#include <fermat/iterator/traits.h>
#include <fermat/utility/static_const.h>

#include <fermat/detail/prologue.h>

namespace fermat::ranges
{
#if defined(__cpp_lib_string_view) && __cpp_lib_string_view >= 201603L
    template<class CharT, class Traits>
    inline constexpr bool
        enable_borrowed_range<std::basic_string_view<CharT, Traits>> = true;
#endif

// libstdc++'s <span> header only defines std::span when concepts
// are also enabled. https://gcc.gnu.org/bugzilla/show_bug.cgi?id=97869
#if defined(__cpp_lib_span) && __cpp_lib_span >= 202002L && \
    (!defined(__GLIBCXX__) || defined(__cpp_lib_concepts))
    template<class T, std::size_t N>
    inline constexpr bool enable_borrowed_range<std::span<T, N>> = true;
#endif

    namespace detail
    {
        template<typename T>
        inline constexpr bool _borrowed_range =
            enable_borrowed_range<uncvref_t<T>>;

        template<typename T>
        inline constexpr bool _borrowed_range<T &> = true;

        template<typename T>
        T _decay_copy(T) noexcept;
    } // namespace detail

    /// \cond
    namespace _begin_
    {
        // Poison pill for std::begin. (See the detailed discussion at
        // https://github.com/ericniebler/stl2/issues/139)
        template<typename T>
        void begin(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void begin();
#endif

        template<typename T>
        void begin(std::initializer_list<T>) = delete;

        template(typename I)(
            requires input_or_output_iterator<I>)
        void is_iterator(I);

        // clang-format off
        /// \concept has_member_begin_
        /// \brief The \c has_member_begin_ concept
        template<typename T>
        CPP_requires(has_member_begin_,
            requires(T & t) //
            (
                _begin_::is_iterator(t.begin())
            ));
        /// \concept has_member_begin
        /// \brief The \c has_member_begin concept
        template<typename T>
        CPP_concept has_member_begin =
            CPP_requires_ref(_begin_::has_member_begin_, T);

        /// \concept has_non_member_begin_
        /// \brief The \c has_non_member_begin_ concept
        template<typename T>
        CPP_requires(has_non_member_begin_,
            requires(T & t) //
            (
                _begin_::is_iterator(begin(t))
            ));
        /// \concept has_non_member_begin
        /// \brief The \c has_non_member_begin concept
        template<typename T>
        CPP_concept has_non_member_begin =
            CPP_requires_ref(_begin_::has_non_member_begin_, T);
        // clang-format on

        struct fn
        {
        private:
            struct _member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(declval(R &).begin()));
            };
            struct _non_member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(begin(declval(R &))));
            };

            template<typename R>
            using _result_t =
                meta::invoke<
                    meta::conditional_t<
                        has_member_begin<R>,
                        _member_result_,
                        _non_member_result_>,
                    R>;

        public:
            template<typename R, std::size_t N>
            void operator()(R(&&)[N]) const = delete;

            template<typename R, std::size_t N>
            constexpr R * operator()(R (&array)[N]) const noexcept
            {
                return array;
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND has_member_begin<R>)
            constexpr _result_t<R> operator()(R && r) const //
                noexcept(noexcept(r.begin()))
            {
                return r.begin();
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND (!has_member_begin<R>) AND
                    has_non_member_begin<R>)
            constexpr _result_t<R> operator()(R && r) const //
                noexcept(noexcept(begin(r)))
            {
                return begin(r);
            }
        };

        template<typename R>
        using _t = decltype(fn{}(declval(R &&)));
    } // namespace _begin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return \c r, if \c r is an array. Otherwise, `r.begin()` if that expression is
    ///   well-formed and returns an input_or_output_iterator. Otherwise, `begin(r)` if
    ///   that expression returns an input_or_output_iterator.
    RANGES_DEFINE_CPO(_begin_::fn, begin)

    /// \cond
    namespace _end_
    {
        // Poison pill for std::end. (See the detailed discussion at
        // https://github.com/ericniebler/stl2/issues/139)
        template<typename T>
        void end(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void end();
#endif

        template<typename T>
        void end(std::initializer_list<T>) = delete;

        template(typename I, typename S)(
            requires sentinel_for<S, I>)
        void _is_sentinel(S, I);

        // clang-format off
        /// \concept has_member_end_
        /// \brief The \c has_member_end_ concept
        template<typename T>
        CPP_requires(has_member_end_,
            requires(T & t) //
            (
                _end_::_is_sentinel(t.end(), fermat::ranges::begin(t))
            ));
        /// \concept has_member_end
        /// \brief The \c has_member_end concept
        template<typename T>
        CPP_concept has_member_end =
            CPP_requires_ref(_end_::has_member_end_, T);

        /// \concept has_non_member_end_
        /// \brief The \c has_non_member_end_ concept
        template<typename T>
        CPP_requires(has_non_member_end_,
            requires(T & t) //
            (
                _end_::_is_sentinel(end(t), fermat::ranges::begin(t))
            ));
        /// \concept has_non_member_end
        /// \brief The \c has_non_member_end concept
        template<typename T>
        CPP_concept has_non_member_end =
            CPP_requires_ref(_end_::has_non_member_end_, T);
        // clang-format on

        struct fn
        {
        private:
            struct _member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(declval(R &).end()));
            };
            struct _non_member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(end(declval(R &))));
            };

            template<typename R>
            using _result_t =
                meta::invoke<
                    meta::conditional_t<
                        has_member_end<R>,
                        _member_result_,
                        _non_member_result_>,
                    R>;

            template<typename Int>
            using iter_diff_t =
                meta::_t<meta::conditional_t<std::is_integral<Int>::value,
                                             std::make_signed<Int>, //
                                             meta::id<Int>>>;

        public:
            template<typename R, std::size_t N>
            void operator()(R(&&)[N]) const = delete;

            template<typename R, std::size_t N>
            constexpr R * operator()(R (&array)[N]) const noexcept
            {
                return array + N;
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND has_member_end<R>)
            constexpr _result_t<R> operator()(R && r) const //
                noexcept(noexcept(r.end()))
            {
                return r.end();
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND (!has_member_end<R>) AND
                    has_non_member_end<R>)
            constexpr _result_t<R> operator()(R && r) const //
                noexcept(noexcept(end(r)))
            {
                return end(r);
            }

            template(typename Int)(
                requires detail::integer_like_<Int>)
            auto operator-(Int dist) const
                -> detail::from_end_<iter_diff_t<Int>>
            {
                using SInt = iter_diff_t<Int>;
                RANGES_EXPECT(0 <= dist);
                RANGES_EXPECT(dist <=
                              static_cast<Int>((std::numeric_limits<SInt>::max)()));
                return detail::from_end_<SInt>{-static_cast<SInt>(dist)};
            }
        };

        template<typename R>
        using _t = decltype(fn{}(declval(R &&)));
    } // namespace _end_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return \c r+size(r), if \c r is an array. Otherwise, `r.end()` if that expression
    /// is
    ///   well-formed and returns an input_or_output_iterator. Otherwise, `end(r)` if that
    ///   expression returns an input_or_output_iterator.
    RANGES_DEFINE_CPO(_end_::fn, end)

    /// \cond
    namespace _cbegin_
    {
        struct fn
        {
            template<typename T, std::size_t N>
            void operator()(T(&&)[N]) const = delete;

            template<typename R>
            constexpr _begin_::_t<detail::as_const_t<R>> operator()(R && r) const
                noexcept(noexcept(fermat::ranges::begin(detail::as_const(r))))
            {
                return fermat::ranges::begin(detail::as_const(r));
            }
        };
    } // namespace _cbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `fermat::ranges::begin` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_cbegin_::fn, cbegin)

    /// \cond
    namespace _cend_
    {
        struct fn
        {
            template<typename T, std::size_t N>
            void operator()(T(&&)[N]) const = delete;

            template<typename R>
            constexpr _end_::_t<detail::as_const_t<R>> operator()(R && r) const
                noexcept(noexcept(fermat::ranges::end(detail::as_const(r))))
            {
                return fermat::ranges::end(detail::as_const(r));
            }
        };
    } // namespace _cend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `fermat::ranges::end` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_cend_::fn, cend)

    /// \cond
    namespace _rbegin_
    {
        template<typename R>
        void rbegin(R &&) = delete;
        // Non-standard, to keep unqualified rbegin(r) from finding std::rbegin
        // and returning a std::reverse_iterator.
        template<typename T>
        void rbegin(std::initializer_list<T>) = delete;
        template<typename T, std::size_t N>
        void rbegin(T (&)[N]) = delete;

        // clang-format off
        /// \concept has_member_rbegin_
        /// \brief The \c has_member_rbegin_ concept
        template<typename T>
        CPP_requires(has_member_rbegin_,
            requires(T & t) //
            (
                _begin_::is_iterator(t.rbegin())
            ));
        /// \concept has_member_rbegin
        /// \brief The \c has_member_rbegin concept
        template<typename T>
        CPP_concept has_member_rbegin =
            CPP_requires_ref(_rbegin_::has_member_rbegin_, T);

        /// \concept has_non_member_rbegin_
        /// \brief The \c has_non_member_rbegin_ concept
        template<typename T>
        CPP_requires(has_non_member_rbegin_,
            requires(T & t) //
            (
                _begin_::is_iterator(rbegin(t))
            ));
        /// \concept has_non_member_rbegin
        /// \brief The \c has_non_member_rbegin concept
        template<typename T>
        CPP_concept has_non_member_rbegin =
            CPP_requires_ref(_rbegin_::has_non_member_rbegin_, T);

        template<typename I>
        void _same_type(I, I);

        /// \concept can_reverse_end_
        /// \brief The \c can_reverse_end_ concept
        template<typename T>
        CPP_requires(can_reverse_end_,
            requires(T & t) //
            (
                // make_reverse_iterator is constrained with
                // bidirectional_iterator.
                fermat::ranges::make_reverse_iterator(fermat::ranges::end(t)),
                _rbegin_::_same_type(fermat::ranges::begin(t), fermat::ranges::end(t))
            ));
        /// \concept can_reverse_end
        /// \brief The \c can_reverse_end concept
        template<typename T>
        CPP_concept can_reverse_end =
            CPP_requires_ref(_rbegin_::can_reverse_end_, T);
        // clang-format on

        struct fn
        {
        private:
            struct _member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(declval(R &).rbegin()));
            };
            struct _non_member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(rbegin(declval(R &))));
            };
            struct _reverse_result_
            {
                template<typename R>
                using invoke =
                    decltype(fermat::ranges::make_reverse_iterator(fermat::ranges::end(declval(R &))));
            };
            struct _other_result_
            {
                template<typename R>
                using invoke =
                    meta::invoke<
                        meta::conditional_t<
                            has_non_member_rbegin<R>,
                            _non_member_result_,
                            _reverse_result_>,
                        R>;
            };

            template<typename R>
            using _result_t =
                meta::invoke<
                    meta::conditional_t<
                        has_member_rbegin<R>,
                        _member_result_,
                        _other_result_>,
                    R>;

        public:
            template(typename R)(
                requires detail::_borrowed_range<R> AND has_member_rbegin<R>)
            constexpr auto operator()(R && r) const //
                noexcept(noexcept(r.rbegin())) //
            {
                return r.rbegin();
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND (!has_member_rbegin<R>) AND
                    has_non_member_rbegin<R>)
            constexpr auto operator()(R && r) const //
                noexcept(noexcept(rbegin(r))) //
            {
                return rbegin(r);
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND (!has_member_rbegin<R>) AND
                    (!has_non_member_rbegin<R>) AND can_reverse_end<R>)
            constexpr auto operator()(R && r) const //
                noexcept(noexcept(fermat::ranges::make_reverse_iterator(fermat::ranges::end(r))))
            {
                return fermat::ranges::make_reverse_iterator(fermat::ranges::end(r));
            }
        };

        template<typename R>
        using _t = decltype(fn{}(declval(R &&)));
    } // namespace _rbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return `make_reverse_iterator(r + fermat::ranges::size(r))` if r is an array. Otherwise,
    ///   `r.rbegin()` if that expression is well-formed and returns an
    ///   input_or_output_iterator. Otherwise, `make_reverse_iterator(fermat::ranges::end(r))` if
    ///   `fermat::ranges::begin(r)` and `fermat::ranges::end(r)` are both well-formed and have the same
    ///   type that satisfies `bidirectional_iterator`.
    RANGES_DEFINE_CPO(_rbegin_::fn, rbegin)

    /// \cond
    namespace _rend_
    {
        template<typename R>
        void rend(R &&) = delete;
        // Non-standard, to keep unqualified rend(r) from finding std::rend
        // and returning a std::reverse_iterator.
        template<typename T>
        void rend(std::initializer_list<T>) = delete;
        template<typename T, std::size_t N>
        void rend(T (&)[N]) = delete;

        // clang-format off
        /// \concept has_member_rend_
        /// \brief The \c has_member_rend_ concept
        template<typename T>
        CPP_requires(has_member_rend_,
            requires(T & t) //
            (
                _end_::_is_sentinel(t.rend(), fermat::ranges::rbegin(t))
            ));
        /// \concept has_member_rend
        /// \brief The \c has_member_rend concept
        template<typename T>
        CPP_concept has_member_rend =
            CPP_requires_ref(_rend_::has_member_rend_, T);

        /// \concept has_non_member_rend_
        /// \brief The \c has_non_member_rend_ concept
        template<typename T>
        CPP_requires(has_non_member_rend_,
            requires(T & t) //
            (
                _end_::_is_sentinel(rend(t), fermat::ranges::rbegin(t))
            ));
        /// \concept has_non_member_rend
        /// \brief The \c has_non_member_rend concept
        template<typename T>
        CPP_concept has_non_member_rend =
            CPP_requires_ref(_rend_::has_non_member_rend_, T);

        /// \concept can_reverse_begin_
        /// \brief The \c can_reverse_begin_ concept
        template<typename T>
        CPP_requires(can_reverse_begin_,
            requires(T & t) //
            (
                // make_reverse_iterator is constrained with
                // bidirectional_iterator.
                fermat::ranges::make_reverse_iterator(fermat::ranges::begin(t)),
                _rbegin_::_same_type(fermat::ranges::begin(t), fermat::ranges::end(t))
            ));
        /// \concept can_reverse_begin
        /// \brief The \c can_reverse_begin concept
        template<typename T>
        CPP_concept can_reverse_begin =
            CPP_requires_ref(_rend_::can_reverse_begin_, T);
        // clang-format on

        struct fn
        {
        private:
            struct _member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(declval(R &).rend()));
            };
            struct _non_member_result_
            {
                template<typename R>
                using invoke = decltype(detail::_decay_copy(rend(declval(R &))));
            };
            struct _reverse_result_
            {
                template<typename R>
                using invoke =
                    decltype(fermat::ranges::make_reverse_iterator(fermat::ranges::begin(declval(R &))));
            };
            struct _other_result_
            {
                template<typename R>
                using invoke =
                    meta::invoke<
                        meta::conditional_t<
                            has_non_member_rend<R>,
                            _non_member_result_,
                            _reverse_result_>,
                        R>;
            };

            template<typename R>
            using _result_t =
                meta::invoke<
                    meta::conditional_t<
                        has_member_rend<R>,
                        _member_result_,
                        _other_result_>,
                    R>;

        public:
            template(typename R)(
                requires detail::_borrowed_range<R> AND has_member_rend<R>)
            constexpr auto operator()(R && r) const //
                noexcept(noexcept(r.rend())) //
            {
                return r.rend();
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND (!has_member_rend<R>) AND
                    has_non_member_rend<R>)
            constexpr auto operator()(R && r) const //
                noexcept(noexcept(rend(r))) //
            {
                return rend(r);
            }

            template(typename R)(
                requires detail::_borrowed_range<R> AND (!has_member_rend<R>) AND
                    (!has_non_member_rend<R>) AND can_reverse_begin<R>)
            constexpr auto operator()(R && r) const //
                noexcept(noexcept(fermat::ranges::make_reverse_iterator(fermat::ranges::begin(r))))
            {
                return fermat::ranges::make_reverse_iterator(fermat::ranges::begin(r));
            }
        };

        template<typename R>
        using _t = decltype(fn{}(declval(R &&)));
    } // namespace _rend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return `make_reverse_iterator(r)` if `r` is an array. Otherwise,
    ///   `r.rend()` if that expression is well-formed and returns a type that
    ///   satisfies `sentinel_for<S, I>` where `I` is the type of `fermat::ranges::rbegin(r)`.
    ///   Otherwise, `make_reverse_iterator(fermat::ranges::begin(r))` if `fermat::ranges::begin(r)`
    ///   and `fermat::ranges::end(r)` are both well-formed and have the same type that
    ///   satisfies `bidirectional_iterator`.
    RANGES_DEFINE_CPO(_rend_::fn, rend)

    /// \cond
    namespace _crbegin_
    {
        struct fn
        {
            template<typename T, std::size_t N>
            void operator()(T(&&)[N]) const = delete;

            template<typename R>
            constexpr _rbegin_::_t<detail::as_const_t<R>> operator()(R && r) const
                noexcept(noexcept(fermat::ranges::rbegin(detail::as_const(r))))
            {
                return fermat::ranges::rbegin(detail::as_const(r));
            }
        };
    } // namespace _crbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `fermat::ranges::rbegin` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_crbegin_::fn, crbegin)

    /// \cond
    namespace _crend_
    {
        struct fn
        {
            template<typename T, std::size_t N>
            void operator()(T(&&)[N]) const = delete;

            template<typename R>
            constexpr _rend_::_t<detail::as_const_t<R>> operator()(R && r) const
                noexcept(noexcept(fermat::ranges::rend(detail::as_const(r))))
            {
                return fermat::ranges::rend(detail::as_const(r));
            }
        };
    } // namespace _crend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `fermat::ranges::rend` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_crend_::fn, crend)

    template<typename Rng>
    using iterator_t = decltype(begin(declval(Rng &)));

    template<typename Rng>
    using sentinel_t = decltype(end(declval(Rng &)));

    namespace cpp20
    {
        using fermat::ranges::begin;
        using fermat::ranges::cbegin;
        using fermat::ranges::cend;
        using fermat::ranges::crbegin;
        using fermat::ranges::crend;
        using fermat::ranges::end;
        using fermat::ranges::rbegin;
        using fermat::ranges::rend;

        using fermat::ranges::iterator_t;
        using fermat::ranges::sentinel_t;

        using fermat::ranges::enable_borrowed_range;
    } // namespace cpp20
} // namespace fermat::ranges

#include <fermat/detail/epilogue.h>

#endif
