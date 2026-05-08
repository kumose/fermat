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
#include <vector>
#include <turbo/container/span.h>
#include <turbo/utility/status.h>
namespace fermat {

    /// Category tags for compile-time dispatching.
    /// Used by Appender to select memory management strategies.
    struct IOBufTag {};

    /// Contiguous memory with auto-resize capability.
    struct DynamicTag {};

    /// Fixed address with hard capacity limits.
    struct FixedTag {};

    /// Fallback for unsupported types.
    struct UnknownTag {};

    /// Primary template for container traits.
    /// Inherits from UnknownTag by default.
    template <typename T, typename = void>
    struct ContainerTraits {
        using category = UnknownTag;
    };

    /// Trait specialization for fermat::IOBuf.
    ///
    /// Identified by the presence of 'is_iobuf' static member.
    /// This specialization enables high-performance state-machine based IO.
    template <typename T>
    struct ContainerTraits<T, std::enable_if_t<T::is_iobuf>> {
        using category = IOBufTag;

        static constexpr size_t block_size = T::kBlockSize;
        static constexpr size_t alignment  = T::kAlignment;

        /// Checks if 'Other' IOBuf can be referenced by this one.
        ///
        /// For zero-copy append:
        /// 1. Source (Other) alignment must be >= Target (Self) alignment.
        /// 2. Source (Other) alignment must be a multiple of Target (Self) alignment.
        /// 3. Source (Other) block size must be >= Target (Self) block size.
        template <typename Other>
        static constexpr bool is_compatible() {
            using OtherTraits = ContainerTraits<std::decay_t<Other>>;
            if constexpr (std::is_same_v<typename OtherTraits::category, IOBufTag>) {
                /// Provider's (Other) specs must cover consumer's (Self) requirements.
                return (OtherTraits::alignment >= alignment) &&
                       (OtherTraits::alignment % alignment == 0) &&
                       (OtherTraits::block_size >= block_size);
            }
            return false;
        }
    };

    /// Trait specialization for dynamic contiguous containers.
    ///
    /// These containers must provide .resize(), .data(), and .size()
    /// to simulate the "borrow" mechanism via contiguous memory expansion.
    template <typename T>
    struct ContainerTraits<T, std::enable_if_t<
        !std::is_same_v<typename ContainerTraits<T, void>::category, IOBufTag> &&
        std::is_invocable_v<decltype(&T::resize), T, size_t> &&
        std::is_invocable_v<decltype(&T::data), T> &&
        std::is_invocable_v<decltype(&T::size), T>
    >> {
        /// Assigned category for standard dynamic containers like std::string or std::vector.
        using category = DynamicTag;

        /// Helper to access data pointer for dynamic containers.
        static char* get_data(T* c) {
            return reinterpret_cast<char*>(c->data());
        }

        /// Helper to get current size for dynamic containers.
        static size_t get_size(const T* c) {
            return c->size();
        }
    };

    /// Trait specialization for fixed-capacity buffers.
    ///
    /// These containers possess a hard-coded memory limit and do not support
    /// automatic reallocation. They must provide .capacity(), .data(), and
    /// .set_size() to allow Appender to manage the write cursor manually.
    template <typename T>
    struct ContainerTraits<T, std::enable_if_t<
        !std::is_invocable_v<decltype(&T::resize), T, size_t> &&
        std::is_invocable_v<decltype(&T::capacity), T> &&
        std::is_invocable_v<decltype(&T::set_size), T, size_t> &&
        std::is_invocable_v<decltype(&T::data), T>
    >> {
        /// Assigned category for buffers with static or pre-allocated capacity.
        using category = FixedTag;

        /// Helper to access data pointer for fixed buffers.
        static char* get_data(T* c) {
            return reinterpret_cast<char*>(c->data());
        }

        /// Helper to get current logical size.
        static size_t get_size(const T* c) {
            return c->size();
        }

        /// Helper to get the maximum capacity.
        static size_t get_capacity(const T* c) {
            return c->capacity();
        }
    };


    template <typename Source, typename Container, typename Tag = void>
    class Appender;

    template <typename Source, typename Container>
        class Appender<Source,Container, std::enable_if_t<std::is_same_v<typename ContainerTraits<Container>::category,IOBufTag>, IOBufTag>> {
        public:

        static  turbo::Status pop_front(Source& source, Container&target, size_t size) ;

    };

}  // namespace fermat
