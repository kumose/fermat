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
/// @file url.h
/// @brief Declaration for the URL

#pragma once

#include <algorithm>
#include <charconv>
#include <iostream>
#include <optional>
#include <fermat/container/string.h>
#include <string_view>

#include <fermat/uri/checkers.h>
#include <fermat/uri/common_defs.h>
#include <fermat/uri/log.h>
#include <fermat/uri/scheme.h>
#include <fermat/uri/serializers.h>
#include <fermat/uri/unicode.h>
#include <fermat/uri/url_base.h>
#include <fermat/uri/url_components.h>

namespace fermat::uri {
    /// @brief Generic URL struct reliant on fermat::KString instantiation.
    ///
    /// @details To disambiguate from a valid URL string it can also be referred to
    /// as a URL record. A URL is a struct that represents a universal identifier.
    /// Unlike the UrlAggregator, the fermat::uri::url represents the different components
    /// of a parsed URL as independent fermat::KString instances. This makes the
    /// structure heavier and more reliant on memory allocations. When getting
    /// components from the parsed URL, a new fermat::KString is typically constructed.
    ///
    /// @see https://url.spec.whatwg.org/#url-representation
    struct Url : UrlBase {
        Url() = default;

        Url(const Url &u) = default;

        Url(Url &&u) noexcept = default;

        Url &operator=(Url &&u) noexcept = default;

        Url &operator=(const Url &u) = default;

        ~Url() override = default;

        /// @private
        /// A URL's username is an ASCII string identifying a username. It is initially
        /// the empty string.
        fermat::KString username{};

        /// @private
        /// A URL's password is an ASCII string identifying a password. It is initially
        /// the empty string.
        fermat::KString password{};

        /// @private
        /// A URL's host is null or a host. It is initially null.
        std::optional<fermat::KString> host{};

        /// @private
        /// A URL's port is either null or a 16-bit unsigned integer that identifies a
        /// networking port. It is initially null.
        std::optional<uint16_t> port{};

        /// @private
        /// A URL's path is either an ASCII string or a list of zero or more ASCII
        /// strings, usually identifying a location.
        fermat::KString path{};

        /// @private
        /// A URL's query is either null or an ASCII string. It is initially null.
        std::optional<fermat::KString> query{};

        /// @private
        /// A URL's fragment is either null or an ASCII string that can be used for
        /// further processing on the resource the URL's other components identify. It
        /// is initially null.
        std::optional<fermat::KString> hash{};

        /// @return true if it has an host but it is the empty string
        [[nodiscard]] inline bool has_empty_hostname() const noexcept;

        /// @return true if the URL has a (non default) port
        [[nodiscard]] inline bool has_port() const noexcept;

        /// @return true if it has a host (included an empty host)
        [[nodiscard]] inline bool has_hostname() const noexcept;

        [[nodiscard]] bool has_valid_domain() const noexcept override;

        /// Returns a JSON string representation of this URL.
        [[nodiscard]] fermat::KString to_string() const override;

        /// @see https://url.spec.whatwg.org/#dom-url-href
        /// @see https://url.spec.whatwg.org/#concept-url-serializer
        [[nodiscard]] TURBO_FORCE_INLINE fermat::KString get_href() const noexcept;

        /// The origin getter steps are to return the serialization of this's URL's
        /// origin. [HTML]
        /// @return a newly allocated string.
        /// @see https://url.spec.whatwg.org/#concept-url-origin
        [[nodiscard]] fermat::KString get_origin() const noexcept override;

        /// The protocol getter steps are to return this's URL's scheme, followed by
        /// U+003A (:).
        /// @return a newly allocated string.
        /// @see https://url.spec.whatwg.org/#dom-url-protocol
        [[nodiscard]] fermat::KString get_protocol() const noexcept;

        /// Return url's host, serialized, followed by U+003A (:) and url's port,
        /// serialized.
        /// When there is no host, this function returns the empty string.
        /// @return a newly allocated string.
        /// @see https://url.spec.whatwg.org/#dom-url-host
        [[nodiscard]] fermat::KString get_host() const noexcept;

        /// Return this's URL's host, serialized.
        /// When there is no host, this function returns the empty string.
        /// @return a newly allocated string.
        /// @see https://url.spec.whatwg.org/#dom-url-hostname
        [[nodiscard]] fermat::KString get_hostname() const noexcept;

        /// The pathname getter steps are to return the result of URL path serializing
        /// this's URL.
        /// @return a newly allocated string.
        /// @see https://url.spec.whatwg.org/#dom-url-pathname
        [[nodiscard]] std::string_view get_pathname() const noexcept;

