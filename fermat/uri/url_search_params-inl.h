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
/// @file url_search_params-inl.h
/// @brief Inline declarations for the URL Search Params

#pragma once

#include <fermat/uri.h>
#include <fermat/uri/character_sets-inl.h>
#include <fermat/uri/unicode.h>
#include <fermat/uri/url_search_params.h>

#include <algorithm>
#include <optional>
#include <fermat/container/string.h>
#include <fermat/container/vector.h>
#include <string_view>


namespace fermat::uri {
    // A default, empty UrlSearchParams for use with empty iterators.
    template<typename T, fermat::uri::UrlSearchParamsIterType Type>
    UrlSearchParams UrlSearchParamsIter<T, Type>::EMPTY;

    inline void UrlSearchParams::reset(std::string_view input) {
        params.clear();
        initialize(input);
    }

    inline void UrlSearchParams::initialize(std::string_view input) {
        if (!input.empty() && input.front() == '?') {
            input.remove_prefix(1);
        }

        auto process_key_value = [&](const std::string_view current) {
            auto equal = current.find('=');

            if (equal == std::string_view::npos) {
                fermat::KString name(current);
                std::replace(name.begin(), name.end(), '+', ' ');
                params.emplace_back(unicode::percent_decode(name, name.find('%')), "");
            } else {
                fermat::KString name(current.substr(0, equal));
                fermat::KString value(current.substr(equal + 1));

                std::replace(name.begin(), name.end(), '+', ' ');
                std::replace(value.begin(), value.end(), '+', ' ');

                params.emplace_back(unicode::percent_decode(name, name.find('%')),
                                    unicode::percent_decode(value, value.find('%')));
            }
        };

        while (!input.empty()) {
            auto ampersand_index = input.find('&');

            if (ampersand_index == std::string_view::npos) {
                if (!input.empty()) {
                    process_key_value(input);
                }
                break;
            } else if (ampersand_index != 0) {
                process_key_value(input.substr(0, ampersand_index));
            }

            input.remove_prefix(ampersand_index + 1);
        }
    }

    inline void UrlSearchParams::append(const std::string_view key,
                                        const std::string_view value) {
        params.emplace_back(key, value);
    }

    inline size_t UrlSearchParams::size() const noexcept { return params.size(); }

    inline std::optional<std::string_view> UrlSearchParams::get(
        const std::string_view key) {
        auto entry = std::find_if(params.begin(), params.end(),
                                  [&key](auto &param) { return param.first == key; });

        if (entry == params.end()) {
            return std::nullopt;
        }

        return entry->second;
    }

    inline fermat::Vector<fermat::KString> UrlSearchParams::get_all(
        const std::string_view key) {
        fermat::Vector<fermat::KString> out{};

        for (auto &param: params) {
            if (param.first == key) {
                out.emplace_back(param.second);
            }
        }

        return out;
    }

    inline bool UrlSearchParams::has(const std::string_view key) noexcept {
        auto entry = std::find_if(params.begin(), params.end(),
                                  [&key](auto &param) { return param.first == key; });
        return entry != params.end();
    }

    inline bool UrlSearchParams::has(std::string_view key,
                                     std::string_view value) noexcept {
        auto entry =
                std::find_if(params.begin(), params.end(), [&key, &value](auto &param) {
                    return param.first == key && param.second == value;
                });
        return entry != params.end();
    }

    inline fermat::KString UrlSearchParams::to_string() const {
        auto character_set = fermat::uri::character_sets::WWW_FORM_URLENCODED_PERCENT_ENCODE;
        fermat::KString out{};
        for (size_t i = 0; i < params.size(); i++) {
            auto key = fermat::uri::unicode::percent_encode(params[i].first, character_set);
            auto value = fermat::uri::unicode::percent_encode(params[i].second, character_set);

            // Performance optimization: Move this inside percent_encode.
            std::replace(key.begin(), key.end(), ' ', '+');
            std::replace(value.begin(), value.end(), ' ', '+');

            if (i != 0) {
                out += "&";
            }
            out.append(key);
            out += "=";
            out.append(value);
        }
        return out;
    }

    inline void UrlSearchParams::set(const std::string_view key,
                                     const std::string_view value) {
        const auto find = [&key](auto &param) { return param.first == key; };

        auto it = std::find_if(params.begin(), params.end(), find);

        if (it == params.end()) {
            params.emplace_back(key, value);
        } else {
            it->second = value;
            params.erase(std::remove_if(std::next(it), params.end(), find),
                         params.end());
        }
    }

    inline void UrlSearchParams::remove(const std::string_view key) {
        params.erase(
            std::remove_if(params.begin(), params.end(),
                           [&key](auto &param) { return param.first == key; }),
            params.end());
    }

    inline void UrlSearchParams::remove(const std::string_view key,
                                        const std::string_view value) {
        params.erase(std::remove_if(params.begin(), params.end(),
                                    [&key, &value](auto &param) {
                                        return param.first == key &&
                                               param.second == value;
                                    }),
                     params.end());
    }

    inline void UrlSearchParams::sort() {
        std::stable_sort(params.begin(), params.end(),
                         [](const key_value_pair &lhs, const key_value_pair &rhs) {
                             return lhs.first < rhs.first;
                         });
    }

    inline UrlSearchParamsKeysIter UrlSearchParams::get_keys() {
        return UrlSearchParamsKeysIter(*this);
    }

    /// @see https://url.spec.whatwg.org/#interface-urlsearchparams
    inline url_search_params_values_iter UrlSearchParams::get_values() {
        return url_search_params_values_iter(*this);
    }

    /// @see https://url.spec.whatwg.org/#interface-urlsearchparams
    inline url_search_params_entries_iter UrlSearchParams::get_entries() {
        return url_search_params_entries_iter(*this);
    }

    template<typename T, UrlSearchParamsIterType Type>
    inline bool UrlSearchParamsIter<T, Type>::has_next() {
        return pos < params.params.size();
    }

    template<>
    inline std::optional<std::string_view> UrlSearchParamsKeysIter::next() {
        if (!has_next()) {
            return std::nullopt;
        }
        return params.params[pos++].first;
    }

    template<>
    inline std::optional<std::string_view> url_search_params_values_iter::next() {
        if (!has_next()) {
            return std::nullopt;
        }
        return params.params[pos++].second;
    }

    template<>
    inline std::optional<key_value_view_pair>
    url_search_params_entries_iter::next() {
        if (!has_next()) {
            return std::nullopt;
        }
        return params.params[pos++];
    }
} // namespace fermat::uri
