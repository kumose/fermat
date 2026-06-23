// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <fermat/container/internal/special_member_functions_expected.h>
#include <fermat/container/construct.h>
#include <fermat/container/traits.h>
#include <memory>
#include <type_traits>
#include <utility>

#include <initializer_list> // for std::initializer_list

namespace fermat {
    template<class T, class E>
    class expected;

    template<class E>
    class unexpected;

    // Some helper type traits:
    namespace internal {
        // TODO: move this somewhere else? It doesn't handle
        // templates with non-type template parameters so it isn't
        // really generic...
        template<class T, template <class...> class Template>
        struct is_specialization : std::false_type {
        };

        template<template <class...> class Template, class... Args>
        struct is_specialization<Template<Args...>, Template> : std::true_type {
        };

        // Used in the SFINAE expression for a constructor in the expected class.
        template<class T, class U>
        static constexpr bool converts_from_any_cvref_v =
                std::is_constructible_v<T, U &> || std::is_convertible_v<U &, T> || std::is_constructible_v<T, U> ||
                std::is_convertible_v<U, T> || std::is_constructible_v<T, const U> || std::is_convertible_v<const U, T>
                ||
                std::is_constructible_v<T, const U &> || std::is_convertible_v<const U &, T>;

        template<class T, class E, class U>
        static constexpr bool generic_constructor_constraint_v =
                !std::is_same_v<remove_cvref_t<U>, std::in_place_t> && !std::is_same_v<expected<T, E>, remove_cvref_t<
                    U> > &&
                !internal::is_specialization<remove_cvref_t<U>, unexpected>::value && std::is_constructible_v<T, U> &&
                (!std::is_same_v<std::remove_cv<T>, bool> || !internal::is_specialization<remove_cvref_t<U>,
                     expected>::value);

        template<class T, class E, class U, class G, class UF, class GF>
        static constexpr bool expected_to_expected_ctor_constraint_v =
                std::is_constructible_v<T, UF> && std::is_constructible_v<E, GF> &&
                !std::is_constructible_v<unexpected<E>, expected<U, G> &> && !std::is_constructible_v<unexpected<E>,
                    expected<U, G> > &&
                !std::is_constructible_v<unexpected<E>, const expected<U, G> &> &&
                !std::is_constructible_v<unexpected<E>, const expected<U, G>> &&
                (!std::is_same_v<std::remove_cv<T>, bool> || !internal::converts_from_any_cvref_v<T, expected<U, G> >);
    } // namespace internal

    template<class E>
    class unexpected {
    public:
        // constructors
        constexpr unexpected(const unexpected &) = default;

        constexpr unexpected(unexpected &&) = default;

        template<class Err,
            typename = std::enable_if_t<!std::is_same_v<remove_cvref_t<Err>, unexpected<E> > &&
                                        !std::is_same_v<remove_cvref_t<Err>, std::in_place_t> && std::is_constructible_v
                                        <E, Err>> >
        constexpr explicit unexpected(Err &&e) : _error(std::forward<Err>(e)) {
        };

        template<class... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
        constexpr explicit unexpected(std::in_place_t, Args &&... args) : _error(std::forward<Args>(args)...) {
        };

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit unexpected(std::in_place_t, std::initializer_list<U> il, Args &&... args)
            : _error(il, std::forward<Args>(args)...) {
        };

        constexpr unexpected &operator=(const unexpected &) = default;

        constexpr unexpected &operator=(unexpected &&) = default;

        constexpr const E &error() const & noexcept { return _error; };
        constexpr E &error() & noexcept { return _error; };
        constexpr const E &&error() const && noexcept { return std::move(_error); };
        constexpr E &&error() && noexcept { return std::move(_error); };

        constexpr void swap(unexpected &other) noexcept(std::is_nothrow_swappable_v<E>) {
            static_assert(std::is_swappable_v<E>, "unexpected<E> swap requires E to be swappable");
            using std::swap;
            swap(_error, other._error);
        };

        friend constexpr void swap(unexpected &x, unexpected &y) noexcept(noexcept(x.swap(y))) {
            static_assert(std::is_swappable_v<E>, "unexpected<E> swap requires E to be swappable");
            x.swap(y);
        }

        // equality operator
        template<class E2>
        friend constexpr bool operator==(const unexpected &x, const unexpected<E2> &y) {
            return x._error == y._error;
        }

    private:
        E _error;

        // The standard specifies these are ill-formed.
        static_assert(std::is_object_v<E> && !std::is_array_v<E> && !std::is_const_v<E> && !std::is_volatile_v<E> &&
                      !internal::is_specialization<E, unexpected>::value,
                      "This type is not supported by a conforming implementation of unexpected.");
    };

    template<class E>
    unexpected(E) -> unexpected<E>;


    // in-place construction of unexpected values
    struct unexpect_t {
        explicit unexpect_t() = default;
    };