        /// Compute the pathname length in bytes without instantiating a view or a
        /// string.
        /// @return size of the pathname in bytes
        /// @see https://url.spec.whatwg.org/#dom-url-pathname
        [[nodiscard]] TURBO_FORCE_INLINE size_t get_pathname_length() const noexcept;

        /// Return U+003F (?), followed by this's URL's query.
        /// @return a newly allocated string.
        /// @see https://url.spec.whatwg.org/#dom-url-search
        [[nodiscard]] fermat::KString get_search() const noexcept;

        /// The username getter steps are to return this's URL's username.
        /// @return a constant reference to the underlying string.
        /// @see https://url.spec.whatwg.org/#dom-url-username
        [[nodiscard]] const fermat::KString &get_username() const noexcept;

        /// @return Returns true on successful operation.
        /// @see https://url.spec.whatwg.org/#dom-url-username
        bool set_username(std::string_view input);

        /// @return Returns true on success.
        /// @see https://url.spec.whatwg.org/#dom-url-password
        bool set_password(std::string_view input);

        /// @return Returns true on success.
        /// @see https://url.spec.whatwg.org/#dom-url-port
        bool set_port(std::string_view input);

        /// This function always succeeds.
        /// @see https://url.spec.whatwg.org/#dom-url-hash
        void set_hash(std::string_view input);

        /// This function always succeeds.
        /// @see https://url.spec.whatwg.org/#dom-url-search
        void set_search(std::string_view input);

        /// @return Returns true on success.
        /// @see https://url.spec.whatwg.org/#dom-url-search
        bool set_pathname(std::string_view input);

        /// @return Returns true on success.
        /// @see https://url.spec.whatwg.org/#dom-url-host
        bool set_host(std::string_view input);

        /// @return Returns true on success.
        /// @see https://url.spec.whatwg.org/#dom-url-hostname
        bool set_hostname(std::string_view input);

        /// @return Returns true on success.
        /// @see https://url.spec.whatwg.org/#dom-url-protocol
        bool set_protocol(std::string_view input);

        /// @see https://url.spec.whatwg.org/#dom-url-href
        bool set_href(std::string_view input);

        /// The password getter steps are to return this's URL's password.
        /// @return a constant reference to the underlying string.
        /// @see https://url.spec.whatwg.org/#dom-url-password
        [[nodiscard]] const fermat::KString &get_password() const noexcept;

        /// Return this's URL's port, serialized.
        /// @return a newly constructed string representing the port.
        /// @see https://url.spec.whatwg.org/#dom-url-port
        [[nodiscard]] fermat::KString get_port() const noexcept;

        /// Return U+0023 (#), followed by this's URL's fragment.
        /// @return a newly constructed string representing the hash.
        /// @see https://url.spec.whatwg.org/#dom-url-hash
        [[nodiscard]] fermat::KString get_hash() const noexcept;

        /// A URL includes credentials if its username or password is not the empty
        /// string.
        [[nodiscard]] TURBO_FORCE_INLINE bool has_credentials() const noexcept;

        /// Useful for implementing efficient serialization for the URL.
        ///
        /// https://user:pass@example.com:1234/foo/bar?baz#quux
        ///       |     |    |          | ^^^^|       |   |
        ///       |     |    |          | |   |       |   `----- hash_start
        ///       |     |    |          | |   |       `--------- search_start
        ///       |     |    |          | |   `----------------- pathname_start
        ///       |     |    |          | `--------------------- port
        ///       |     |    |          `----------------------- host_end
        ///       |     |    `---------------------------------- host_start
        ///       |     `--------------------------------------- username_end
        ///       `--------------------------------------------- protocol_end
        ///
        /// Inspired after servo/url
        ///
        /// @return a newly constructed component.
        ///
        /// @see
        /// https://github.com/servo/rust-url/blob/b65a45515c10713f6d212e6726719a020203cc98/url/src/quirks.rs#L31
        [[nodiscard]] TURBO_FORCE_INLINE fermat::uri::UrlComponents get_components()
        const noexcept;

        /// @return true if the URL has a hash component
        [[nodiscard]] inline bool has_hash() const noexcept override;

        /// @return true if the URL has a search component
        [[nodiscard]] inline bool has_search() const noexcept override;

    private:
        friend fermat::uri::Url fermat::uri::parser::parse_url<fermat::uri::Url>(std::string_view,
            const fermat::uri::Url *);

