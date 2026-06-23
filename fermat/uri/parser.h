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
/// @file parser.h
/// @brief Definitions for the parser.

#pragma once

#include <optional>
#include <string_view>

#include <fermat/uri/encoding_type.h>
#include <fermat/container/expected.h>
#include <fermat/uri/state.h>

/// @private
namespace fermat::uri {
    struct UrlAggregator;
    struct Url;
} // namespace fermat::uri

/// @namespace fermat::uri::parser
/// @brief Includes the definitions for supported parsers
namespace fermat::uri::parser {
    /// Parses a url. The parameter user_input is the input to be parsed:
    /// it should be a valid UTF-8 string. The parameter base_url is an optional
    /// parameter that can be used to resolve relative URLs. If the base_url is
    /// provided, the user_input is resolved against the base_url.
    template<typename result_type = fermat::uri::UrlAggregator>
    result_type parse_url(std::string_view user_input,
                          const result_type *base_url = nullptr);

    extern template UrlAggregator parse_url<UrlAggregator>(
        std::string_view user_input, const UrlAggregator *base_url);

    extern template Url parse_url<Url>(std::string_view user_input,
                                       const Url *base_url);

    template<typename result_type = fermat::uri::UrlAggregator, bool store_values = true>
    result_type parse_url_impl(std::string_view user_input,
                               const result_type *base_url = nullptr);

    extern template UrlAggregator parse_url_impl<UrlAggregator>(
        std::string_view user_input, const UrlAggregator *base_url);

    extern template Url parse_url_impl<Url>(std::string_view user_input,
                                            const Url *base_url);


} // namespace fermat::uri::parser