    inline constexpr unexpect_t unexpect{};

    namespace internal {
        ///////
        // ExpectedDestructLayer handles the triviality of the destructor
        //
        // The general case when both T and E are trivially destructible.
        template<class T, class E, bool = std::is_trivially_destructible_v<T> && std::is_trivially_destructible_v<E> >
        struct ExpectedDestructLayer {
            // Note: we deliberately don't initialize anything here, member initailization for
            // the default conxtructoris done in the `expected` class.
            constexpr ExpectedDestructLayer() {
            };

            union {
                T _value;
                E _error;
            };

            bool _has_value;
        };

        //
        // The case where one of T or E is not trivially destructible.
        template<class T, class E>
        struct ExpectedDestructLayer<T, E, false> {
            ~ExpectedDestructLayer() {
                if (_has_value) {
                    fermat::destroy_at(&_value);
                } else {
                    fermat::destroy_at(&_error);
                }
            }

            // Note: we deliberately don't initialize anything here, member initailization for
            // the default conxtructoris done in the `expected` class.
            constexpr ExpectedDestructLayer() {
            };

            union {
                T _value;
                E _error;
            };

            bool _has_value;
        };


        ///////
        // ExpectedConstructLayer handles the implemenation of the copy/move constructor/assignment
        //
        template<class T, class E>
        struct ExpectedConstructLayer : ExpectedDestructLayer<T, E> {
            using ExpectedDestructLayer<T, E>::ExpectedDestructLayer;

            void ConstructFrom(const ExpectedConstructLayer &other) {
                this->_has_value = other._has_value;
                if (this->_has_value) {
                    fermat::construct_at(std::addressof(this->_value), other._value);
                } else {
                    fermat::construct_at(std::addressof(this->_error), other._error);
                }
            }

            void ConstructFrom(ExpectedConstructLayer &&other) {
                this->_has_value = other._has_value;
                if (this->_has_value) {
                    fermat::construct_at(std::addressof(this->_value), std::move(other._value));
                } else {
                    fermat::construct_at(std::addressof(this->_error), std::move(other._error));
                }
            }

            void AssignFrom(const ExpectedConstructLayer &other) {
                if (this->_has_value && other._has_value) {
                    this->_value = other._value;
                } else if (this->_has_value) {
                    ReInit(this->_error, this->_value, other._error);
                    this->_has_value = false;
                } else if (other._has_value) {
                    ReInit(this->_value, this->_error, other._value);
                    this->_has_value = true;
                } else {
                    this->_error = other._error;
                }
            }

            void AssignFrom(ExpectedConstructLayer &&other) {
                if (this->_has_value && other._has_value) {
                    this->_value = std::move(other._value);
                } else if (this->_has_value) {
                    ReInit(this->_error, this->_value, std::move(other._error));
                    this->_has_value = false;
                } else if (other._has_value) {
                    ReInit(this->_value, this->_error, std::move(other._value));
                    this->_has_value = true;
                } else {
                    this->_error = std::move(other._error);
                }
            }


            template<class NewVal, class OldVal, class... Args>
            void ReInit(NewVal &newval, OldVal &oldval, Args &&... args) {
                fermat::destroy_at(&oldval);
                fermat::construct_at(&newval, std::forward<Args>(args)...);
            }
        };
    } // namespace internal

    // TODO: we've marked member functions and constructors as constexpr when the standard
    // dictates it, but in reality a lot of these functions do now work at constant evaluation
    // time becuase they use facilities like `std::addressof` and `construct_at` which
    // are currently not constexpr.
    template<class T, class E>
    class expected : internal::EnableExpectedSpecialMemberFunctions<internal::ExpectedConstructLayer<T, E>, T, E> {
    private:
        using LayeredBase = internal::EnableExpectedSpecialMemberFunctions<internal::ExpectedConstructLayer<T, E>, T, E>
        ;

    public:
        using value_type = T;
        using error_type = E;
        using unexpected_type = unexpected<E>;


        template<class U>
        using rebind = expected<U, error_type>;

        template<bool Requires = std::is_default_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr expected() {
            this->_has_value = true;
            fermat::construct_at(std::addressof(this->_value));
        };

        // non-explicit version for when std::is_convertible_v<U, T> is true.
        template<class U,
            std::enable_if_t<std::is_convertible_v<U, T> && internal::generic_constructor_constraint_v<T, E, U>, int> =
                    0>
        constexpr expected(U &&v) {
            this->_has_value = true;
            fermat::construct_at(std::addressof(this->_value), std::forward<U>(v));
        }

