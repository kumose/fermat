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
/// @file unicode-inl.h
/// @brief Definitions for unicode operations.

#pragma once

#include <algorithm>
#include <fermat/uri/unicode.h>

/// Unicode operations. These functions are not part of our public API and may
/// change at any time.
///
/// private
/// @namespace fermat::uri::unicode
/// @brief Includes the declarations for unicode operations
namespace fermat::uri::unicode {
    TURBO_FORCE_INLINE size_t percent_encode_index(const std::string_view input,
                                                  const uint8_t character_set[]) {
        return std::distance(
            input.begin(),
            std::find_if(input.begin(), input.end(), [character_set](const char c) {
                return character_sets::bit_at(character_set, c);
            }));
    }
} // namespace fermat::uri::unicode