        friend fermat::uri::UrlAggregator fermat::uri::parser::parse_url<fermat::uri::UrlAggregator>(
            std::string_view, const fermat::uri::UrlAggregator *);

        friend void fermat::uri::helpers::strip_trailing_spaces_from_opaque_path<fermat::uri::Url>(
            fermat::uri::Url &url) noexcept;

        friend fermat::uri::Url fermat::uri::parser::parse_url_impl<fermat::uri::Url, true>(std::string_view,
            const fermat::uri::Url *);

        friend fermat::uri::UrlAggregator fermat::uri::parser::parse_url_impl<
            fermat::uri::UrlAggregator, true>(std::string_view, const fermat::uri::UrlAggregator *);

        inline void update_unencoded_base_hash(std::string_view input);

        inline void update_base_hostname(std::string_view input);

        inline void update_base_search(std::string_view input);

        inline void update_base_search(std::string_view input,
                                       const uint8_t query_percent_encode_set[]);

        inline void update_base_search(std::optional<fermat::KString> input);

        inline void update_base_pathname(std::string_view input);

        inline void update_base_username(std::string_view input);

        inline void update_base_password(std::string_view input);

        inline void update_base_port(std::optional<uint16_t> input);

        /// Sets the host or hostname according to override condition.
        /// Return true on success.
        /// @see https://url.spec.whatwg.org/#hostname-state
        template<bool override_hostname = false>
        bool set_host_or_hostname(std::string_view input);

        /// Return true on success.
        /// @see https://url.spec.whatwg.org/#concept-ipv4-parser
        [[nodiscard]] bool parse_ipv4(std::string_view input);

        /// Return true on success.
        /// @see https://url.spec.whatwg.org/#concept-ipv6-parser
        [[nodiscard]] bool parse_ipv6(std::string_view input);

        /// Return true on success.
        /// @see https://url.spec.whatwg.org/#concept-opaque-host-parser
        [[nodiscard]] bool parse_opaque_host(std::string_view input);

        /// A URL's scheme is an ASCII string that identifies the type of URL and can
        /// be used to dispatch a URL for further processing after parsing. It is
        /// initially the empty string. We only set non_special_scheme when the scheme
        /// is non-special, otherwise we avoid constructing string.
        ///
        /// Special schemes are stored in fermat::uri::scheme::details::is_special_list so we
        /// typically do not need to store them in each url instance.
        fermat::KString non_special_scheme{};

        /// A URL cannot have a username/password/port if its host is null or the empty
        /// string, or its scheme is "file".
        [[nodiscard]] inline bool cannot_have_credentials_or_port() const;

        TURBO_FORCE_INLINE size_t parse_port(
            std::string_view view, bool check_trailing_content) noexcept override;

        TURBO_FORCE_INLINE size_t parse_port(std::string_view view) noexcept override {
            return this->parse_port(view, false);
        }

        /// Take the scheme from another URL. The scheme string is copied from the
        /// provided url.
        inline void copy_scheme(const fermat::uri::Url &u);

        /// Parse the host from the provided input. We assume that
        /// the input does not contain spaces or tabs. Control
        /// characters and spaces are not trimmed (they should have
        /// been removed if needed).
        /// Return true on success.
        /// @see https://url.spec.whatwg.org/#host-parsing
        [[nodiscard]] TURBO_FORCE_INLINE bool parse_host(std::string_view input);

        template<bool has_state_override = false>
        [[nodiscard]] TURBO_FORCE_INLINE bool parse_scheme(std::string_view input);

        inline void clear_pathname() override;

        inline void clear_search() override;

        inline void set_protocol_as_file();

        /// Parse the path from the provided input.
        /// Return true on success. Control characters not
        /// trimmed from the ends (they should have
        /// been removed if needed).
        ///
        /// The input is expected to be UTF-8.
        ///
        /// @see https://url.spec.whatwg.org/
        TURBO_FORCE_INLINE void parse_path(std::string_view input);

        /// Set the scheme for this URL. The provided scheme should be a valid
        /// scheme string, be lower-cased, not contain spaces or tabs. It should
        /// have no spurious trailing or leading content.
        inline void set_scheme(fermat::KString &&new_scheme) noexcept;

        /// Take the scheme from another URL. The scheme string is moved from the
        /// provided url.
        inline void copy_scheme(fermat::uri::Url &&u) noexcept;
    }; // struct url

    inline std::ostream &operator<<(std::ostream &out, const fermat::uri::Url &u);
} // namespace fermat::uri