        // explicit version for when std::is_convertible_v<U, T> is false
        template<class U,
            std::enable_if_t<!std::is_convertible_v<U, T> && internal::generic_constructor_constraint_v<T, E, U>, int> =
                    0>
        constexpr explicit expected(U &&v) {
            this->_has_value = true;
            fermat::construct_at(std::addressof(this->_value), std::forward<U>(v));
        }

        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, const T1 &, const E1 &> &&
                             (!std::is_convertible_v<const T1 &, T> || !std::is_convertible_v<const E1 &, E>),
                int> = 0>
        constexpr explicit expected(const expected<T1, E1> &other) {
            this->_has_value = other.has_value();
            if (this->_has_value) {
                fermat::construct_at(std::addressof(this->_value), other.value());
            } else {
                fermat::construct_at(std::addressof(this->_error), other.error());
            }
        }

        // Same as above except this is implicit when std::is_convertible_v<const T1&, T> && std::is_convertible_v<const E1&, E>.
        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, const T1 &, const E1 &> &&
                             (std::is_convertible_v<const T1 &, T> && std::is_convertible_v<const E1 &, E>),
                int> = 0>
        constexpr expected(const expected<T1, E1> &other) {
            this->_has_value = other.has_value();
            if (this->_has_value) {
                fermat::construct_at(std::addressof(this->_value), other.value());
            } else {
                fermat::construct_at(std::addressof(this->_error), other.error());
            }
        }

        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, T1, E1> &&
                             (!std::is_convertible_v<T1, T> || !std::is_convertible_v<E1, E>),
                int> = 0>
        constexpr explicit expected(expected<T1, E1> &&other) {
            this->_has_value = other.has_value();
            if (this->_has_value) {
                fermat::construct_at(std::addressof(this->_value), std::move(other).value());
            } else {
                fermat::construct_at(std::addressof(this->_error), std::move(other).error());
            }
        }

