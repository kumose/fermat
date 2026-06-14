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
/// auto-generated on 2023-09-19 15:58:51 -0400. Do not edit!
/// @file idna.h

#pragma once

#include <fermat/container/string.h>
#include <string_view>

namespace fermat::uri::idna {
    size_t utf8_to_utf32(const char *buf, size_t len, char32_t *utf32_output);

    size_t utf8_length_from_utf32(const char32_t *buf, size_t len);

    size_t utf32_length_from_utf8(const char *buf, size_t len);

    size_t utf32_to_utf8(const char32_t *buf, size_t len, char *utf8_output);

    /// If the input is ascii, then the mapping is just -> lower case.
    void ascii_map(char *input, size_t length);

    /// check whether an ascii string needs mapping
    bool ascii_has_upper_case(char *input, size_t length);

    /// Map the characters according to IDNA, returning the empty string on error.
    std::u32string map(std::u32string_view input);

    /// Normalize the characters according to IDNA (Unicode Normalization Form C).
    void normalize(std::u32string &input);

    bool punycode_to_utf32(std::string_view input, std::u32string &out);

    bool verify_punycode(std::string_view input);

    bool utf32_to_punycode(std::u32string_view input, fermat::KString &out);

    /// @see https://www.unicode.org/reports/tr46/#Validity_Criteria
    bool is_label_valid(std::u32string_view label);

    /// Converts a domain (e.g., www.google.com) possibly containing international
    /// characters to an ascii domain (with punycode). It will not do percent
    /// decoding: percent decoding should be done prior to calling this function. We
    /// do not remove tabs and spaces, they should have been removed prior to calling
    /// this function. We also do not trim control characters. We also assume that
    /// the input is not empty. We return "" on error.
    ///
    ///
    /// This function may accept or even produce invalid domains.
    fermat::KString to_ascii(std::string_view ut8_string);

    /// Returns true if the string contains a forbidden code point according to the
    /// WHATGL URL specification:
    /// https://url.spec.whatwg.org/#forbidden-domain-code-point
    bool contains_forbidden_domain_code_point(std::string_view ascii_string);

    bool begins_with(std::u32string_view view, std::u32string_view prefix);

    bool begins_with(std::string_view view, std::string_view prefix);

    bool constexpr is_ascii(std::u32string_view view);

    bool constexpr is_ascii(std::string_view view);

    fermat::KString to_unicode(std::string_view input);
} // namespace fermat::uri::idna
