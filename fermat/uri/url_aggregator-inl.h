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
/// @file url_aggregator-inl.h
/// @brief Inline functions for url aggregator

#pragma once

#include <fermat/uri/character_sets.h>
#include <fermat/uri/character_sets-inl.h>
#include <fermat/uri/checkers-inl.h>
#include <fermat/uri/helpers.h>
#include <fermat/uri/unicode.h>
#include <fermat/uri/unicode-inl.h>
#include <fermat/uri/url_aggregator.h>
#include <fermat/uri/url_components.h>
#include <fermat/uri/scheme.h>
#include <fermat/uri/log.h>

#include <optional>
#include <string_view>

namespace fermat::uri {
    inline void UrlAggregator::update_base_authority(
        std::string_view base_buffer, const fermat::uri::UrlComponents &base) {
        std::string_view input = base_buffer.substr(
            base.protocol_end, base.host_start - base.protocol_end);
        ada_log("UrlAggregator::update_base_authority ", input);

        bool input_starts_with_dash = checkers::begins_with(input, "//");
        uint32_t diff = components.host_start - components.protocol_end;

        buffer.erase(components.protocol_end,
                     components.host_start - components.protocol_end);
        components.username_end = components.protocol_end;

        if (input_starts_with_dash) {
            input.remove_prefix(2);
            diff += 2; // add "//"
            buffer.insert(components.protocol_end, "//");
            components.username_end += 2;
        }

        size_t password_delimiter = input.find(':');

        // Check if input contains both username and password by checking the
        // delimiter: ":" A typical input that contains authority would be "user:pass"
        if (password_delimiter != std::string_view::npos) {
            // Insert both username and password
            std::string_view username = input.substr(0, password_delimiter);
            std::string_view password = input.substr(password_delimiter + 1);

            buffer.insert(components.protocol_end + diff, username);
            diff += uint32_t(username.size());
            buffer.insert(components.protocol_end + diff, ":");
            components.username_end = components.protocol_end + diff;
            buffer.insert(components.protocol_end + diff + 1, password);
            diff += uint32_t(password.size()) + 1;
        } else if (!input.empty()) {
            // Insert only username
            buffer.insert(components.protocol_end + diff, input);
            components.username_end =
                    components.protocol_end + diff + uint32_t(input.size());
            diff += uint32_t(input.size());
        }

        components.host_start += diff;

        if (buffer.size() > base.host_start && buffer[base.host_start] != '@') {
            buffer.insert(components.host_start, "@");
            diff++;
        }
        components.host_end += diff;
        components.pathname_start += diff;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += diff;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += diff;
        }
    }

    inline void UrlAggregator::update_unencoded_base_hash(std::string_view input) {
        ada_log("UrlAggregator::update_unencoded_base_hash ", input, " [",
                input.size(), " bytes], buffer is '", buffer, "' [", buffer.size(),
                " bytes] components.hash_start = ", components.hash_start);
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));
        if (components.hash_start != UrlComponents::omitted) {
            buffer.resize(components.hash_start);
        }
        components.hash_start = uint32_t(buffer.size());
        buffer += "#";
        bool encoding_required = unicode::percent_encode<true>(
            input, fermat::uri::character_sets::FRAGMENT_PERCENT_ENCODE, buffer);
        // When encoding_required is false, then buffer is left unchanged, and percent
        // encoding was not deemed required.
        if (!encoding_required) {
            buffer.append(input);
        }
        ada_log("UrlAggregator::update_unencoded_base_hash final buffer is '",
                buffer, "' [", buffer.size(), " bytes]");
        DKCHECK(validate());
    }

    TURBO_FORCE_INLINE uint32_t UrlAggregator::replace_and_resize(
        uint32_t start, uint32_t end, std::string_view input) {
        uint32_t current_length = end - start;
        uint32_t input_size = uint32_t(input.size());
        uint32_t new_difference = input_size - current_length;

        if (current_length == 0) {
            buffer.insert(start, input);
        } else if (input_size == current_length) {
            buffer.replace(start, input_size, input);
        } else if (input_size < current_length) {
            buffer.erase(start, current_length - input_size);
            buffer.replace(start, input_size, input);
        } else {
            buffer.replace(start, current_length, input.substr(0, current_length));
            buffer.insert(start + current_length, input.substr(current_length));
        }

        return new_difference;
    }

    inline void UrlAggregator::update_base_hostname(const std::string_view input) {
        ada_log("UrlAggregator::update_base_hostname ", input, " [", input.size(),
                " bytes], buffer is '", buffer, "' [", buffer.size(), " bytes]");
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));

        // This next line is required for when parsing a URL like `foo://`
        add_authority_slashes_if_needed();

        bool has_credentials = components.protocol_end + 2 < components.host_start;
        uint32_t new_difference =
                replace_and_resize(components.host_start, components.host_end, input);

        if (has_credentials) {
            buffer.insert(components.host_start, "@");
            new_difference++;
        }
        components.host_end += new_difference;
        components.pathname_start += new_difference;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += new_difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += new_difference;
        }
        DKCHECK(validate());
    }

    [[nodiscard]] TURBO_FORCE_INLINE uint32_t

    UrlAggregator::get_pathname_length() const noexcept {
        ada_log("UrlAggregator::get_pathname_length");
        uint32_t ending_index = uint32_t(buffer.size());
        if (components.search_start != UrlComponents::omitted) {
            ending_index = components.search_start;
        } else if (components.hash_start != UrlComponents::omitted) {
            ending_index = components.hash_start;
        }
        return ending_index - components.pathname_start;
    }

    [[nodiscard]] TURBO_FORCE_INLINE bool UrlAggregator::is_at_path()
    const noexcept {
        return buffer.size() == components.pathname_start;
    }

    inline void UrlAggregator::update_base_search(std::string_view input) {
        ada_log("UrlAggregator::update_base_search ", input);
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));
        if (input.empty()) {
            clear_search();
            return;
        }

        if (input[0] == '?') {
            input.remove_prefix(1);
        }

        if (components.hash_start == UrlComponents::omitted) {
            if (components.search_start == UrlComponents::omitted) {
                components.search_start = uint32_t(buffer.size());
                buffer += "?";
            } else {
                buffer.resize(components.search_start + 1);
            }

            buffer.append(input);
        } else {
            if (components.search_start == UrlComponents::omitted) {
                components.search_start = components.hash_start;
            } else {
                buffer.erase(components.search_start,
                             components.hash_start - components.search_start);
                components.hash_start = components.search_start;
            }

            buffer.insert(components.search_start, "?");
            buffer.insert(components.search_start + 1, input);
            components.hash_start += uint32_t(input.size() + 1); // Do not forget `?`
        }

        DKCHECK(validate());
    }

    inline void UrlAggregator::update_base_search(
        std::string_view input, const uint8_t query_percent_encode_set[]) {
        ada_log("UrlAggregator::update_base_search ", input,
                " with encoding parameter ", to_string(), "\n", to_diagram());
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));

        if (components.hash_start == UrlComponents::omitted) {
            if (components.search_start == UrlComponents::omitted) {
                components.search_start = uint32_t(buffer.size());
                buffer += "?";
            } else {
                buffer.resize(components.search_start + 1);
            }

            bool encoding_required =
                    unicode::percent_encode<true>(input, query_percent_encode_set, buffer);
            // When encoding_required is false, then buffer is left unchanged, and
            // percent encoding was not deemed required.
            if (!encoding_required) {
                buffer.append(input);
            }
        } else {
            if (components.search_start == UrlComponents::omitted) {
                components.search_start = components.hash_start;
            } else {
                buffer.erase(components.search_start,
                             components.hash_start - components.search_start);
                components.hash_start = components.search_start;
            }

            buffer.insert(components.search_start, "?");
            size_t idx =
                    fermat::uri::unicode::percent_encode_index(input, query_percent_encode_set);
            if (idx == input.size()) {
                buffer.insert(components.search_start + 1, input);
                components.hash_start += uint32_t(input.size() + 1); // Do not forget `?`
            } else {
                buffer.insert(components.search_start + 1, input, 0, idx);
                input.remove_prefix(idx);
                // We only create a temporary string if we need percent encoding and
                // we attempt to create as small a temporary string as we can.
                fermat::KString encoded =
                        fermat::uri::unicode::percent_encode(input, query_percent_encode_set);
                buffer.insert(components.search_start + idx + 1, encoded);
                components.hash_start +=
                        uint32_t(encoded.size() + idx + 1); // Do not forget `?`
            }
        }

        DKCHECK(validate());
    }

    inline void UrlAggregator::update_base_pathname(const std::string_view input) {
        ada_log("UrlAggregator::update_base_pathname '", input, "' [", input.size(),
                " bytes] \n", to_diagram());
        DKCHECK(!helpers::overlaps(input, buffer));
        DKCHECK(validate());

        const bool begins_with_dashdash = checkers::begins_with(input, "//");
        if (!begins_with_dashdash && has_dash_dot()) {
            ada_log("UrlAggregator::update_base_pathname has /.: \n", to_diagram());
            // We must delete the ./
            delete_dash_dot();
        }

        if (begins_with_dashdash && !has_opaque_path && !has_authority() &&
            !has_dash_dot()) {
            // If url's host is null, url does not have an opaque path, url's path's
            // size is greater than 1, then append U+002F (/) followed by U+002E (.) to
            // output.
            buffer.insert(components.pathname_start, "/.");
            components.pathname_start += 2;
        }

        uint32_t difference = replace_and_resize(
            components.pathname_start,
            components.pathname_start + get_pathname_length(), input);
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += difference;
        }
        ada_log("UrlAggregator::update_base_pathname end '", input, "' [",
                input.size(), " bytes] \n", to_diagram());
        DKCHECK(validate());
    }

    inline void UrlAggregator::append_base_pathname(const std::string_view input) {
        ada_log("UrlAggregator::append_base_pathname ", input, " ", to_string(),
                "\n", to_diagram());
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));
#if ADA_DEVELOPMENT_CHECKS
        // computing the expected password.
        fermat::KString path_expected(get_pathname());
        path_expected.append(input);
