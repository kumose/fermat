///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////
#pragma once


#include <fermat/base/eabase.h>

KM_DISABLE_VC_WARNING(4623) // warning C4623: default constructor was implicitly defined as deleted
KM_DISABLE_VC_WARNING(4625) // warning C4625: copy constructor was implicitly defined as deleted
KM_DISABLE_VC_WARNING(4510) // warning C4510: default constructor could not be generated

#include <fermat/types/internal/special_member_functions_expected.h>
#include <fermat/types/memory.h>
#include <fermat/types/type_traits.h>
#include <utility>

#include <initializer_list> // for std::initializer_list

#if FERMAT_EXCEPTIONS_ENABLED
#include <exception> // for std::exception in bad_exception_access.
#endif

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
                                        <E,
                                            Err>> >
        constexpr explicit unexpected(Err &&e) : mError(std::forward<Err>(e)) {
        };

        template<class... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
        constexpr explicit unexpected(std::in_place_t, Args &&... args) : mError(std::forward<Args>(args)...) {
        };

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit unexpected(std::in_place_t, std::initializer_list<U> il, Args &&... args)
            : mError(il, std::forward<Args>(args)...) {
        };

        constexpr unexpected &operator=(const unexpected &) = default;

        constexpr unexpected &operator=(unexpected &&) = default;

        constexpr const E &error() const & noexcept { return mError; };
        constexpr E &error() & noexcept { return mError; };
        constexpr const E &&error() const && noexcept { return std::move(mError); };
        constexpr E &&error() && noexcept { return std::move(mError); };

        constexpr void swap(unexpected &other) noexcept(std::is_nothrow_swappable_v<E>) {
            static_assert(std::is_swappable_v<E>, "unexpected<E> swap requires E to be swappable");
            using std::swap;
            swap(mError, other.mError);
        };

        friend constexpr void swap(unexpected &x, unexpected &y) noexcept(noexcept(x.swap(y))) {
            static_assert(std::is_swappable_v<E>, "unexpected<E> swap requires E to be swappable");
            x.swap(y);
        }

        // equality operator
        template<class E2>
        friend constexpr bool operator==(const unexpected &x, const unexpected<E2> &y) {
            return x.mError == y.mError;
        }

    private:
        E mError;

        // The standard specifies these are ill-formed.
        static_assert(std::is_object_v<E> && !std::is_array_v<E> && !std::is_const_v<E> && !std::is_volatile_v<E> &&
                      !internal::is_specialization<E, unexpected>::value,
                      "This type is not supported by a conforming implementation of unexpected.");
    };

    template<class E>
    unexpected(E) -> unexpected<E>;


#if FERMAT_EXCEPTIONS_ENABLED
    template<class E>
    class bad_expected_access;

    template<>
    class bad_expected_access<void> : public std::exception {
    public:
        const char *what() const noexcept override { return "Bad expected access."; };

    protected:
        bad_expected_access() noexcept = default;

        bad_expected_access(const bad_expected_access &) = default;

        bad_expected_access(bad_expected_access &&) = default;

        bad_expected_access &operator=(const bad_expected_access &) = default;

        bad_expected_access &operator=(bad_expected_access &&) = default;

        ~bad_expected_access() = default;
    };

    template<class E>
    class bad_expected_access : public bad_expected_access<void> {
    public:
        explicit bad_expected_access(E e) : mError(std::move(e)) {
        };

        // just use the base class' what(), no need to override this.
        // const char* what() const noexcept override;

        E &error() & noexcept { return mError; };
        const E &error() const & noexcept { return mError; };
        E &&error() && noexcept { return std::move(mError); };
        const E &&error() const && noexcept { return std::move(mError); };

    private:
        E mError;
    };
