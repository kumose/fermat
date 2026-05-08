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

#include <string>
#include <vector>
#include <cstring>
#include <turbo/utility/status.h>

namespace fermat {
    namespace container_internal {
        template<typename...>
        using void_t = void;
        /// string. vector
        ///
        // -----------------------------------------------------------------------------
        // HasReserve: checks for T::reserve(size_t)
        template<typename T, typename = void>
        struct HasReserve : std::false_type {
        };

        template<typename T>
        struct HasReserve<T, void_t<decltype(std::declval<T>().reserve(std::declval<size_t>()))> >
                : std::true_type {
        };

        // -----------------------------------------------------------------------------
        // HasResize: checks for T::resize(size_t)
        template<typename T, typename = void>
        struct HasResize : std::false_type {
        };

        template<typename T>
        struct HasResize<T, void_t<decltype(std::declval<T>().resize(std::declval<size_t>()))> >
                : std::true_type {
        };

        // -----------------------------------------------------------------------------
        // HasAppend: checks for T::append(const char*, size_t)
        // Can be generalized for other character types if needed.
        template<typename T, typename = void>
        struct HasAppend : std::false_type {
        };

        template<typename T>
        struct HasAppend<T, void_t<decltype(std::declval<T>().append(std::declval<const char *>(),
                                                                     std::declval<size_t>()))> >
                : std::true_type {
        };

        // HasData: checks for T::data()
        template<typename T, typename = void>
        struct HasData : std::false_type {
        };

        template<typename T>
        struct HasData<T, void_t<decltype(std::declval<T>().data())> >
                : std::true_type {
        };

        // -----------------------------------------------------------------------------
        // HasSize: checks for T::size()
        template<typename T, typename = void>
        struct HasSize : std::false_type {
        };

        template<typename T>
        struct HasSize<T, void_t<decltype(std::declval<T>().size())> >
                : std::true_type {
        };

        // -----------------------------------------------------------------------------
        // HasValueTypeChar
        // Checks whether T has a member type `value_type` and that this type is `char`
        // (ignoring cv-qualification).
        // -----------------------------------------------------------------------------
        template<typename T, typename = void>
        struct HasValueTypeChar : std::false_type {
        };

        template<typename T>
        struct HasValueTypeChar<T, void_t<typename T::value_type> >
                : std::bool_constant<
                    sizeof(std::remove_cv_t<typename T::value_type>) == 1 &&
                    std::is_integral_v<std::remove_cv_t<typename T::value_type> >
                > {
        };

        /// fixed container
        template<typename T, typename = void>
        struct HasSetSize : std::false_type {
        };

        template<typename T>
        struct HasSetSize<T, void_t<decltype(std::declval<T>().set_size(std::declval<size_t>()))> >
                : std::true_type {
        };
    }


    // -----------------------------------------------------------------------------
    // ContainerTraits: provides reserve() and append() for contiguous char containers
    // Requirement: T has value_type = char, and member functions: reserve, resize, data, size.
    // -----------------------------------------------------------------------------
    template<typename T, typename = void>
    struct ContainerTraits {
        static void reserve(T &, size_t) = delete;

        static void append(T &, const char *, size_t) = delete;
    };

    template<typename T>
    struct ContainerTraits<T, std::enable_if_t<container_internal::HasData<T>::value &&
                                               container_internal::HasSize<T>::value &&
                                               container_internal::HasReserve<T>::value &&
                                               container_internal::HasResize<T>::value &&
                                               container_internal::HasValueTypeChar<T>::value> > {
        static turbo::Status reserve(T &c, size_t n) {
            c.reserve(n);
            return turbo::Status();
        }

        static turbo::Status append(T &c, const char *data, size_t len) {
            size_t old_size = c.size();
            c.resize(old_size + len);
            std::memcpy(reinterpret_cast<const T*>(c.data()) + old_size, data, len);
            return turbo::Status();
        }
    };

    // -----------------------------------------------------------------------------
    // ContainerTraits for fixed-size arrays (e.g., std::array<char, N> wrappers)
    // Requirements: T has data(), size(), value_type char, and set_size().
    // Does not have reserve() or resize().
    // -----------------------------------------------------------------------------
    template<typename T>
    struct ContainerTraits<T, std::enable_if_t<container_internal::HasData<T>::value &&
                                               container_internal::HasSize<T>::value &&
                                               container_internal::HasValueTypeChar<T>::value &&
                                               !container_internal::HasReserve<T>::value &&
                                               !container_internal::HasResize<T>::value &&
                                               container_internal::HasSetSize<T>::value> > {
        // Pre-allocate or check capacity; for fixed-size, only check.
        static turbo::Status reserve(T &c, size_t n) {
            if (c.size() < n) {
                return turbo::out_of_range_error("ContainerTraits::reserve: capacity insufficient");
            }
            return turbo::Status();
        }

        // Append data, assuming enough capacity, and update logical size via set_size.
        static turbo::Status append(T &c, const char *data, size_t len) {
            size_t old_size = c.size(); // current logical size
            if (old_size + len > c.size()) {
                return turbo::out_of_range_error("ContainerTraits::append: not enough space");
            }
            std::memcpy(reinterpret_cast<const T*>(c.data()) + old_size, data, len);
            c.set_size(old_size + len);
            return turbo::Status();
        }
    };
} // namespace fermat
