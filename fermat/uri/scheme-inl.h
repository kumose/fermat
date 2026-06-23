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

/////////////////////////////////////////////////////////////////////////////////////
/// @file scheme-inl.h
/// @brief Definitions for the URL scheme.

#pragma once

#include <fermat/uri/scheme.h>

namespace fermat::uri::scheme {
    /// @namespace fermat::uri::scheme::details
    /// @brief Includes the definitions for scheme specific entities
    namespace details {
        // for use with is_special and get_special_port
        // Spaces, if present, are removed from URL.
        constexpr std::string_view is_special_list[] = {
            "http", " ", "https", "ws",
            "ftp", "wss", "file", " "
        };
        // for use with get_special_port
        constexpr uint16_t special_ports[] = {80, 0, 443, 80, 21, 443, 0, 0};
    } // namespace details

    /****
     * @private
     * In is_special, get_scheme_type, and get_special_port, we
     * use a standard hashing technique to find the index of the scheme in
     * the is_special_list. The hashing technique is based on the size of
     * the scheme and the first character of the scheme. It ensures that we
     * do at most one string comparison per call. If the protocol is
     * predictible (e.g., it is always "http"), we can get a better average
     * performance by using a simpler approach where we loop and compare
     * scheme with all possible protocols starting with the most likely
     * protocol. Doing multiple comparisons may have a poor worst case
     * performance, however. In this instance, we choose a potentially
     * slightly lower best-case performance for a better worst-case
     * performance. We can revisit this choice at any time.
     *
     * Reference:
     * Schmidt, Douglas C. "Gperf: A perfect hash function generator."
     * More C++ gems 17 (2000).
     *
     * Reference: https://en.wikipedia.org/wiki/Perfect_hash_function
     *
     * Reference: https://github.com/ada-url/ada/issues/617
     ****/

    TURBO_FORCE_INLINE constexpr bool is_special(std::string_view scheme) {
        if (scheme.empty()) {
            return false;
        }
        int hash_value = (2 * scheme.size() + (unsigned) (scheme[0])) & 7;
        const std::string_view target = details::is_special_list[hash_value];
        return (target[0] == scheme[0]) && (target.substr(1) == scheme.substr(1));
    }

    constexpr uint16_t get_special_port(std::string_view scheme) noexcept {
        if (scheme.empty()) {
            return 0;
        }
        int hash_value = (2 * scheme.size() + (unsigned) (scheme[0])) & 7;
        const std::string_view target = details::is_special_list[hash_value];
        if ((target[0] == scheme[0]) && (target.substr(1) == scheme.substr(1))) {
            return details::special_ports[hash_value];
        } else {
            return 0;
        }
    }

    constexpr uint16_t get_special_port(fermat::uri::scheme::type type) noexcept {
        return details::special_ports[int(type)];
    }

    constexpr fermat::uri::scheme::type get_scheme_type(std::string_view scheme) noexcept {
        if (scheme.empty()) {
            return fermat::uri::scheme::NOT_SPECIAL;
        }
        int hash_value = (2 * scheme.size() + (unsigned) (scheme[0])) & 7;
        const std::string_view target = details::is_special_list[hash_value];
        if ((target[0] == scheme[0]) && (target.substr(1) == scheme.substr(1))) {
            return fermat::uri::scheme::type(hash_value);
        } else {
            return fermat::uri::scheme::NOT_SPECIAL;
        }
    }
} // namespace fermat::uri::scheme
