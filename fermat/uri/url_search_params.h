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
/// @file url_search_params.h
/// @brief Declaration for the URL Search Params

#pragma once

#include <optional>
#include <fermat/container/string.h>
#include <string_view>
#include <fermat/container/vector.h>

namespace fermat::uri {
    enum class UrlSearchParamsIterType {
        KEYS,
        VALUES,
        ENTRIES,
    };

    template<typename T, UrlSearchParamsIterType Type>
    struct UrlSearchParamsIter;

    typedef std::pair<std::string_view, std::string_view> key_value_view_pair;

    using UrlSearchParamsKeysIter =
    UrlSearchParamsIter<std::string_view, UrlSearchParamsIterType::KEYS>;
    using url_search_params_values_iter =
    UrlSearchParamsIter<std::string_view,
        UrlSearchParamsIterType::VALUES>;
    using url_search_params_entries_iter =
    UrlSearchParamsIter<key_value_view_pair,
        UrlSearchParamsIterType::ENTRIES>;

    /// @see https://url.spec.whatwg.org/#interface-urlsearchparams
    struct UrlSearchParams {
        UrlSearchParams() = default;

        /// @see
        /// https://github.com/web-platform-tests/wpt/blob/master/url/urlsearchparams-constructor.any.js
        UrlSearchParams(const std::string_view input) { initialize(input); }

        UrlSearchParams(const UrlSearchParams &u) = default;

        UrlSearchParams(UrlSearchParams &&u) noexcept = default;

        UrlSearchParams &operator=(UrlSearchParams &&u) noexcept = default;

        UrlSearchParams &operator=(const UrlSearchParams &u) = default;

        ~UrlSearchParams() = default;

        [[nodiscard]] inline size_t size() const noexcept;

        /// @see https://url.spec.whatwg.org/#dom-urlsearchparams-append
        inline void append(std::string_view key, std::string_view value);

        /// @see https://url.spec.whatwg.org/#dom-urlsearchparams-delete
        inline void remove(std::string_view key);

        inline void remove(std::string_view key, std::string_view value);

        /// @see https://url.spec.whatwg.org/#dom-urlsearchparams-get
        inline std::optional<std::string_view> get(std::string_view key);

        /// @see https://url.spec.whatwg.org/#dom-urlsearchparams-getall
        inline fermat::Vector<fermat::KString> get_all(std::string_view key);

        /// @see https://url.spec.whatwg.org/#dom-urlsearchparams-has
        inline bool has(std::string_view key) noexcept;

        inline bool has(std::string_view key, std::string_view value) noexcept;

        /// @see https://url.spec.whatwg.org/#dom-urlsearchparams-set
        inline void set(std::string_view key, std::string_view value);

        /// @see https://url.spec.whatwg.org/#dom-urlsearchparams-sort
        inline void sort();

        /// @see https://url.spec.whatwg.org/#urlsearchparams-stringification-behavior
        inline fermat::KString to_string() const;

        /// Returns a simple JS-style iterator over all of the keys in this
        /// UrlSearchParams. The keys in the iterator are not unique. The valid
        /// lifespan of the iterator is tied to the UrlSearchParams. The iterator
        /// must be freed when you're done with it.
        /// @see https://url.spec.whatwg.org/#interface-urlsearchparams
        inline UrlSearchParamsKeysIter get_keys();

        /// Returns a simple JS-style iterator over all of the values in this
        /// UrlSearchParams. The valid lifespan of the iterator is tied to the
        /// UrlSearchParams. The iterator must be freed when you're done with it.
        /// @see https://url.spec.whatwg.org/#interface-urlsearchparams
        inline url_search_params_values_iter get_values();

        /// Returns a simple JS-style iterator over all of the entries in this
        /// UrlSearchParams. The entries are pairs of keys and corresponding values.
        /// The valid lifespan of the iterator is tied to the UrlSearchParams. The
        /// iterator must be freed when you're done with it.
        /// @see https://url.spec.whatwg.org/#interface-urlsearchparams
        inline url_search_params_entries_iter get_entries();

        /// C++ style conventional iterator support. const only because we
        /// do not really want the params to be modified via the iterator.
        inline auto begin() const { return params.begin(); }
        inline auto end() const { return params.end(); }
        inline auto front() const { return params.front(); }
        inline auto back() const { return params.back(); }
        inline auto operator[](size_t index) const { return params[index]; }

        /// @private
        /// Used to reset the search params to a new input.
        /// Used primarily for C API.
        /// @param input
        void reset(std::string_view input);

    private:
        typedef std::pair<fermat::KString, fermat::KString> key_value_pair;
        fermat::Vector<key_value_pair> params{};

        /// @see https://url.spec.whatwg.org/#concept-urlencoded-parser
        void initialize(std::string_view init);

        template<typename T, UrlSearchParamsIterType Type>
        friend struct UrlSearchParamsIter;
    }; // UrlSearchParams

    /// Implements a non-conventional iterator pattern that is closer in style to
    /// JavaScript's definition of an iterator.
    ///
    /// @see https://webidl.spec.whatwg.org/#idl-iterable
    template<typename T, UrlSearchParamsIterType Type>
    struct UrlSearchParamsIter {
        inline UrlSearchParamsIter() : params(EMPTY) {
        }

        UrlSearchParamsIter(const UrlSearchParamsIter &u) = default;

        UrlSearchParamsIter(UrlSearchParamsIter &&u) noexcept = default;

        UrlSearchParamsIter &operator=(UrlSearchParamsIter &&u) noexcept = default;

        UrlSearchParamsIter &operator=(const UrlSearchParamsIter &u) = default;

        ~UrlSearchParamsIter() = default;

        /// Return the next item in the iterator or std::nullopt if done.
        inline std::optional<T> next();

        inline bool has_next();

    private:
        static UrlSearchParams EMPTY;

        inline UrlSearchParamsIter(UrlSearchParams &params_) : params(params_) {
        }

        UrlSearchParams &params;
        size_t pos = 0;

        friend struct UrlSearchParams;
    };
} // namespace fermat::uri
