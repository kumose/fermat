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

#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace fermat {

    /// std
    using std::uninitialized_copy;
    using std::uninitialized_copy_n;
    using std::uninitialized_move;
    using std::uninitialized_move_n;
    using std::uninitialized_default_construct;
    using std::uninitialized_default_construct_n;
    using std::uninitialized_value_construct;
    using std::uninitialized_value_construct_n;
    using std::uninitialized_fill_n;

namespace container_internal {

    template<typename T, typename Enabler = void>
    struct construct_default;

    template<typename T>
    struct construct_default<T, std::enable_if_t<std::is_trivially_default_constructible_v<T> &&
                                                 std::is_assignable_v<T &, const T &>>> {
        static inline const T value{};

        static constexpr T *apply(T *p) noexcept {
            *p = value;
            return p;
        }
    };

    template<typename T>
    struct construct_default<T, std::enable_if_t<!(std::is_trivially_default_constructible_v<T> &&
                                                    std::is_assignable_v<T &, const T &>)>> {
        static constexpr T *apply(T *p) {
            return ::new (static_cast<void *>(p)) T();
        }
    };

    template<typename T, typename Arg, typename Enabler = void>
    struct construct_one;

    template<typename T, typename Arg>
    struct construct_one<T, Arg,
            std::enable_if_t<std::is_trivially_constructible_v<T, Arg> &&
                             std::is_assignable_v<T &, Arg>>> {
        static constexpr T *apply(T *p, Arg &&arg) noexcept(std::is_nothrow_assignable_v<T &, Arg>) {
            *p = std::forward<Arg>(arg);
            return p;
        }
    };

    template<typename T, typename Arg>
    struct construct_one<T, Arg,
            std::enable_if_t<!(std::is_trivially_constructible_v<T, Arg> &&
                               std::is_assignable_v<T &, Arg>)>> {
        template<typename U>
        static constexpr T *apply(T *p, U &&arg) {
            return ::new (static_cast<void *>(p)) T(std::forward<U>(arg));
        }
    };

    template<typename T>
    constexpr T *construct_at_dispatch(T *p) {
        return construct_default<T>::apply(p);
    }

    template<typename T, typename Arg>
    constexpr T *construct_at_dispatch(T *p, Arg &&arg) {
        return construct_one<T, Arg>::apply(p, std::forward<Arg>(arg));
    }

    template<typename T, typename Arg0, typename Arg1, typename... Rest>
    constexpr T *construct_at_dispatch(T *p, Arg0 &&a0, Arg1 &&a1, Rest &&... rest) {
        return ::new (static_cast<void *>(p)) T(
            std::forward<Arg0>(a0), std::forward<Arg1>(a1), std::forward<Rest>(rest)...);
    }

} // namespace container_internal

    template<typename T, typename... Args>
    constexpr T *construct_at(T *p, Args &&... args)
        noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        return container_internal::construct_at_dispatch(p, std::forward<Args>(args)...);
    }

    /// uninitialized_copy_copy
    ///
    /// Copies [first1, last1) into [result, result + (last1 - first1)) then
    /// copies [first2, last2) into [result, result + (last1 - first1) + (last2 - first2)).
    ///
    template<typename InputIterator1, typename InputIterator2, typename ForwardIterator>
    inline ForwardIterator
    uninitialized_copy_copy(InputIterator1 first1, InputIterator1 last1,
                            InputIterator2 first2, InputIterator2 last2,
                            ForwardIterator result) {
        const ForwardIterator mid(std::uninitialized_copy(first1, last1, result));

        return std::uninitialized_copy(first2, last2, mid);
    }

    template<typename T>
    constexpr void destroy_at(T *p) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            ::std::destroy_at(p);
        }
    }

    template<typename ForwardIterator>
    constexpr void destroy(ForwardIterator first, ForwardIterator last) {
        if constexpr (!std::is_trivially_destructible_v<
                          typename std::iterator_traits<ForwardIterator>::value_type>) {
            ::std::destroy(first, last);
        }
    }

    template<typename ForwardIterator, typename T>
    inline void uninitialized_fill(ForwardIterator first, ForwardIterator last, const T &x) {
        if constexpr (std::is_trivial_v<T>) {
            for (; first != last; ++first) {
                *first = x;
            }
        } else {
            std::uninitialized_fill(first, last, x);
        }
    }
} // namespace fermat
