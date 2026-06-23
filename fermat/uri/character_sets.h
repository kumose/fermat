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
/// @file character_sets.h
/// @brief Declaration of the character sets used by unicode functions.
/// @author Node.js
/// @see https://github.com/nodejs/node/blob/main/src/node_url_tables.cc

#pragma once

#include <fermat/uri/common_defs.h>
#include <cstdint>

/// These functions are not part of our public API and may
/// change at any time.
/// @private
/// @namespace fermat::uri::character_sets
/// @brief Includes the definitions for unicode character sets.
namespace fermat::uri::character_sets {
    TURBO_FORCE_INLINE bool bit_at(const uint8_t a[], uint8_t i);
} // namespace fermat::uri::character_sets