#endif  // ADA_DEVELOPMENT_CHECKS
        uint32_t ending_index = uint32_t(buffer.size());
        if (components.search_start != UrlComponents::omitted) {
            ending_index = components.search_start;
        } else if (components.hash_start != UrlComponents::omitted) {
            ending_index = components.hash_start;
        }
        buffer.insert(ending_index, input);

        if (components.search_start != UrlComponents::omitted) {
            components.search_start += uint32_t(input.size());
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += uint32_t(input.size());
        }
#if ADA_DEVELOPMENT_CHECKS
        DKCHECK_EQ(path_expected, fermat::KString(get_pathname())) << "append_base_pathname problem after inserting "<<input;
#endif
        DKCHECK(validate());
    }

    inline void UrlAggregator::update_base_username(const std::string_view input) {
        ada_log("UrlAggregator::update_base_username '", input, "' ", to_string(),
                "\n", to_diagram());
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));

        add_authority_slashes_if_needed();

        bool has_password = has_non_empty_password();
        bool host_starts_with_at = buffer.size() > components.host_start &&
                                   buffer[components.host_start] == '@';
        uint32_t diff = replace_and_resize(components.protocol_end + 2,
                                           components.username_end, input);

        components.username_end += diff;
        components.host_start += diff;

        if (!input.empty() && !host_starts_with_at) {
            buffer.insert(components.host_start, "@");
            diff++;
        } else if (input.empty() && host_starts_with_at && !has_password) {
            // Input is empty, there is no password, and we need to remove "@" from
            // hostname
            buffer.erase(components.host_start, 1);
            diff--;
        }

        components.host_end += diff;
        components.pathname_start += diff;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += diff;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += diff;
        }
        DKCHECK(validate());
    }

    inline void UrlAggregator::append_base_username(const std::string_view input) {
        ada_log("UrlAggregator::append_base_username ", input);
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));
#if ADA_DEVELOPMENT_CHECKS
        // computing the expected password.
        fermat::KString username_expected(get_username());
        username_expected.append(input);
