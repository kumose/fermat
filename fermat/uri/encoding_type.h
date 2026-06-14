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
/// @file encoding_type.h
/// @brief Definition for supported encoding types.

#pragma once

#include <fermat/uri/common_defs.h>
#include <fermat/container/string.h>

namespace fermat::uri {
    /// This specification defines three encodings with the same names as encoding
    /// schemes defined in the Unicode standard: UTF-8, UTF-16LE, and UTF-16BE.
    ///
    /// @see https://encoding.spec.whatwg.org/#encodings
    enum class encoding_type {
        UTF8,
        UTF_16LE,
        UTF_16BE,
    };

    /// Convert a encoding_type to string.
    [[nodiscard]] fermat::KString to_string(encoding_type type);
} // namespace fermat::uri
