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

#pragma once

#include <fermat/container/cord_buffer.h>
#include <fermat/container/string.h>
#include <turbo/base/endian.h>
#include <ostream>
#include <streambuf>
#include <cstring>
#include <fmt/format.h>

namespace fermat {
    template<size_t Alignment, size_t BlockSize>
    class CordFormatSink {
    public:
        using iterator_category = std::output_iterator_tag;
        using value_type = void;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = void;

        explicit CordFormatSink(CordBufferBase<Alignment, BlockSize> *cord) noexcept : cord_(cord) {
        }

        ~CordFormatSink() = default;

        CordFormatSink(const CordFormatSink &) = default;

        CordFormatSink &operator=(const CordFormatSink &) = default;

        CordFormatSink(CordFormatSink &&) = default;

        CordFormatSink &operator=(CordFormatSink &&) = default;

        // Output iterator requirements
        CordFormatSink &operator*() noexcept { return *this; }
        CordFormatSink &operator++() noexcept { return *this; }
        CordFormatSink operator++(int) noexcept { return *this; }

        // Write a single character
        CordFormatSink &operator=(char ch) {
            cord_->append(&ch, 1);
            return *this;
        }

        void append(fmt::string_view data) {
            cord_->append(data.data(), data.size());
        }


        // Bulk write (preferred by fmt)
        friend CordFormatSink &operator+=(CordFormatSink &sink, fmt::string_view data) {
            sink.cord_->append(data.data(), data.size());
            return sink;
        }

    private:
        CordBufferBase<Alignment, BlockSize> *cord_;
    };

    template<size_t Alignment, size_t BlockSize>
    class CordFormatter {
    public:
        explicit CordFormatter(CordBufferBase<Alignment, BlockSize>* cord) : _sink(cord) {}

        // Template function to format directly into the cord
        template<typename... Args>
        void format(fmt::format_string<Args...> fmt_str, Args&&... args) {
            fmt::format_to(_sink, fmt_str, std::forward<Args>(args)...);
        }

        template<typename T>
        CordFormatter&operator<<(const T& data) {
            fmt::format_to(_sink, "{}", data);
            return *this;
        }

        template <typename T>
        CordFormatter &operator+=(const T& data) {
            fmt::format_to(_sink, "{}", data);
            return *this;
        }

        // Return the underlying sink for direct use with fmt::format_to
        CordFormatSink<Alignment, BlockSize>& sink() noexcept { return _sink; }

    private:
        CordFormatSink<Alignment, BlockSize> _sink;
    };
} // namespace fermat
