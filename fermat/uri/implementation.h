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
/// @file implementation.h
/// @brief Definitions for user facing functions for parsing URL and its
/// components.

#pragma once

#include <fermat/container/string.h>
#include <optional>

#include <fermat/uri/parser.h>
#include <fermat/uri/common_defs.h>
#include <fermat/uri/encoding_type.h>
#include <fermat/uri/url.h>
#include <fermat/uri/state.h>
#include <fermat/uri/url_aggregator.h>

namespace fermat::uri {
    enum class errors { generic_error };

    template<class result_type = fermat::uri::UrlAggregator>
    using result = fermat::expected<result_type, fermat::uri::errors>;

    /// The URL parser takes a scalar value string input, with an optional null or
    /// base URL base (default null). The parser assumes the input is a valid ASCII
    /// or UTF-8 string.
    ///
    /// @param input the string input to analyze (must be valid ASCII or UTF-8)
    /// @param base_url the optional URL input to use as a base url.
    /// @return a parsed URL.
    template<class result_type = fermat::uri::UrlAggregator>
    [[nodiscard]] fermat::uri::result<result_type> parse(
        std::string_view input, const result_type *base_url = nullptr);

    extern template fermat::uri::result<Url> parse<Url>(std::string_view input,
                                                        const Url *base_url);

    extern template fermat::uri::result<UrlAggregator> parse<UrlAggregator>(
        std::string_view input, const UrlAggregator *base_url);


    /// Verifies whether the URL strings can be parsed. The function assumes
    /// that the inputs are valid ASCII or UTF-8 strings.
    /// @see https://url.spec.whatwg.org/#dom-url-canparse
    /// @return If URL can be parsed or not.
    [[nodiscard]] bool can_parse(std::string_view input,
                                 const std::string_view *base_input = nullptr);

    /// Computes a href string from a file path. The function assumes
    /// that the input is a valid ASCII or UTF-8 string.
    /// @return a href string (starts with file:://)
    [[nodiscard]] fermat::KString href_from_file(std::string_view path);
} // namespace fermat::uri
