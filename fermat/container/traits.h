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
#include <turbo/log/logging.h>

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
        struct HasReserve<T, std::void_t<decltype(std::declval<T>().reserve(std::declval<size_t>()))> >
                : std::true_type {
        };

        // -----------------------------------------------------------------------------
        // HasResize: checks for T::resize(size_t)
        template<typename T, typename = void>
        struct HasResize : std::false_type {
        };

        template<typename T>
        struct HasResize<T, std::void_t<decltype(std::declval<T>().resize(std::declval<size_t>()))> >
                : std::true_type {
        };

        // -----------------------------------------------------------------------------
        // HasAppend: checks for T::append(const char*, size_t)
        // Can be generalized for other character types if needed.
        template<typename T, typename = void>
        struct HasAppend : std::false_type {
        };

        template<typename T>
        struct HasAppend<T, std::void_t<decltype(std::declval<T>().append(std::declval<const char *>(),
                                                                     std::declval<size_t>()))> >
                : std::true_type {
        };

        template<typename T, typename = void>
        struct HasPushBack : std::false_type {
        };

        template<typename T>
        struct HasPushBack<T, std::void_t<decltype(std::declval<T>().push_back(std::declval<char>()))> >
                : std::true_type {
        };

        // HasData: checks for T::data()
        template<typename T, typename = void>
        struct HasData : std::false_type {
        };

        template<typename T>
        struct HasData<T, std::void_t<decltype(std::declval<T>().data())> >
                : std::true_type {
        };

        // -----------------------------------------------------------------------------
        // HasSize: checks for T::size()
        template<typename T, typename = void>
        struct HasSize : std::false_type {
        };

        template<typename T>
        struct HasSize<T, std::void_t<decltype(std::declval<T>().size())> >
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
        struct HasValueTypeChar<T, std::void_t<typename T::value_type> >
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
        struct HasSetSize<T, std::void_t<decltype(std::declval<T>().set_size(std::declval<size_t>()))> >
                : std::true_type {
        };

        /// fixed container
        template<typename T, typename = void>
        struct HasMaxSize : std::false_type {
        };

        template<typename T>
        struct HasMaxSize<T, std::void_t<decltype(std::declval<T>().max_size())> >
                : std::true_type {
        };
    }


    // -----------------------------------------------------------------------------
    // ContainerTraits: provides reserve() and append() for contiguous char containers
    // Requirement: T has value_type = char, and member functions: reserve, resize, data, size.
    // -----------------------------------------------------------------------------
    template<typename T, typename Enabler = void>
    struct ContainerTraits {
        using container_tag = void;

        static void reserve(T &, size_t) = delete;

        static void append(T &, const char *, size_t) = delete;
    };

    struct VectorContainerTag {
    };

    template<typename T>
    struct ContainerTraits<T, std::enable_if_t<container_internal::HasPushBack<T>::value &&
                                               !container_internal::HasAppend<T>::value &&
                                               container_internal::HasData<T>::value &&
                                               container_internal::HasSize<T>::value &&
                                               container_internal::HasReserve<T>::value &&
                                               container_internal::HasResize<T>::value &&
                                               container_internal::HasValueTypeChar<T>::value> > {
        static constexpr bool is_dynamic_container = true;

        using container_tag = VectorContainerTag;

        static turbo::Status reserve(T &c, size_t n) {
            c.reserve(n);
            return turbo::Status();
        }

        static turbo::Status append(T &c, const char *data, size_t len) {
            size_t old_size = c.size();
            c.resize(old_size + len);
            std::memcpy(c.data() + old_size, data, len);
            return turbo::Status();
        }
    };

    struct StringContainerTag {
    };

    template<typename T>
    struct ContainerTraits<T, std::enable_if_t<container_internal::HasAppend<T>::value &&
                                               container_internal::HasData<T>::value &&
                                               container_internal::HasSize<T>::value &&
                                               container_internal::HasReserve<T>::value &&
                                               container_internal::HasResize<T>::value &&
                                               container_internal::HasValueTypeChar<T>::value> > {
        static constexpr bool is_dynamic_container = true;

        using container_tag = StringContainerTag;

        static turbo::Status reserve(T &c, size_t n) {
            c.reserve(n);
            return turbo::Status();
        }

        static turbo::Status append(T &c, const char *data, size_t len) {
            c.append(data, len);
            return turbo::Status();
        }
    };

    // -----------------------------------------------------------------------------
    // ContainerTraits for fixed-size arrays (e.g., std::array<char, N> wrappers)
    // Requirements: T has data(), size(), value_type char, and set_size().
    // Does not have reserve() or resize().
    // -----------------------------------------------------------------------------

    struct FixedContainerTag {
    };

    template<typename T>
    struct ContainerTraits<T, std::enable_if_t<container_internal::HasData<T>::value &&
                                               container_internal::HasSize<T>::value &&
                                               container_internal::HasValueTypeChar<T>::value &&
                                               !container_internal::HasReserve<T>::value &&
                                               !container_internal::HasResize<T>::value &&
                                               container_internal::HasSetSize<T>::value &&
                                               container_internal::HasMaxSize<T>::value> > {
        static constexpr bool is_dynamic_container = true;
        using container_tag = FixedContainerTag;
        // Pre-allocate or check capacity; for fixed-size, only check.
        static turbo::Status reserve(T &c, size_t n) {
            if (c.max_size() < n) {
                return turbo::out_of_range_error("ContainerTraits::reserve: capacity insufficient [ ", c.max_size(),
                                                 " vs ",
                                                 n, " ]");
            }
            return turbo::Status();
        }

        // Append data, assuming enough capacity, and update logical size via set_size.
        static turbo::Status append(T &c, const char *data, size_t len) {
            size_t old_size = c.size(); // current logical size
            if (old_size + len > c.max_size()) {
                return turbo::out_of_range_error("ContainerTraits::append: not enough space");
            }
            std::memcpy(c.data() + old_size, data, len);
            c.set_size(old_size + len);
            return turbo::Status();
        }
    };

    template<typename T, typename = void>
    struct has_equality_operator : std::false_type {
    };

    template<typename T>
    struct has_equality_operator<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())> >
            : std::true_type {
    };

    static_assert(has_equality_operator<int>::value);


    /// @brief Trait to // data() and size
    template<typename T>
    struct is_contiguous_string_visitor : std::false_type {
        static constexpr size_t kAlignment = 0;
    };

    template<>
    struct is_contiguous_string_visitor<std::string_view> : std::true_type {
        static constexpr size_t kAlignment = 0;
    };

    template<>
    struct is_contiguous_string_visitor<std::string> : std::true_type {
        static constexpr size_t kAlignment = 0;
    };


    template<>
    struct is_contiguous_string_visitor<std::vector<char> > : std::true_type {
        static constexpr size_t kAlignment = 0;
    };

    namespace detail {
        template<typename, typename = void>
        struct is_transparent_comparison : std::false_type {
        };

        template<typename T>
        struct is_transparent_comparison<T, std::void_t<typename T::is_transparent> > : std::true_type {
        };

        template<typename T>
        constexpr bool is_transparent_comparison_v = is_transparent_comparison<T>::value;
    } // namespace detail
} // namespace fermat
