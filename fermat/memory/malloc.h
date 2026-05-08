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
#include <mimalloc.h>
#include <turbo/utility/status.h>

namespace fermat {
    static constexpr size_t kDefaultPageSize = 4096;
    static constexpr size_t kDefaultAlignedSize = 64;

    struct Malloc {
        static size_t good_alloc_size(size_t n) {
            return mi_good_size(n );
        }

        static void *good_alloc(size_t *n) {
            auto rn = good_alloc_size(*n);
            *n = rn;
            return mi_malloc(rn);
        }

        static void good_free(void *ptr, size_t n) {
            if (ptr == nullptr) {
                return;
            }
            mi_free_size(ptr, n);
        }

        static void good_free(void *ptr) {
            if (ptr == nullptr) {
                return;
            }
            mi_free(ptr);
        }

        static void *good_realloc(void *p, size_t *n) {
            auto rn = good_alloc_size(*n);
            *n = rn;
            return mi_recalloc(p, rn, 1);
        }

        static size_t good_usable_size(void *p) {
            if (p == nullptr) {
                return 0;
            }
            return mi_malloc_usable_size(p);
        }

    };

    template<size_t Alignment>
    struct AlignedMalloc {
        static_assert(Alignment > 0 && (Alignment & (Alignment - 1)) == 0, "Alignment must be a power of 2");

        static size_t good_alloc_size(size_t n) {
            return mi_good_size((n + Alignment - 1) & ~(Alignment - 1));
        }

        static void *good_alloc(size_t *n) {
            auto rn = good_alloc_size(*n);
            *n = rn;
            return mi_aligned_alloc(Alignment, rn);
        }

        static void good_free(void *ptr, size_t n) {
            if (ptr == nullptr) {
                return;
            }
            mi_free_size_aligned(ptr, n, Alignment);
        }

        static void good_free(void *ptr) {
            if (ptr == nullptr) {
                return;
            }
            mi_free_aligned(ptr, Alignment);
        }

        static void *good_realloc(void *p, size_t *n) {
            auto rn = good_alloc_size(*n);
            *n = rn;
            return mi_aligned_recalloc(p, rn, 1, Alignment);
        }

        static size_t good_usable_size(void *p) {
            if (p == nullptr) {
                return 0;
            }
            return mi_malloc_usable_size(p);
        }

        /// @brief Check if a pointer is aligned to the template Alignment.
        /// @param p The pointer to check.
        /// @return True if the pointer is aligned.
        static bool is_aligned(const void *p) noexcept {
            return (reinterpret_cast<uintptr_t>(p) & (Alignment - 1)) == 0;
        }

        /// @brief Check if the size is a multiple of the template Alignment.
        /// @param n The size to check.
        static bool is_aligned_size(size_t n) noexcept {
            return (n & (Alignment - 1)) == 0;
        }
    };
} // namespace fermat