        // Same as above except this is implicit when (std::is_convertible_v<T1, T> && std::is_convertible_v<E1, E>)
        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, T1, E1> &&
                             (std::is_convertible_v<T1, T> && std::is_convertible_v<E1, E>),
                int> = 0>
        constexpr expected(expected<T1, E1> &&other) {
            this->_has_value = other.has_value();
            if (this->_has_value) {
                fermat::construct_at(std::addressof(this->_value), std::move(other).value());
            } else {
                fermat::construct_at(std::addressof(this->_error), std::move(other).error());
            }
        }


        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && !std::is_convertible_v<const G &, E>
            , int> = 0>
        constexpr explicit expected(const unexpected<G> &unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), unex.error());
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && std::is_convertible_v<const G &, E>,
            int> = 0>
        constexpr expected(const unexpected<G> &unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), unex.error());
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && !std::is_convertible_v<G, E>, int> = 0>
        constexpr explicit expected(unexpected<G> &&unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), std::move(unex.error()));
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && std::is_convertible_v<G, E>, int> = 0>
        constexpr expected(unexpected<G> &&unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), std::move(unex.error()));
        }

        template<class... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
        constexpr explicit expected(std::in_place_t, Args &&... args) {
            this->_has_value = true;
            fermat::construct_at(std::addressof(this->_value), std::forward<Args>(args)...);
        }

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit expected(std::in_place_t, std::initializer_list<U> il, Args &&... args) {
            this->_has_value = true;
            fermat::construct_at(std::addressof(this->_value), il, std::forward<Args>(args)...);
        }

        template<class... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, Args &&... args) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), std::forward<Args>(args)...);
        }

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&... args) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), il, std::forward<Args>(args)...);
        }

        // copy/move assignments are done by means of ExpectedConstructLayer::AssignFrom, and the
        // special function layers the assignments are deleted when they should be.
        //
        // constexpr expected& operator=(const expected&);
        // constexpr expected& operator=(expected&&) noexcept(/* see description */);
        //
        ///////

        // Note: The default template parameter is in the standard, the nothrow constraints are
        // also in the standard.
        template<class U = T,
            std::enable_if_t<!std::is_same_v<expected, remove_cvref_t<U> > &&
                             !internal::is_specialization<remove_cvref_t<U>, unexpected>::value &&
                             std::is_constructible_v<T, U> && std::is_assignable_v<T &, U> &&
                             (std::is_nothrow_constructible_v<T, U> || std::is_nothrow_move_constructible_v<T> ||
                              std::is_nothrow_move_constructible_v<E>),
                int> = 0>
        constexpr expected &operator=(U &&x) {
            if (this->_has_value) {
                this->_value = std::forward<U>(x);
            } else {
                this->ReInit(this->_value, this->_error, std::forward<U>(x));
                this->_has_value = true;
            }
            return *this;
        }

        template<class G,
            std::enable_if_t<std::is_constructible_v<E, const G &> && std::is_assignable_v<E &, const G &> &&
                             (std::is_nothrow_constructible_v<E, const G &> || std::is_nothrow_move_constructible_v<T>
                              ||
                              std::is_nothrow_move_constructible_v<E>),
                int> = 0>
        constexpr expected &operator=(const unexpected<G> &unex) {
            if (this->_has_value) {
                this->ReInit(this->_error, this->_value, unex.error());
                this->_has_value = false;
            } else {
                this->_error = unex.error();
            }
            return *this;
        }

        template<class G,
            std::enable_if_t<std::is_constructible_v<E, G> && std::is_assignable_v<E &, G> &&
                             (std::is_nothrow_constructible_v<E, G> || std::is_nothrow_move_constructible_v<T> ||
                              std::is_nothrow_move_constructible_v<E>),
                int> = 0>
        constexpr expected &operator=(unexpected<G> &&unex) {
            if (this->_has_value) {
                this->ReInit(this->_error, this->_value, std::move(unex).error());
                this->_has_value = false;
            } else {
                this->_error = std::move(unex).error();
            }
            return *this;
        }

        // Note: this only works if the constructor is noexcept, kind of strict but that's what the standard dictates...
        template<class... Args, std::enable_if_t<std::is_nothrow_constructible_v<T, Args...>, int> = 0>
        constexpr T &emplace(Args &&... args) noexcept {
            if (this->_has_value) {
                fermat::destroy_at(&this->_value);
            } else {
                fermat::destroy_at(&this->_error);
                this->_has_value = true;
            }
            return *fermat::construct_at(std::addressof(this->_value), std::forward<Args>(args)...);
        }

        // Note: this only works if the constructor is noexcept, kind of strict but that's what the standard dictates...
        template<class U,
            class... Args,
            std::enable_if_t<std::is_nothrow_constructible_v<T, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr T &emplace(std::initializer_list<U> il, Args &&... args) noexcept {
            if (this->_has_value) {
                fermat::destroy_at(&this->_value);
            } else {
                fermat::destroy_at(&this->_error);
                this->_has_value = true;
            }
            return *fermat::construct_at(std::addressof(this->_value), il, std::forward<Args>(args)...);
        }

        // swap
        template<bool Requires = std::is_swappable_v<T> && std::is_swappable_v<E> && std::is_move_constructible_v<T> &&
                                 std::is_move_constructible_v<E> &&
                                 (std::is_nothrow_move_assignable_v<E> || std::is_nothrow_move_assignable_v<T>),
            std::enable_if_t<Requires, int> = 0,
            bool NoExcept = std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T> &&
                            std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E> >
        void swap(expected &other) noexcept(NoExcept) {
            using std::swap;
            if (other._has_value) {
                if (this->_has_value) {
                    swap(this->_value, other._value);
                } else {
                    other.swap(*this);
                }
            } else // other._has_value is false
            {
                if (!this->_has_value) {
                    swap(this->_error, other._error);
                } else // `other` has an error and `this` has a value, we need to swap them around.
                {
                    // Note that std::is_nothrow_swappable_v implies the destructors cannot throw.
                    // The definition of NoExcept implies the constructions here cannot throw.
                    // So notheng here throws.
                    E tmp(std::move(other._error));
                    fermat::destroy_at(&other._error);
                    fermat::construct_at(std::addressof(other._value), std::move(this->_value));
                    fermat::destroy_at(&this->_value);
                    fermat::construct_at(std::addressof(this->_error), std::move(tmp));
                    this->_has_value = false;
                    other._has_value = true;
                }
            }
        }

        friend constexpr void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

        // These all assume has_value() is true. Otherwise, calling them is UB (as per the
        // standard).  When asserts are enabled, we've decided to assert the precondition
        // similar to what would be done in a hardened library implementation.
        constexpr const T *operator->() const noexcept {
            KCHECK(has_value()) <<
			                 "Pre-condition failed! Accessing an expected value while containing an error.";
            return std::addressof(this->_value);
        }

        constexpr T *operator->() noexcept {
            KCHECK(has_value()) <<
			                 "Pre-condition failed! Accessing an expected value while containing an error.";
            return std::addressof(this->_value);
        }

        constexpr const T &operator*() const & noexcept {
            KCHECK(has_value()) <<
			                 "Pre-condition failed! Accessing an expected value while containing an error.";
            return this->_value;
        }

        constexpr T &operator*() & noexcept {
            KCHECK(has_value()) <<
			                 "Pre-condition failed! Accessing an expected value while containing an error.";
            return this->_value;
        }

        constexpr const T &&operator*() const && noexcept {
            KCHECK(has_value()) << "Pre-condition failed! Accessing an expected value while containing an error.";
            return std::move(this->_value);
        }

        constexpr T &&operator*() && noexcept {
            KCHECK(has_value()) <<
			                 "Pre-condition failed! Accessing an expected value while containing an error.";
            return std::move(this->_value);
        }

        constexpr explicit operator bool() const noexcept { return this->_has_value; }
        constexpr bool has_value() const noexcept { return this->_has_value; };

        constexpr const T &value() const & {
            if (!has_value()) {
                KCHECK(false) << "Calling `value()` when expected contains no value.";
            }
            return this->_value;
        }

        constexpr T &value() & {
            if (!has_value()) {
                KCHECK(false) << "Calling `value()` when expected contains no value.";
            }
            return this->_value;
        }

        constexpr const T &&value() const && {
            if (!has_value()) {
                KCHECK(false) << "Calling `value()` when expected contains no value.";
            }
            return std::move(this->_value);
        }

        constexpr T &&value() && {
            if (!has_value()) {
                KCHECK(false) << "Calling `value()` when expected contains no value.";
            }
            return std::move(this->_value);
        }

        // These all assume has_value() is false. Otherwise, calling them is UB (as per the
        // standard).  When asserts are enabled, we've decided to assert the precondition
        // similar to what would be done in a hardened library implementation.
        constexpr const E &error() const & {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return this->_error;
        };

        constexpr E &error() & {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return this->_error;
        };

        constexpr const E &&error() const && {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return std::move(this->_error);
        };

        constexpr E &&error() && {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return std::move(this->_error);
        };

        template<class U>
        constexpr T value_or(U &&alt) const & {
            static_assert(std::is_copy_constructible_v<T> && std::is_convertible_v<U, T>);
            return has_value() ? this->_value : static_cast<T>(std::forward<U>(alt));
        }

        template<class U>
        constexpr T value_or(U &&alt) && {
            static_assert(std::is_move_constructible_v<T> && std::is_convertible_v<U, T>);
            return has_value() ? std::move(this->_value) : static_cast<T>(std::forward<U>(alt));
        }

        template<class U>
        constexpr E error_or(U &&alt) const & {
            static_assert(std::is_copy_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return this->_error;
        }

        template<class U>
        constexpr E error_or(U &&alt) && {
            static_assert(std::is_move_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return std::move(this->_error);
        }

        // Note: the constraint in the standard is std::is_constructible_v<E, decltype(error())>
        // here and std::is_constructible_v<E, decltype(std::move(error()))> in the && qualified
        // versions, we're just explicitly spellig the decltype in our implementations since we
        // can't put the member call in the template argument. declval doesn't really help us
        // much since it always returns an rvalue reference, and `expected` is an incomplete
        // type at this point.
        template<class F, bool Requires = std::is_constructible_v<E, E &>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) & {
            using U = remove_cvref_t<std::invoke_result_t<F, decltype(value())> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f), value());
            }
            return U(unexpect, error());
        }

        // See note about constraint above.
        template<class F, bool Requires = std::is_constructible_v<E, const E &>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) const & {
            using U = remove_cvref_t<std::invoke_result_t<F, decltype(value())> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f), value());
            }
            return U(unexpect, error());
        }

        // See note about constraint above.
        template<class F, bool Requires = std::is_constructible_v<E, E &&>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) && {
            using U = remove_cvref_t<std::invoke_result_t<F, decltype(std::move(value()))> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f), std::move(value()));
            }
            return U(unexpect, std::move(error()));
        }

        // See note about constraint above.
        template<class F, bool Requires = std::is_constructible_v<E, const E &&>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) const && {
            using U = remove_cvref_t<std::invoke_result_t<F, decltype(std::move(value()))> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f), std::move(value()));
            }
            return U(unexpect, std::move(error()));
        }

        template<class F, bool Requires = std::is_copy_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto or_else(F &&f) & {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(error())> >;
            static_assert(std::is_same_v<typename G::value_type, T> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G(std::in_place, value());
            }
            return std::invoke(std::forward<F>(f), error());
        }

        template<class F, bool Requires = std::is_copy_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto or_else(F &&f) const & {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(error())> >;
            static_assert(std::is_same_v<typename G::value_type, T> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G(std::in_place, value());
            }
            return std::invoke(std::forward<F>(f), error());
        }

        template<class F, bool Requires = std::is_move_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto or_else(F &&f) && {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            static_assert(std::is_same_v<typename G::value_type, T> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G(std::in_place, std::move(value()));
            }
            return std::invoke(std::forward<F>(f), std::move(error()));
        }

        template<class F, bool Requires = std::is_move_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto or_else(F &&f) const && {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            static_assert(std::is_same_v<typename G::value_type, T> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G(std::in_place, std::move(value()));
            }
            return std::invoke(std::forward<F>(f), std::move(error()));
        }

        template<class F, bool Requires = std::is_copy_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) & {
            using U = std::remove_cv_t<std::invoke_result_t<F, decltype(value())> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, error());
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f), value());
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f), value()));
            }
        }

        template<class F, bool Requires = std::is_copy_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) const & {
            using U = std::remove_cv_t<std::invoke_result_t<F, decltype(value())> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, error());
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f), value());
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f), value()));
            }
        }

        template<class F, bool Requires = std::is_move_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) && {
            using U = std::remove_cv_t<std::invoke_result_t<F, decltype(std::move(value()))> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, std::move(error()));
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f), std::move(value()));
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f), std::move(value())));
            }
        }

        template<class F, bool Requires = std::is_move_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) const && {
            using U = std::remove_cv_t<std::invoke_result_t<F, decltype(std::move(value()))> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, std::move(error()));
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f), std::move(value()));
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f), std::move(value())));
            }
        }

        template<class F, bool Requires = std::is_copy_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform_error(F &&f) & {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(error())> >;
            if (has_value()) {
                return expected<T, G>(std::in_place, value());
            } else {
                return expected<T, G>(unexpect, std::invoke(std::forward<F>(f), error()));
            }
        }

        template<class F, bool Requires = std::is_copy_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform_error(F &&f) const & {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(error())> >;
            if (has_value()) {
                return expected<T, G>(std::in_place, value());
            } else {
                return expected<T, G>(unexpect, std::invoke(std::forward<F>(f), error()));
            }
        }

        template<class F, bool Requires = std::is_move_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform_error(F &&f) && {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            if (has_value()) {
                return expected<T, G>(std::in_place, std::move(value()));
            } else {
                return expected<T, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
            }
        }

        template<class F, bool Requires = std::is_move_constructible_v<T>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform_error(F &&f) const && {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            if (has_value()) {
                return expected<T, G>(std::in_place, std::move(value()));
            } else {
                return expected<T, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
            }
        }

        // equality operators
        template<class T2, class E2>
        friend constexpr bool operator==(const expected &x, const expected<T2, E2> &y) {
            if (x.has_value() != y.has_value()) {
                return false;
            }
            if (x.has_value()) {
                return *x == *y;
            }
            return x.error() == y.error();
        }

        template<class T2>
        friend constexpr bool operator==(const expected &x, const T2 &y) {
            return x.has_value() && static_cast<bool>(*x == y);
        }

        template<class E2>
        friend constexpr bool operator==(const expected &x, const unexpected<E2> &y) {
            return !x.has_value() && static_cast<bool>(x.error() == y.error());
        }

    private:
        static_assert(std::is_same_v<std::remove_cv_t<T>, void> ||
                      (std::is_object_v<std::remove_cv_t<T> > && !std::is_array_v<std::remove_cv_t<T> > &&
                       !std::is_same_v<std::remove_cv_t<T>, std::in_place_t> && !std::is_same_v<std::remove_cv_t<T>,
                           unexpect_t> &&
                       !internal::is_specialization<std::remove_cv_t<T>, unexpected>::value),
                      "Invalid type for fermat::expected.");


        // TODO: When T is not cv void, it shall meet the
        // Cpp17Destructible requirements. E shall meet the
        // Cpp17Destructible requirements. Can we statically assert this?
    };

    namespace internal {
        // Used as a fake "Value" type in the void specialization of expected so it can be default
        // constructible and so we can use all the other machinery we have for value/error pairs.
        struct ExpectedEmptyUnionMember {
            constexpr ExpectedEmptyUnionMember() noexcept {
                // Provide default constructor to avoid zero-initialization when objects are value-initialized.
            };
        };
    } // namespace internal

    // it for non-qualified void.
    template<class E>
    class expected<void, E>
            : internal::EnableExpectedSpecialMemberFunctions<internal::ExpectedConstructLayer<
                    internal::ExpectedEmptyUnionMember, E>,
                E> {
    private:
        using LayeredBase = internal::
        EnableExpectedSpecialMemberFunctions<internal::ExpectedConstructLayer<internal::ExpectedEmptyUnionMember, E>, E>
        ;

    public:
        using value_type = void;
        using error_type = E;
        using unexpected_type = unexpected<E>;

        template<class U>
        using rebind = expected<U, error_type>;

        constexpr expected() noexcept { this->_has_value = true; }
        constexpr expected(std::in_place_t) noexcept { this->_has_value = true; }

        // Copy/move constructors and the destructor are handled by the layers.

        template<class U,
            class G,
            std::enable_if_t<std::is_void_v<U> && std::is_constructible_v<E, const G &> &&
                             !std::is_constructible_v<unexpected<E>, expected<U, G> &> &&
                             !std::is_constructible_v<unexpected<E>, expected<U, G> > &&
                             !std::is_constructible_v<unexpected<E>, const expected<U, G> &> &&
                             !std::is_constructible_v<unexpected<E>, const expected<U, G>>,
                int> = 0>
        // TODO: More SFINAE for the explicit vs not explicit version...
        // explicit(!std::is_convertible_v<const G&, E>)
        constexpr expected(const expected<U, G> &other) {
            this->_has_value = other.has_value();
            if (!this->_has_value) {
                fermat::construct_at(std::addressof(this->_error), other.error());
            }
        }

        template<class U,
            class G,
            std::enable_if_t<std::is_void_v<U> && std::is_constructible_v<E, G> &&
                             !std::is_constructible_v<unexpected<E>, expected<U, G> &> &&
                             !std::is_constructible_v<unexpected<E>, expected<U, G> > &&
                             !std::is_constructible_v<unexpected<E>, const expected<U, G> &> &&
                             !std::is_constructible_v<unexpected<E>, const expected<U, G>>,
                int> = 0>
        // explicit(!std::is_convertible_v<T1, T> || !std::is_convertible_v<E1, E>)
        constexpr expected(expected<U, G> &&other) {
            this->_has_value = other.has_value();
            if (!this->_has_value) {
                fermat::construct_at(std::addressof(this->_error), std::move(other).error());
            }
        }


        // Conversion from unexpected lvalue explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && !std::is_convertible_v<const G &, E>
            , int> = 0>
        constexpr explicit expected(const unexpected<G> &unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), unex.error());
        }

        // Conversion from unexpected lvalue non-explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && std::is_convertible_v<const G &, E>,
            int> = 0>
        constexpr expected(const unexpected<G> &unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), unex.error());
        }

        // Conversion from unexpected rvalue explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && !std::is_convertible_v<G, E>, int> = 0>
        constexpr explicit expected(unexpected<G> &&unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), std::move(unex.error()));
        }

        // Conversion from unexpected rvalue non-explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && std::is_convertible_v<G, E>, int> = 0>
        constexpr expected(unexpected<G> &&unex) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), std::move(unex.error()));
        }

        template<class... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, Args &&... args) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), std::forward<Args>(args)...);
        }

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&... args) {
            this->_has_value = false;
            fermat::construct_at(std::addressof(this->_error), il, std::forward<Args>(args)...);
        }

        ////////
        //
        // copy/move assignments are done by means of ExpectedConstructLayer::AssignFrom, and the
        // special function layers so the assignments are deleted when they should be.
        //
        // constexpr expected& operator=(const expected&);
        // constexpr expected& operator=(expected&&) noexcept(/* see description */);
        //
        ///////


        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && std::is_assignable_v<E &, const G &>
            , int> = 0>
        constexpr expected &operator=(const unexpected<G> &unex) {
            if (this->_has_value) {
                this->ReInit(this->_error, this->_value, unex.error());
                this->_has_value = false;
            } else {
                this->_error = unex.error();
            }
            return *this;
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && std::is_assignable_v<E &, G>, int> = 0>
        constexpr expected &operator=(unexpected<G> &&unex) {
            if (this->_has_value) {
                this->ReInit(this->_error, this->_value, std::move(unex).error());
                this->_has_value = false;
            } else {
                this->_error = std::move(unex).error();
            }
            return *this;
        }

        template<bool Requires = std::is_swappable_v<E> && std::is_move_constructible_v<E>,
            std::enable_if_t<Requires, int> = 0,
            bool NoExcept = std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E> >
        void swap(expected &other) noexcept(NoExcept) {
            using std::swap;
            if (other._has_value) {
                if (this->_has_value) {
                    return;
                } else {
                    other.swap(*this);
                }
            } else // other._has_value is false
            {
                if (!this->_has_value) {
                    swap(this->_error, other._error);
                } else {
                    // other has an error and this has a value, we need to swap them around.
                    E tmp(std::move(other._error));
                    fermat::destroy_at(&other._error);
                    fermat::construct_at(std::addressof(other._value));
                    fermat::destroy_at(&this->_value);
                    fermat::construct_at(std::addressof(this->_error), std::move(tmp));
                    this->_has_value = false;
                    other._has_value = true;
                }
            }
        }

        friend constexpr void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

        constexpr explicit operator bool() const noexcept { return this->_has_value; }

       [[nodiscard]] constexpr bool has_value() const noexcept { return this->_has_value; };

        constexpr void operator*() const noexcept {
        }

        constexpr void value() const & {
            if (!has_value()) {
                KCHECK(false)<<"Calling `value()` when expected contains no value.";
            }
        }

        constexpr void value() && {
            if (!has_value()) {
                KCHECK(false) << "Calling `value()` when expected contains no value.";
            }
        }

        // These assume has_value() is false, otherwise this is UB, as per the standard.
        constexpr const E &error() const & {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return this->_error;
        };

        constexpr E &error() & {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return this->_error;
        };

        constexpr const E &&error() const && {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return std::move(this->_error);
        };

        constexpr E &&error() && {
            KCHECK(!has_value()) << "Pre-condition failed! Calling error() while containing a value.";
            return std::move(this->_error);
        };

        template<class U = E>
        constexpr E error_or(U &&alt) const & {
            static_assert(std::is_copy_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return this->_error;
        }

        template<class U = E>
        constexpr E error_or(U &&alt) && {
            static_assert(std::is_move_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return std::move(this->_error);
        }

        ///////////////////////
        // Monadic operations
        ///////////////////////
        template<class F, bool Requires = std::is_copy_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) & {
            using U = remove_cvref_t<std::invoke_result_t<F> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f));
            }
            return U(unexpect, error());
        }

        template<class F, bool Requires = std::is_copy_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) const & {
            using U = remove_cvref_t<std::invoke_result_t<F> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f));
            }
            return U(unexpect, error());
        }

        template<class F, bool Requires = std::is_move_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) && {
            using U = remove_cvref_t<std::invoke_result_t<F> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f));
            }
            return U(unexpect, std::move(error()));
        }

        template<class F, bool Requires = std::is_move_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto and_then(F &&f) const && {
            using U = remove_cvref_t<std::invoke_result_t<F> >;
            static_assert(std::is_same_v<typename U::error_type, E> && internal::is_specialization<U, expected>::value);
            if (has_value()) {
                return std::invoke(std::forward<F>(f));
            }
            return U(unexpect, std::move(error()));
        }

        template<class F>
        constexpr auto or_else(F &&f) & {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(error())> >;
            static_assert(
                std::is_same_v<typename G::value_type, void> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G();
            }
            return std::invoke(std::forward<F>(f), error());
        }

        template<class F>
        constexpr auto or_else(F &&f) const & {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(error())> >;
            static_assert(
                std::is_same_v<typename G::value_type, void> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G();
            }
            return std::invoke(std::forward<F>(f), error());
        }

        template<class F>
        constexpr auto or_else(F &&f) && {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            static_assert(
                std::is_same_v<typename G::value_type, void> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G();
            }
            return std::invoke(std::forward<F>(f), std::move(error()));
        }

        template<class F>
        constexpr auto or_else(F &&f) const && {
            using G = remove_cvref_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            static_assert(
                std::is_same_v<typename G::value_type, void> && internal::is_specialization<G, expected>::value);
            if (has_value()) {
                return G();
            }
            return std::invoke(std::forward<F>(f), std::move(error()));
        }

        template<class F, bool Requires = std::is_copy_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) & {
            using U = std::remove_cv_t<std::invoke_result_t<F> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, error());
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            }
        }

        template<class F, bool Requires = std::is_copy_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) const & {
            using U = std::remove_cv_t<std::invoke_result_t<F> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, error());
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            }
        }

        template<class F, bool Requires = std::is_move_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) && {
            using U = std::remove_cv_t<std::invoke_result_t<F> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, std::move(error()));
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            }
        }

        template<class F, bool Requires = std::is_move_constructible_v<E>, std::enable_if_t<Requires, int> = 0>
        constexpr auto transform(F &&f) const && {
            using U = std::remove_cv_t<std::invoke_result_t<F> >;
            if (!has_value()) {
                return expected<U, E>(unexpect, std::move(error()));
            }

            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f));
                return expected<U, E>();
            } else {
                return expected<U, E>(std::invoke(std::forward<F>(f)));
            }
        }

        template<class F>
        constexpr auto transform_error(F &&f) & {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(error())> >;
            if (has_value()) {
                return expected<void, G>();
            } else {
                return expected<void, G>(unexpect, std::invoke(std::forward<F>(f), error()));
            }
        }

        template<class F>
        constexpr auto transform_error(F &&f) const & {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(error())> >;
            if (has_value()) {
                return expected<void, G>();
            } else {
                return expected<void, G>(unexpect, std::invoke(std::forward<F>(f), error()));
            }
        }

        template<class F>
        constexpr auto transform_error(F &&f) && {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            if (has_value()) {
                return expected<void, G>();
            } else {
                return expected<void, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
            }
        }

        template<class F>
        constexpr auto transform_error(F &&f) const && {
            using G = std::remove_cv_t<std::invoke_result_t<F, decltype(std::move(error()))> >;
            if (has_value()) {
                return expected<void, G>();
            } else {
                return expected<void, G>(unexpect, std::invoke(std::forward<F>(f), std::move(error())));
            }
        }

        // equality operators
        template<class T2, class E2, bool Requires = std::is_void_v<T2>, std::enable_if_t<Requires, int> = 0>
        friend constexpr bool operator==(const expected &x, const expected<T2, E2> &y) {
            if (x.has_value() != y.has_value()) {
                return false;
            }
            return x.has_value() || static_cast<bool>(x.error() == y.error());
        }

        template<class E2>
        friend constexpr bool operator==(const expected &x, const unexpected<E2> &y) {
            return !x.has_value() && static_cast<bool>(x.error() == y.error());
        }
    };
} // namespace fermat