#endif  // ADA_DEVELOPMENT_CHECKS
        add_authority_slashes_if_needed();

        // If input is empty, do nothing.
        if (input.empty()) {
            return;
        }

        uint32_t difference = uint32_t(input.size());
        buffer.insert(components.username_end, input);
        components.username_end += difference;
        components.host_start += difference;

        if (buffer[components.host_start] != '@' &&
            components.host_start != components.host_end) {
            buffer.insert(components.host_start, "@");
            difference++;
        }

        components.host_end += difference;
        components.pathname_start += difference;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += difference;
        }
#if ADA_DEVELOPMENT_CHECKS
        DKCHECK_EQ(username_expected, fermat::KString(get_username()))<<"append_base_username problem after inserting "<<input;
#endif

        DKCHECK(validate());
    }

    inline void UrlAggregator::clear_password() {
        ada_log("UrlAggregator::clear_password ", to_string(), "\n", to_diagram());
        DKCHECK(validate());
        if (!has_password()) {
            return;
        }

        uint32_t diff = components.host_start - components.username_end;
        buffer.erase(components.username_end, diff);
        components.host_start -= diff;
        components.host_end -= diff;
        components.pathname_start -= diff;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start -= diff;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start -= diff;
        }
    }

    inline void UrlAggregator::update_base_password(const std::string_view input) {
        ada_log("UrlAggregator::update_base_password ", input);
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));

        add_authority_slashes_if_needed();

        // TODO: Optimization opportunity. Merge the following removal functions.
        if (input.empty()) {
            clear_password();

            // Remove username too, if it is empty.
            if (!has_non_empty_username()) {
                update_base_username("");
            }

            return;
        }

        bool password_exists = has_password();
        uint32_t difference = uint32_t(input.size());

        if (password_exists) {
            uint32_t current_length =
                    components.host_start - components.username_end - 1;
            buffer.erase(components.username_end + 1, current_length);
            difference -= current_length;
        } else {
            buffer.insert(components.username_end, ":");
            difference++;
        }

        buffer.insert(components.username_end + 1, input);
        components.host_start += difference;

        // The following line is required to add "@" to hostname. When updating
        // password if hostname does not start with "@", it is "update_base_password"s
        // responsibility to set it.
        if (buffer[components.host_start] != '@') {
            buffer.insert(components.host_start, "@");
            difference++;
        }

        components.host_end += difference;
        components.pathname_start += difference;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += difference;
        }
        DKCHECK(validate());
    }

    inline void UrlAggregator::append_base_password(const std::string_view input) {
        ada_log("UrlAggregator::append_base_password ", input, " ", to_string(),
                "\n", to_diagram());
        DKCHECK(validate());
        DKCHECK(!helpers::overlaps(input, buffer));
#if ADA_DEVELOPMENT_CHECKS
        // computing the expected password.
        fermat::KString password_expected = fermat::KString(get_password());
        password_expected.append(input);
#endif  // ADA_DEVELOPMENT_CHECKS
        add_authority_slashes_if_needed();

        // If input is empty, do nothing.
        if (input.empty()) {
            return;
        }

        uint32_t difference = uint32_t(input.size());
        if (has_password()) {
            buffer.insert(components.host_start, input);
        } else {
            difference++; // Increment for ":"
            buffer.insert(components.username_end, ":");
            buffer.insert(components.username_end + 1, input);
        }
        components.host_start += difference;

        // The following line is required to add "@" to hostname. When updating
        // password if hostname does not start with "@", it is "append_base_password"s
        // responsibility to set it.
        if (buffer[components.host_start] != '@') {
            buffer.insert(components.host_start, "@");
            difference++;
        }

        components.host_end += difference;
        components.pathname_start += difference;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += difference;
        }
