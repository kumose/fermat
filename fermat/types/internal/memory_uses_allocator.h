// (c) 2024 Electronic Arts Inc.

#pragma once

#include <fermat/types/internal/config.h>
#include <fermat/types/internal/memory_base.h>
#include <tuple> // via its transitive includes, needs memory_base.h but *not* memory_uses_allocator.h
#include <fermat/types/type_traits.h>

namespace fermat {
    ///////////////////////////////////////////////////////////////////////
    // uses_allocator
    //
    // Determines if the class T has an allocator_type member typedef
    // which Allocator is convertible to.
    //
    // http://en.cppreference.com/w/cpp/memory/uses_allocator
    //
    // A program may specialize this template to derive from std::true_type for a
    // user-defined type T that does not have a nested allocator_type but
    // nonetheless can be constructed with an allocator where either:
    //    - the first argument of a constructor has type allocator_arg_t and
    //      the second argument has type Allocator.
    //    or
    //    - the last argument of a constructor has type Allocator.
    //
    // Example behavilor:
    //     uses_allocator<vector>::value => true
    //     uses_allocator<int>::value    => false
    //
    // This is useful for writing generic code for containers when you can't
    // know ahead of time that the container has an allocator_type.
    ///////////////////////////////////////////////////////////////////////

    template<typename T>
    struct has_allocator_type_helper {
    private:
        template<typename>
        static fermat::no_type test(...);

        template<typename U>
        static fermat::yes_type test(typename U::allocator_type * = NULL);

    public:
        static const bool value = sizeof(test<T>(NULL)) == sizeof(fermat::yes_type);
    };


    template<typename T, typename Allocator, bool = has_allocator_type_helper<T>::value>
    struct uses_allocator_impl
            : public integral_constant<bool, std::is_convertible<Allocator, typename T::allocator_type>::value> {
    };

    template<typename T, typename Allocator>
    struct uses_allocator_impl<T, Allocator, false> : public std::false_type {
    };

    template<typename T, typename Allocator>
    struct uses_allocator : public uses_allocator_impl<T, Allocator> {
    };

    template<typename T, typename Allocator>
    constexpr bool uses_allocator_v = uses_allocator<T, Allocator>::value;

    namespace detail {
        template<typename T, typename... Args>
        struct has_allocator_construct {
            template<typename Allocator, typename = void>
            struct inner : std::false_type {
            };

            template<typename Allocator>
            struct inner<Allocator,
                        fermat::void_t<decltype(std::declval<Allocator &>().construct(std::declval<T *>(),
                            std::declval<Args &&>()...))> >
                    : std::true_type {
            };
        };

        // equivalent to std::allocator_traits<Alloc>::construct
        template<typename Allocator, typename T, typename... Args>
        constexpr
        std::enable_if_t<has_allocator_construct<T, Args...>::template inner<Allocator>::value, void>
        allocator_construct(Allocator &allocator, T *p, Args &&... args) {
            allocator.construct(p, std::forward<Args>(args)...);
        }

        template<typename Allocator, typename T, typename... Args>
        constexpr
        std::enable_if_t<!has_allocator_construct<T, Args...>::template inner<Allocator>::value, void>
        allocator_construct(Allocator &, T *p, Args &&... args) {
            fermat::construct_at(p, std::forward<Args>(args)...);
        }
    } // namespace detail
} // namespace fermat
