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

#include <turbo/container/span.h>
#include <fermat/container/stl.h>
#include <fermat/container/traits.h>
#include <fermat/container/receiver.h>
#include <fermat/memory/allocator.h>
#include <type_traits>
#include <turbo/log/check.h>
#include <turbo/log/logging.h>
#include <turbo/utility/status.h>
#include <fermat/io/iobuf.h>
#include <fermat/io/peeker.h>

#include "iobuf.h"

namespace fermat {

    /// @brief Specialization for dynamic and fixed containers via ContainerTraits.
    template<bool Custom>
    class FlattenCustomer {
    public:
        /// @brief Consumes up to `n` bytes from the source IOBuf and appends them to the target container.
        ///        If `n` exceeds the available data, only the existing bytes are read (no error).
        /// @param source The source IOBuf (data is read from its logical head).
        /// @param target The destination container (must satisfy ContainerTraits).
        /// @param n Maximum number of bytes to read.
        /// @return OkStatus on success (even if fewer than `n` bytes were read).
        template<size_t Alignment, size_t BlockSize>
        static turbo::Status custom(IOBuf<Alignment, BlockSize> &source, Receiver &target, size_t n) {
            if (n == 0) return turbo::OkStatus();

            // Limit to what is actually available
            size_t readable = std::min(n, source.size());
            if (readable == 0) return turbo::OkStatus();

            // Reserve capacity if the container is dynamic
            size_t total_needed = target.size() + readable;
            auto status = target.reserve(total_needed);
            if (!status.ok()) return status;

            // Use Peeker for zero‑copy, cross‑block reading
            Peeker<IOBuf<Alignment, BlockSize>> peeker(&source);
            size_t remaining = readable;
            while (remaining > 0) {
                auto chunk = peeker.readn(remaining);
                TURBO_RETURN_NOT_OK(
                    target.append(chunk->data(), chunk->size()));
                remaining -= chunk->size();
            }

            // If Custom is true, physically consume the bytes from the source
            if constexpr (Custom) {
                size_t consumed = readable - remaining;
                if (consumed > 0) {
                    TURBO_RETURN_NOT_OK(source.custom(consumed));
                }
            }
            return turbo::OkStatus();
        }

        template<size_t Alignment, size_t BlockSize>
        static turbo::Status custom_until(IOBuf<Alignment, BlockSize> &source, Receiver &target, char c) {
            // Use Peeker for zero‑copy, cross‑block reading
            Peeker<IOBuf<Alignment, BlockSize>> peeker(&source);
            auto offset = peeker.find_first_offset(c);
            auto n = source.size();
            auto cus_n = 0;
            if (offset != Peeker<IOBuf<Alignment, BlockSize>>::kNPos) {
                n = offset;
                cus_n = 1;
            }
            if (n == 0) return turbo::OkStatus();

            // Limit to what is actually available
            size_t readable = std::min(n, source.size());
            if (readable == 0) return turbo::OkStatus();

            size_t total_needed = target.size() + readable;
            auto status = target.reserve(total_needed);
            if (!status.ok()) return status;

            size_t remaining = readable;
            while (remaining > 0) {
                auto chunk = peeker.readn(remaining);
                /// reach c
                if (chunk->empty()) {
                    break;
                }
                TURBO_RETURN_NOT_OK(
                    target.append(chunk->data(), chunk->size()));
                remaining -= chunk->size();
            }

            // If Custom is true, physically consume the bytes from the source
            if constexpr (Custom) {
                size_t consumed = readable - remaining + cus_n;
                if (consumed > 0) {
                    TURBO_RETURN_NOT_OK(source.custom(consumed));
                }
            }
            return turbo::OkStatus();
        }
    };


    /// @brief Alias for a consumer that physically removes data from the source.
    using Customer = FlattenCustomer<true>;

    /// @brief Alias for a reader that only copies data, leaving the source untouched.
    using Reader = FlattenCustomer<false>;

} // namespace fermat