#if ADA_DEVELOPMENT_CHECKS
        DKCHECK_EQ(password_expected, fermat::KString(get_password()))<<"append_base_password problem after inserting "<<input;
#endif

        DKCHECK(validate());
    }

    inline void UrlAggregator::update_base_port(uint32_t input) {
        ada_log("UrlAggregator::update_base_port");
        DKCHECK(validate());
        if (input == UrlComponents::omitted) {
            clear_port();
            return;
        }
        // calling std::to_string(input.value()) is unfortunate given that the port
        // value is probably already available as a string.
        fermat::KString value = helpers::concat(":", std::to_string(input));
        uint32_t difference = uint32_t(value.size());

        if (components.port != UrlComponents::omitted) {
            difference -= components.pathname_start - components.host_end;
            buffer.erase(components.host_end,
                         components.pathname_start - components.host_end);
        }

        buffer.insert(components.host_end, value);
        components.pathname_start += difference;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += difference;
        }
        components.port = input;
        DKCHECK(validate());
    }

    inline void UrlAggregator::clear_port() {
        ada_log("UrlAggregator::clear_port");
        DKCHECK(validate());
        if (components.port == UrlComponents::omitted) {
            return;
        }
        uint32_t length = components.pathname_start - components.host_end;
        buffer.erase(components.host_end, length);
        components.pathname_start -= length;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start -= length;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start -= length;
        }
        components.port = UrlComponents::omitted;
        DKCHECK(validate());
    }

    [[nodiscard]] inline uint32_t UrlAggregator::retrieve_base_port() const {
        ada_log("UrlAggregator::retrieve_base_port");
        return components.port;
    }

    inline void UrlAggregator::clear_search() {
        ada_log("UrlAggregator::clear_search");
        DKCHECK(validate());
        if (components.search_start == UrlComponents::omitted) {
            return;
        }

        if (components.hash_start == UrlComponents::omitted) {
            buffer.resize(components.search_start);
        } else {
            buffer.erase(components.search_start,
                         components.hash_start - components.search_start);
            components.hash_start = components.search_start;
        }

        components.search_start = UrlComponents::omitted;

        DKCHECK_EQ(get_search(), "")<<"search should have been cleared on buffer=" + buffer +
                         " with " + components.to_string() + "\n" + to_diagram();
        DKCHECK(validate());
    }

    inline void UrlAggregator::clear_hash() {
        ada_log("UrlAggregator::clear_hash");
        DKCHECK(validate());
        if (components.hash_start == UrlComponents::omitted) {
            return;
        }
        buffer.resize(components.hash_start);
        components.hash_start = UrlComponents::omitted;

        DKCHECK_EQ(get_hash(), "")<<
                         "hash should have been cleared on buffer=" + buffer +
                         " with " + components.to_string() + "\n" + to_diagram();
        DKCHECK(validate());
    }

    inline void UrlAggregator::clear_pathname() {
        ada_log("UrlAggregator::clear_pathname");
        DKCHECK(validate());
        uint32_t ending_index = uint32_t(buffer.size());
        if (components.search_start != UrlComponents::omitted) {
            ending_index = components.search_start;
        } else if (components.hash_start != UrlComponents::omitted) {
            ending_index = components.hash_start;
        }
        uint32_t pathname_length = ending_index - components.pathname_start;
        buffer.erase(components.pathname_start, pathname_length);
        uint32_t difference = pathname_length;
        if (components.pathname_start == components.host_end + 2 &&
            buffer[components.host_end] == '/' &&
            buffer[components.host_end + 1] == '.') {
            components.pathname_start -= 2;
            buffer.erase(components.host_end, 2);
            difference += 2;
        }
        if (components.search_start != UrlComponents::omitted) {
            components.search_start -= difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start -= difference;
        }
        ada_log("UrlAggregator::clear_pathname completed, running checks...");

        DKCHECK_EQ(get_pathname(), "")<<
                         "pathname should have been cleared on buffer=" + buffer +
                         " with " + components.to_string() + "\n" + to_diagram();
        DKCHECK(validate());
        ada_log("UrlAggregator::clear_pathname completed, running checks... ok");
    }

    inline void UrlAggregator::clear_hostname() {
        ada_log("UrlAggregator::clear_hostname");
        DKCHECK(validate());
        if (!has_authority()) {
            return;
        }
        DKCHECK(has_authority());

        uint32_t hostname_length = components.host_end - components.host_start;
        uint32_t start = components.host_start;

        // If hostname starts with "@", we should not remove that character.
        if (hostname_length > 0 && buffer[start] == '@') {
            start++;
            hostname_length--;
        }
        buffer.erase(start, hostname_length);
        components.host_end = start;
        components.pathname_start -= hostname_length;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start -= hostname_length;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start -= hostname_length;
        }
        DKCHECK_EQ(get_hostname(), "")<<
                         "hostname should have been cleared on buffer=" + buffer +
                         " with " + components.to_string() + "\n" + to_diagram();
        DKCHECK(has_authority());
        DKCHECK_EQ(has_empty_hostname(), true)<<
                         "hostname should have been cleared on buffer=" + buffer +
                         " with " + components.to_string() + "\n" + to_diagram();
        DKCHECK(validate());
    }

    [[nodiscard]] inline bool UrlAggregator::has_hash() const noexcept {
        ada_log("UrlAggregator::has_hash");
        return components.hash_start != UrlComponents::omitted;
    }

    [[nodiscard]] inline bool UrlAggregator::has_search() const noexcept {
        ada_log("UrlAggregator::has_search");
        return components.search_start != UrlComponents::omitted;
    }

    TURBO_FORCE_INLINE bool UrlAggregator::has_credentials() const noexcept {
        ada_log("UrlAggregator::has_credentials");
        return has_non_empty_username() || has_non_empty_password();
    }

    inline bool UrlAggregator::cannot_have_credentials_or_port() const {
        ada_log("UrlAggregator::cannot_have_credentials_or_port");
        return type == fermat::uri::scheme::type::FILE ||
               components.host_start == components.host_end;
    }

    [[nodiscard]] TURBO_FORCE_INLINE const fermat::uri::UrlComponents &
    UrlAggregator::get_components() const noexcept {
        return components;
    }

    [[nodiscard]] inline bool fermat::uri::UrlAggregator::has_authority() const noexcept {
        ada_log("UrlAggregator::has_authority");
        // Performance: instead of doing this potentially expensive check, we could
        // have a boolean in the struct.
        return components.protocol_end + 2 <= components.host_start &&
               helpers::substring(buffer, components.protocol_end,
                                  components.protocol_end + 2) == "//";
    }

    inline void fermat::uri::UrlAggregator::add_authority_slashes_if_needed() noexcept {
        ada_log("UrlAggregator::add_authority_slashes_if_needed");
        DKCHECK(validate());
        // Protocol setter will insert `http:` to the URL. It is up to hostname setter
        // to insert
        // `//` initially to the buffer, since it depends on the hostname existence.
        if (has_authority()) {
            return;
        }
        // Performance: the common case is components.protocol_end == buffer.size()
        // Optimization opportunity: in many cases, the "//" is part of the input and
        // the insert could be fused with another insert.
        buffer.insert(components.protocol_end, "//");
        components.username_end += 2;
        components.host_start += 2;
        components.host_end += 2;
        components.pathname_start += 2;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += 2;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += 2;
        }
        DKCHECK(validate());
    }

    inline void fermat::uri::UrlAggregator::reserve(uint32_t capacity) {
        buffer.reserve(capacity);
    }

    inline bool UrlAggregator::has_non_empty_username() const noexcept {
        ada_log("UrlAggregator::has_non_empty_username");
        return components.protocol_end + 2 < components.username_end;
    }

    inline bool UrlAggregator::has_non_empty_password() const noexcept {
        ada_log("UrlAggregator::has_non_empty_password");
        return components.host_start - components.username_end > 0;
    }

    inline bool UrlAggregator::has_password() const noexcept {
        ada_log("UrlAggregator::has_password");
        // This function does not care about the length of the password
        return components.host_start > components.username_end &&
               buffer[components.username_end] == ':';
    }

    inline bool UrlAggregator::has_empty_hostname() const noexcept {
        if (!has_hostname()) {
            return false;
        }
        if (components.host_start == components.host_end) {
            return true;
        }
        if (components.host_end > components.host_start + 1) {
            return false;
        }
        return components.username_end != components.host_start;
    }

    inline bool UrlAggregator::has_hostname() const noexcept {
        return has_authority();
    }

    inline bool UrlAggregator::has_port() const noexcept {
        ada_log("UrlAggregator::has_port");
        // A URL cannot have a username/password/port if its host is null or the empty
        // string, or its scheme is "file".
        return has_hostname() && components.pathname_start != components.host_end;
    }

    [[nodiscard]] inline bool UrlAggregator::has_dash_dot() const noexcept {
        // If url's host is null, url does not have an opaque path, url's path's size
        // is greater than 1, and url's path[0] is the empty string, then append
        // U+002F (/) followed by U+002E (.) to output.
        ada_log("UrlAggregator::has_dash_dot");
#if ADA_DEVELOPMENT_CHECKS
        // If pathname_start and host_end are exactly two characters apart, then we
        // either have a one-digit port such as http://test.com:5?param=1 or else we
        // have a /.: sequence such as "non-spec:/.//". We test that this is the case.
        if (components.pathname_start == components.host_end + 2) {
            DKCHECK((buffer[components.host_end] == '/' &&
                    buffer[components.host_end + 1] == '.') ||
                (buffer[components.host_end] == ':' &&
                    checkers::is_digit(buffer[components.host_end + 1])));
        }
        if (components.pathname_start == components.host_end + 2 &&
            buffer[components.host_end] == '/' &&
            buffer[components.host_end + 1] == '.') {
            DKCHECK(components.pathname_start + 1 < buffer.size());
            DKCHECK(buffer[components.pathname_start] == '/');
            DKCHECK(buffer[components.pathname_start + 1] == '/');
        }
#endif
        // Performance: it should be uncommon for components.pathname_start ==
        // components.host_end + 2 to be true. So we put this check first in the
        // sequence. Most times, we do not have an opaque path. Checking for '/.' is
        // more expensive, but should be uncommon.
        return components.pathname_start == components.host_end + 2 &&
               !has_opaque_path && buffer[components.host_end] == '/' &&
               buffer[components.host_end + 1] == '.';
    }

    [[nodiscard]] inline std::string_view UrlAggregator::get_href() const noexcept

    TURBO_ATTRIBUTE_LIFETIME_BOUND {
        ada_log("UrlAggregator::get_href");
        return buffer;
    }

    TURBO_FORCE_INLINE size_t UrlAggregator::parse_port(
        std::string_view view, bool check_trailing_content) noexcept {
        ada_log("UrlAggregator::parse_port('", view, "') ", view.size());
        if (!view.empty() && view[0] == '-') {
            ada_log("parse_port: view[0] == '0' && view.size() > 1");
            is_valid = false;
            return 0;
        }
        uint16_t parsed_port{};
        auto r = std::from_chars(view.data(), view.data() + view.size(), parsed_port);
        if (r.ec == std::errc::result_out_of_range) {
            ada_log("parse_port: r.ec == std::errc::result_out_of_range");
            is_valid = false;
            return 0;
        }
        ada_log("parse_port: ", parsed_port);
        const size_t consumed = size_t(r.ptr - view.data());
        ada_log("parse_port: consumed ", consumed);
        if (check_trailing_content) {
            is_valid &=
            (consumed == view.size() || view[consumed] == '/' ||
             view[consumed] == '?' || (is_special() && view[consumed] == '\\'));
        }
        ada_log("parse_port: is_valid = ", is_valid);
        if (is_valid) {
            ada_log("parse_port", r.ec == std::errc());
            // scheme_default_port can return 0, and we should allow 0 as a base port.
            auto default_port = scheme_default_port();
            bool is_port_valid = (default_port == 0 && parsed_port == 0) ||
                                 (default_port != parsed_port);
            if (r.ec == std::errc() && is_port_valid) {
                update_base_port(parsed_port);
            } else {
                clear_port();
            }
        }
        return consumed;
    }

    inline void UrlAggregator::set_protocol_as_file() {
        ada_log("UrlAggregator::set_protocol_as_file ");
        DKCHECK(validate());
        type = fermat::uri::scheme::type::FILE;
        // next line could overflow but unsigned arithmetic has well-defined
        // overflows.
        uint32_t new_difference = 5 - components.protocol_end;

        if (buffer.empty()) {
            buffer.append("file:");
        } else {
            buffer.erase(0, components.protocol_end);
            buffer.insert(0, "file:");
        }
        components.protocol_end = 5;

        // Update the rest of the components.
        components.username_end += new_difference;
        components.host_start += new_difference;
        components.host_end += new_difference;
        components.pathname_start += new_difference;
        if (components.search_start != UrlComponents::omitted) {
            components.search_start += new_difference;
        }
        if (components.hash_start != UrlComponents::omitted) {
            components.hash_start += new_difference;
        }
        DKCHECK(validate());
    }

    inline std::ostream &operator<<(std::ostream &out,
                                    const fermat::uri::UrlAggregator &u) {
        return out << u.to_string();
    }
} // namespace fermat::uri