#endif // FERMAT_EXCEPTIONS_ENABLED

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
                T mValue;
                E mError;
            };

            bool mHasValue;
        };

        //
        // The case where one of T or E is not trivially destructible.
        template<class T, class E>
        struct ExpectedDestructLayer<T, E, false> {
            ~ExpectedDestructLayer() {
                if (mHasValue) {
                    std::destroy_at(&mValue);
                } else {
                    std::destroy_at(&mError);
                }
            }

            // Note: we deliberately don't initialize anything here, member initailization for
            // the default conxtructoris done in the `expected` class.
            constexpr ExpectedDestructLayer() {
            };

            union {
                T mValue;
                E mError;
            };

            bool mHasValue;
        };


        ///////
        // ExpectedConstructLayer handles the implemenation of the copy/move constructor/assignment
        //
        template<class T, class E>
        struct ExpectedConstructLayer : ExpectedDestructLayer<T, E> {
            using ExpectedDestructLayer<T, E>::ExpectedDestructLayer;

            void ConstructFrom(const ExpectedConstructLayer &other) {
                this->mHasValue = other.mHasValue;
                if (this->mHasValue) {
                    fermat::construct_at(fermat::addressof(this->mValue), other.mValue);
                } else {
                    fermat::construct_at(fermat::addressof(this->mError), other.mError);
                }
            }

            void ConstructFrom(ExpectedConstructLayer &&other) {
                this->mHasValue = other.mHasValue;
                if (this->mHasValue) {
                    fermat::construct_at(fermat::addressof(this->mValue), std::move(other.mValue));
                } else {
                    fermat::construct_at(fermat::addressof(this->mError), std::move(other.mError));
                }
            }

            void AssignFrom(const ExpectedConstructLayer &other) {
                if (this->mHasValue && other.mHasValue) {
                    this->mValue = other.mValue;
                } else if (this->mHasValue) {
                    ReInit(this->mError, this->mValue, other.mError);
                    this->mHasValue = false;
                } else if (other.mHasValue) {
                    ReInit(this->mValue, this->mError, other.mValue);
                    this->mHasValue = true;
                } else {
                    this->mError = other.mError;
                }
            }

            void AssignFrom(ExpectedConstructLayer &&other) {
                if (this->mHasValue && other.mHasValue) {
                    this->mValue = std::move(other.mValue);
                } else if (this->mHasValue) {
                    ReInit(this->mError, this->mValue, std::move(other.mError));
                    this->mHasValue = false;
                } else if (other.mHasValue) {
                    ReInit(this->mValue, this->mError, std::move(other.mValue));
                    this->mHasValue = true;
                } else {
                    this->mError = std::move(other.mError);
                }
            }


            template<class NewVal, class OldVal, class... Args>
            void ReInit(NewVal &newval, OldVal &oldval, Args &&... args) {
#if FERMAT_EXCEPTIONS_ENABLED
                if constexpr (std::is_nothrow_constructible_v<NewVal, Args...>) {
                    std::destroy_at(&oldval);
                    fermat::construct_at(fermat::addressof(newval), std::forward<Args>(args)...);
                } else if constexpr (std::is_nothrow_move_constructible_v<NewVal>) {
                    NewVal tmp(std::forward<Args>(args)...);
                    std::destroy_at(&oldval);
                    fermat::construct_at(fermat::addressof(newval), std::move(tmp));
                } else {
                    OldVal tmp(std::move(oldval));
                    std::destroy_at(&oldval);
                    try {
                        fermat::construct_at(fermat::addressof(newval), std::forward<Args>(args)...);
                    } catch (...) {
                        fermat::construct_at(fermat::addressof(oldval), std::move(tmp));
                        throw;
                    }
                }
#else
                std::destroy_at(&oldval);
                fermat::construct_at(&newval, std::forward<Args>(args)...);
#endif
            }
        };
    } // namespace internal

    // TODO: we've marked member functions and constructors as constexpr when the standard
    // dictates it, but in reality a lot of these functions do now work at constant evaluation
    // time becuase they use facilities like `fermat::addressof` and `fermat::construct_at` which
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
            this->mHasValue = true;
            fermat::construct_at(fermat::addressof(this->mValue));
        };

        // non-explicit version for when std::is_convertible_v<U, T> is true.
        template<class U,
            std::enable_if_t<std::is_convertible_v<U, T> && internal::generic_constructor_constraint_v<T, E, U>, int> =
                    0>
        constexpr expected(U &&v) {
            this->mHasValue = true;
            fermat::construct_at(fermat::addressof(this->mValue), std::forward<U>(v));
        }

        // explicit version for when std::is_convertible_v<U, T> is false
        template<class U,
            std::enable_if_t<!std::is_convertible_v<U, T> && internal::generic_constructor_constraint_v<T, E, U>, int> =
                    0>
        constexpr explicit expected(U &&v) {
            this->mHasValue = true;
            fermat::construct_at(fermat::addressof(this->mValue), std::forward<U>(v));
        }

        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, const T1 &, const E1 &> &&
                             (!std::is_convertible_v<const T1 &, T> || !std::is_convertible_v<const E1 &, E>),
                int> = 0>
        constexpr explicit expected(const expected<T1, E1> &other) {
            this->mHasValue = other.has_value();
            if (this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mValue), other.value());
            } else {
                fermat::construct_at(fermat::addressof(this->mError), other.error());
            }
        }

        // Same as above except this is implicit when std::is_convertible_v<const T1&, T> && std::is_convertible_v<const E1&, E>.
        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, const T1 &, const E1 &> &&
                             (std::is_convertible_v<const T1 &, T> && std::is_convertible_v<const E1 &, E>),
                int> = 0>
        constexpr expected(const expected<T1, E1> &other) {
            this->mHasValue = other.has_value();
            if (this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mValue), other.value());
            } else {
                fermat::construct_at(fermat::addressof(this->mError), other.error());
            }
        }

        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, T1, E1> &&
                             (!std::is_convertible_v<T1, T> || !std::is_convertible_v<E1, E>),
                int> = 0>
        constexpr explicit expected(expected<T1, E1> &&other) {
            this->mHasValue = other.has_value();
            if (this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mValue), std::move(other).value());
            } else {
                fermat::construct_at(fermat::addressof(this->mError), std::move(other).error());
            }
        }

        // Same as above except this is implicit when (std::is_convertible_v<T1, T> && std::is_convertible_v<E1, E>)
        template<class T1,
            class E1,
            std::enable_if_t<internal::expected_to_expected_ctor_constraint_v<T, E, T1, E1, T1, E1> &&
                             (std::is_convertible_v<T1, T> && std::is_convertible_v<E1, E>),
                int> = 0>
        constexpr expected(expected<T1, E1> &&other) {
            this->mHasValue = other.has_value();
            if (this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mValue), std::move(other).value());
            } else {
                fermat::construct_at(fermat::addressof(this->mError), std::move(other).error());
            }
        }


        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && !std::is_convertible_v<const G &, E>
            , int> = 0>
        constexpr explicit expected(const unexpected<G> &unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), unex.error());
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && std::is_convertible_v<const G &, E>,
            int> = 0>
        constexpr expected(const unexpected<G> &unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), unex.error());
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && !std::is_convertible_v<G, E>, int> = 0>
        constexpr explicit expected(unexpected<G> &&unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), std::move(unex.error()));
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && std::is_convertible_v<G, E>, int> = 0>
        constexpr expected(unexpected<G> &&unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), std::move(unex.error()));
        }

        template<class... Args, std::enable_if_t<std::is_constructible_v<T, Args...>, int> = 0>
        constexpr explicit expected(std::in_place_t, Args &&... args) {
            this->mHasValue = true;
            fermat::construct_at(fermat::addressof(this->mValue), std::forward<Args>(args)...);
        }

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<T, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit expected(std::in_place_t, std::initializer_list<U> il, Args &&... args) {
            this->mHasValue = true;
            fermat::construct_at(fermat::addressof(this->mValue), il, std::forward<Args>(args)...);
        }

        template<class... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, Args &&... args) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), std::forward<Args>(args)...);
        }

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&... args) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), il, std::forward<Args>(args)...);
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
            if (this->mHasValue) {
                this->mValue = std::forward<U>(x);
            } else {
                this->ReInit(this->mValue, this->mError, std::forward<U>(x));
                this->mHasValue = true;
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
            if (this->mHasValue) {
                this->ReInit(this->mError, this->mValue, unex.error());
                this->mHasValue = false;
            } else {
                this->mError = unex.error();
            }
            return *this;
        }

        template<class G,
            std::enable_if_t<std::is_constructible_v<E, G> && std::is_assignable_v<E &, G> &&
                             (std::is_nothrow_constructible_v<E, G> || std::is_nothrow_move_constructible_v<T> ||
                              std::is_nothrow_move_constructible_v<E>),
                int> = 0>
        constexpr expected &operator=(unexpected<G> &&unex) {
            if (this->mHasValue) {
                this->ReInit(this->mError, this->mValue, std::move(unex).error());
                this->mHasValue = false;
            } else {
                this->mError = std::move(unex).error();
            }
            return *this;
        }

        // Note: this only works if the constructor is noexcept, kind of strict but that's what the standard dictates...
        template<class... Args, std::enable_if_t<std::is_nothrow_constructible_v<T, Args...>, int> = 0>
        constexpr T &emplace(Args &&... args) noexcept {
            if (this->mHasValue) {
                std::destroy_at(&this->mValue);
            } else {
                std::destroy_at(&this->mError);
                this->mHasValue = true;
            }
            return *fermat::construct_at(fermat::addressof(this->mValue), std::forward<Args>(args)...);
        }

        // Note: this only works if the constructor is noexcept, kind of strict but that's what the standard dictates...
        template<class U,
            class... Args,
            std::enable_if_t<std::is_nothrow_constructible_v<T, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr T &emplace(std::initializer_list<U> il, Args &&... args) noexcept {
            if (this->mHasValue) {
                std::destroy_at(&this->mValue);
            } else {
                std::destroy_at(&this->mError);
                this->mHasValue = true;
            }
            return *fermat::construct_at(fermat::addressof(this->mValue), il, std::forward<Args>(args)...);
        }

        // swap
        template<bool Requires = std::is_swappable_v<T> && std::is_swappable_v<E> && std::is_move_constructible_v<T> &&
                                 std::is_move_constructible_v<E> &&
                                 (std::is_nothrow_move_assignable_v<E> || std::is_nothrow_move_assignable_v<T>),
            std::enable_if_t<Requires, int> = 0,
            bool NoExcept = std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T> &&
                            std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E> >
        KM_CPP20_CONSTEXPR void swap(expected &other) noexcept(NoExcept) {
            using std::swap;
            if (other.mHasValue) {
                if (this->mHasValue) {
                    swap(this->mValue, other.mValue);
                } else {
                    other.swap(*this);
                }
            } else // other.mHasValue is false
            {
                if (!this->mHasValue) {
                    swap(this->mError, other.mError);
                } else // `other` has an error and `this` has a value, we need to swap them around.
                {
#if FERMAT_EXCEPTIONS_ENABLED
                    if constexpr (NoExcept) {
#endif
                    // Note that std::is_nothrow_swappable_v implies the destructors cannot throw.
                    // The definition of NoExcept implies the constructions here cannot throw.
                    // So notheng here throws.
                    E tmp(std::move(other.mError));
                    std::destroy_at(&other.mError);
                    fermat::construct_at(fermat::addressof(other.mValue), std::move(this->mValue));
                    std::destroy_at(&this->mValue);
                    fermat::construct_at(fermat::addressof(this->mError), std::move(tmp));
#if FERMAT_EXCEPTIONS_ENABLED
					}
					else if constexpr (std::is_nothrow_move_constructible_v<E>) {
                        E tmp(std::move(other.mError));
                        std::destroy_at(&other.mError);
                        try {
                            // this may throw
                            fermat::construct_at(fermat::addressof(other.mValue), std::move(this->mValue));

                            std::destroy_at(&this->mValue);
                            fermat::construct_at(fermat::addressof(this->mError), std::move(tmp));
                        } catch (...) {
                            // We need to reconstruct other.mError.
                            fermat::construct_at(fermat::addressof(other.mError), std::move(tmp));
                            throw;
                        }
                    } else // T is nothrow_move_constructible (see sfinae condition for swap)
                    {
                        T tmp(std::move(this->mValue));

                        std::destroy_at(&this->mValue);
                        try {
                            // this may throw
                            fermat::construct_at(fermat::addressof(this->mError), std::move(other.mError));

                            std::destroy_at(&other.mError);
                            fermat::construct_at(fermat::addressof(other.mValue), std::move(tmp));
                        } catch (...) {
                            // We need to reconstruct this->mValue
                            fermat::construct_at(fermat::addressof(this->mValue), std::move(tmp));
                            throw;
                        }
                    }
#endif
                    this->mHasValue = false;
                    other.mHasValue = true;
                }
            }
        }

        friend constexpr void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

        // These all assume has_value() is true. Otherwise, calling them is UB (as per the
        // standard).  When asserts are enabled, we've decided to assert the precondition
        // similar to what would be done in a hardened library implementation.
        constexpr const T *operator->() const noexcept {
            FERMAT_ASSERT_MSG(has_value(),
                              "Pre-condition failed! Accessing an expected value while containing an error.");
            return fermat::addressof(this->mValue);
        }

        constexpr T *operator->() noexcept {
            FERMAT_ASSERT_MSG(has_value(),
                              "Pre-condition failed! Accessing an expected value while containing an error.");
            return fermat::addressof(this->mValue);
        }

        constexpr const T &operator*() const & noexcept {
            FERMAT_ASSERT_MSG(has_value(),
                              "Pre-condition failed! Accessing an expected value while containing an error.");
            return this->mValue;
        }

        constexpr T &operator*() & noexcept {
            FERMAT_ASSERT_MSG(has_value(),
                              "Pre-condition failed! Accessing an expected value while containing an error.");
            return this->mValue;
        }

        constexpr const T &&operator*() const && noexcept {
            FERMAT_ASSERT_MSG(has_value(),
                              "Pre-condition failed! Accessing an expected value while containing an error.");
            return std::move(this->mValue);
        }

        constexpr T &&operator*() && noexcept {
            FERMAT_ASSERT_MSG(has_value(),
                              "Pre-condition failed! Accessing an expected value while containing an error.");
            return std::move(this->mValue);
        }

        constexpr explicit operator bool() const noexcept { return this->mHasValue; }
        constexpr bool has_value() const noexcept { return this->mHasValue; };

        constexpr const T &value() const & {
            if (!has_value()) {
#if FERMAT_EXCEPTIONS_ENABLED
                throw(fermat::bad_expected_access(this->mError));
#else
                FERMAT_FAIL_MSG("Calling `value()` when expected contains no value.");
#endif
            }
            return this->mValue;
        }

        constexpr T &value() & {
            if (!has_value()) {
#if FERMAT_EXCEPTIONS_ENABLED
                throw(fermat::bad_expected_access(this->mError));
#else
                FERMAT_FAIL_MSG("Calling `value()` when expected contains no value.");
#endif
            }
            return this->mValue;
        }

        constexpr const T &&value() const && {
            if (!has_value()) {
#if FERMAT_EXCEPTIONS_ENABLED
                throw(fermat::bad_expected_access(std::move(this->mError)));
#else
                FERMAT_FAIL_MSG("Calling `value()` when expected contains no value.");
#endif
            }
            return std::move(this->mValue);
        }

        constexpr T &&value() && {
            if (!has_value()) {
#if FERMAT_EXCEPTIONS_ENABLED
                throw(fermat::bad_expected_access(std::move(this->mError)));
#else
                FERMAT_FAIL_MSG("Calling `value()` when expected contains no value.");
#endif
            }
            return std::move(this->mValue);
        }

        // These all assume has_value() is false. Otherwise, calling them is UB (as per the
        // standard).  When asserts are enabled, we've decided to assert the precondition
        // similar to what would be done in a hardened library implementation.
        constexpr const E &error() const & {
            FERMAT_ASSERT_MSG(!has_value(), "Pre-condition failed! Calling error() while containing a value.");
            return this->mError;
        };

        constexpr E &error() & {
            FERMAT_ASSERT_MSG(!has_value(), "Pre-condition failed! Calling error() while containing a value.");
            return this->mError;
        };

        constexpr const E &&error() const && {
            FERMAT_ASSERT_MSG(!has_value(), "Pre-condition failed! Calling error() while containing a value.");
            return std::move(this->mError);
        };

        constexpr E &&error() && {
            FERMAT_ASSERT_MSG(!has_value(), "Pre-condition failed! Calling error() while containing a value.");
            return std::move(this->mError);
        };

        template<class U>
        constexpr T value_or(U &&alt) const & {
            static_assert(std::is_copy_constructible_v<T> && std::is_convertible_v<U, T>);
            return has_value() ? this->mValue : static_cast<T>(std::forward<U>(alt));
        }

        template<class U>
        constexpr T value_or(U &&alt) && {
            static_assert(std::is_move_constructible_v<T> && std::is_convertible_v<U, T>);
            return has_value() ? std::move(this->mValue) : static_cast<T>(std::forward<U>(alt));
        }

        template<class U>
        constexpr E error_or(U &&alt) const & {
            static_assert(std::is_copy_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return this->mError;
        }

        template<class U>
        constexpr E error_or(U &&alt) && {
            static_assert(std::is_move_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return std::move(this->mError);
        }

        // Note: the constraint in the standard is std::is_constructible_v<E, decltype(error())>
        // here and std::is_constructible_v<E, decltype(std::move(error()))> in the && qualified
        // versions, we're just explicitly spellig the decltype in our implementations since we
        // can't put the member call in the template argument. std::declval doesn't really help us
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

    // TODO: The standard has specializations for all cv-qualified void, but we're only doing
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

        constexpr expected() noexcept { this->mHasValue = true; }
        constexpr expected(std::in_place_t) noexcept { this->mHasValue = true; }

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
            this->mHasValue = other.has_value();
            if (!this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mError), other.error());
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
        // TODO: More SFINAE for the explicit vs not explicit version...
        // explicit(!std::is_convertible_v<T1, T> || !std::is_convertible_v<E1, E>)
        constexpr expected(expected<U, G> &&other) {
            this->mHasValue = other.has_value();
            if (!this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mError), std::move(other).error());
            }
        }


        // Conversion from unexpected lvalue explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && !std::is_convertible_v<const G &, E>
            , int> = 0>
        constexpr explicit expected(const unexpected<G> &unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), unex.error());
        }

        // Conversion from unexpected lvalue non-explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && std::is_convertible_v<const G &, E>,
            int> = 0>
        constexpr expected(const unexpected<G> &unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), unex.error());
        }

        // Conversion from unexpected rvalue explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && !std::is_convertible_v<G, E>, int> = 0>
        constexpr explicit expected(unexpected<G> &&unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), std::move(unex.error()));
        }

        // Conversion from unexpected rvalue non-explicit version.
        template<class G, std::enable_if_t<std::is_constructible_v<E, G> && std::is_convertible_v<G, E>, int> = 0>
        constexpr expected(unexpected<G> &&unex) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), std::move(unex.error()));
        }

        template<class... Args, std::enable_if_t<std::is_constructible_v<E, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, Args &&... args) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), std::forward<Args>(args)...);
        }

        template<class U,
            class... Args,
            std::enable_if_t<std::is_constructible_v<E, std::initializer_list<U> &, Args...>, int> = 0>
        constexpr explicit expected(unexpect_t, std::initializer_list<U> il, Args &&... args) {
            this->mHasValue = false;
            fermat::construct_at(fermat::addressof(this->mError), il, std::forward<Args>(args)...);
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
            if (this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mError), unex.error());
                this->mHasValue = false;
            } else {
                this->mError = unex.error();
            }
            return *this;
        }

        template<class G, std::enable_if_t<std::is_constructible_v<E, const G &> && std::is_assignable_v<E &, G>, int> =
                0>
        constexpr expected &operator=(unexpected<G> &&unex) {
            if (this->mHasValue) {
                fermat::construct_at(fermat::addressof(this->mError), std::move(unex.error()));
                this->mHasValue = false;
            } else {
                this->mError = unex.error();
            }
            return *this;
        }

        template<bool Requires = std::is_swappable_v<E> && std::is_move_constructible_v<E>,
            std::enable_if_t<Requires, int> = 0,
            bool NoExcept = std::is_nothrow_move_constructible_v<E> && std::is_nothrow_swappable_v<E> >
        KM_CPP20_CONSTEXPR void swap(expected &other) noexcept(NoExcept) {
            using std::swap;
            if (other.mHasValue) {
                if (this->mHasValue) {
                    return;
                } else {
                    other.swap(*this);
                }
            } else // other.mHasValue is false
            {
                if (!this->mHasValue) {
                    swap(this->mError, other.mError);
                } else {
                    // other has an error and this has a value, we need to swap them around.
                    fermat::construct_at(fermat::addressof(this->mError), std::move(other.mError));
                    std::destroy_at(&other.mError);

                    this->mHasValue = false;
                    other.mHasValue = true;
                }
            }
        }

        friend constexpr void swap(expected &x, expected &y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

        constexpr explicit operator bool() const noexcept { return this->mHasValue; }

        constexpr bool has_value() const noexcept { return this->mHasValue; };

        constexpr void operator*() const noexcept {
        }

        constexpr void value() const & {
            if (!has_value()) {
#if FERMAT_EXCEPTIONS_ENABLED
                throw(fermat::bad_expected_access(std::move(this->mError)));
#else
                FERMAT_FAIL_MSG("Calling `value()` when expected contains no value.");
#endif
            }
        }

        constexpr void value() && {
            if (!has_value()) {
#if FERMAT_EXCEPTIONS_ENABLED
                throw(fermat::bad_expected_access(std::move(this->mError)));
#else
                FERMAT_FAIL_MSG("Calling `value()` when expected contains no value.");
#endif
            }
        }

        // These assume has_value() is false, otherwise this is UB, as per the standard.
        constexpr const E &error() const & noexcept { return this->mError; };
        constexpr E &error() & noexcept { return this->mError; };
        constexpr const E &&error() const && noexcept { return std::move(this->mError); };
        constexpr E &&error() && noexcept { return std::move(this->mError); };

        template<class U = E>
        constexpr E error_or(U &&alt) const & {
            static_assert(std::is_copy_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return this->mError;
        }

        template<class U = E>
        constexpr E error_or(U &&alt) && {
            static_assert(std::is_move_constructible_v<E> && std::is_convertible_v<U, E>);
            if (has_value()) {
                return std::forward<U>(alt);
            }
            return std::move(this->mError);
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
            using U = remove_cvref_t<std::invoke_result_t<F, decltype(std::move(value()))> >;
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
                return expected<U, E>(std::invoke(std::forward<F>(f), std::move(value())));
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
                return expected<U, E>(std::invoke(std::forward<F>(f), std::move(value())));
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

KM_RESTORE_VC_WARNING() KM_RESTORE_VC_WARNING() KM_RESTORE_VC_WARNING()

