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

#include <cstddef>
#include <cstdint>
#include <turbo/utility/status.h>

namespace fermat {
    static constexpr size_t kDefaultPageSize = 4096;
    static constexpr size_t kDefaultAlignedSize = 64;

    struct Malloc {
        static size_t good_alloc_size(size_t n);

        static void *good_alloc(size_t n);

        static void good_free(void *ptr, size_t n);

        static void good_free(void *ptr);

        /// alignment
        static size_t good_align_alloc_size(size_t align, size_t n);

        static void *good_align_alloc(size_t align, size_t n);

        static void good_align_free(void *ptr, size_t align, size_t n);

        static void good_align_free(void *ptr, size_t align);

        /// @brief Check if a pointer is aligned to the template Alignment.
        /// @param p The pointer to check.
        /// @return True if the pointer is aligned.
        static bool is_aligned(const void *p, size_t align) noexcept {
            return (reinterpret_cast<uintptr_t>(p) & (align - 1)) == 0;
        }

        /// @brief Check if the size is a multiple of the template Alignment.
        /// @param n The size to check.
        static bool is_aligned_size(size_t n, size_t align) noexcept {
            return (n & (align - 1)) == 0;
        }

        template<size_t Alignment, typename T>
        static size_t good_type_size() {
            if constexpr (Alignment == 0) {
                return sizeof(T);
            } else {
                return good_align_alloc_size(Alignment, sizeof(T));
            }
        }

        template<size_t Alignment>
        static constexpr bool is_valid_alignment() noexcept {
            if constexpr (Alignment == 0) {
                return true;
            } else if constexpr (Alignment == 16) {
                return true;
            } else if constexpr (Alignment == 32) {
                return true;
            } else if constexpr (Alignment == 64) {
                return true;
            } else if constexpr (Alignment == 128) {
                return true;
            } else if constexpr (Alignment == 256) {
                return true;
            } else if constexpr (Alignment == 512) {
                return true;
            } else if constexpr (Alignment == 1024) {
                return true;
            } else if constexpr (Alignment == 2048) {
                return true;
            } else if constexpr (Alignment == 4096) {
                return true;
            }
            return false;
        }
    };
} // namespace fermat
