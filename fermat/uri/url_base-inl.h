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
/// @file url_base-inl.h
/// @brief Inline functions for url base

#pragma once

#include <fermat/uri/url_aggregator.h>
#include <fermat/uri/url_components.h>
#include <fermat/uri/scheme.h>
#include <fermat/uri/scheme-inl.h>
#include <fermat/uri/log.h>
#include <fermat/uri/checkers.h>
#include <fermat/uri/url.h>

#include <optional>
#include <fermat/container/string.h>
#if ADA_REGULAR_VISUAL_STUDIO
#include <intrin.h>
#endif  // ADA_REGULAR_VISUAL_STUDIO

namespace fermat::uri {

    [[nodiscard]] TURBO_FORCE_INLINE bool UrlBase::is_special() const noexcept {
        return type != fermat::uri::scheme::NOT_SPECIAL;
    }

    [[nodiscard]] inline uint16_t UrlBase::get_special_port() const noexcept {
        return fermat::uri::scheme::get_special_port(type);
    }

    [[nodiscard]] TURBO_FORCE_INLINE uint16_t

    UrlBase::scheme_default_port() const noexcept {
        return scheme::get_special_port(type);
    }

} // namespace fermat::uri
