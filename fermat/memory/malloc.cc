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

#include <fermat/memory/malloc.h>
#include <fermat/version.h>
#include <cstdlib>
#if USE_MIMALLOC
#include <mimalloc.h>
#endif

namespace fermat {
#if    USE_MIMALLOC
    size_t Malloc::good_alloc_size(size_t n) {
        return ::mi_good_size(n);
    }

    void *Malloc::good_alloc(size_t n) {
        return ::mi_malloc(n);
    }

    void Malloc::good_free(void *ptr, size_t n) {
        mi_free_size(ptr, n);
    }

    void Malloc::good_free(void *ptr) {
        mi_free(ptr);
    }

    size_t Malloc::good_align_alloc_size(size_t align, size_t n) {
        return mi_good_size((n + align - 1) & ~(align - 1));
    }

    void *Malloc::good_align_alloc(size_t align, size_t n) {
        return mi_aligned_alloc(align, n);
    }

    void Malloc::good_align_free(void *ptr, size_t align, size_t n) {
        mi_free_size_aligned(ptr, n, align);
    }

    void Malloc::good_align_free(void *ptr, size_t align) {
        mi_free_aligned(ptr, align);
    }

#else
    size_t Malloc::good_alloc_size(size_t n) {
        return n;
    }

    void *Malloc::good_alloc(size_t n) {
        return ::malloc(n);
    }

    void Malloc::good_free(void *ptr, size_t n) {
        TURBO_UNUSED(n);
        ::free(ptr);
    }
    void Malloc::good_free(void *ptr) {
        ::free(ptr);
    }

    size_t Malloc::good_align_alloc_size(size_t align, size_t n) {
        return (n + align - 1) & ~(align - 1);
    }

    void *Malloc::good_align_alloc(size_t align, size_t n) {
        return ::aligned_alloc(align, n);
    }

    void Malloc::good_align_free(void *ptr, size_t align, size_t n) {
        TURBO_UNUSED(align);
        TURBO_UNUSED(n);
        ::free(ptr);
    }

    void Malloc::good_align_free(void *ptr, size_t align) {
        TURBO_UNUSED(align);
        ::free(ptr);
    }

#endif
} // namespace fermat
