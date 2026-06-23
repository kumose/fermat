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
/// @file serializers.h
/// @brief Definitions for the URL serializers.

#pragma once

#include <fermat/uri/common_defs.h>

#include <array>
#include <optional>
#include <fermat/container/string.h>

/// @namespace fermat::uri::serializers
/// @brief Includes the definitions for URL serializers
namespace fermat::uri::serializers {

    /// Finds and returns the longest sequence of 0 values in a ipv6 input.
    void find_longest_sequence_of_ipv6_pieces(
        const std::array<uint16_t, 8> &address, size_t &compress,
        size_t &compress_length) noexcept;

    /// Serializes an ipv6 address.
    /// @details An IPv6 address is a 128-bit unsigned integer that identifies a
    /// network address.
    /// @see https://url.spec.whatwg.org/#concept-ipv6-serializer
    fermat::KString ipv6(const std::array<uint16_t, 8> &address) noexcept;

    /// Serializes an ipv4 address.
    /// @details An IPv4 address is a 32-bit unsigned integer that identifies a
    /// network address.
    /// @see https://url.spec.whatwg.org/#concept-ipv4-serializer
    fermat::KString ipv4(uint64_t address) noexcept;

} // namespace fermat::uri::serializers
