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

#include <cstdint>
#include <type_traits>

namespace fermat {

    template<typename T>
    class ReferenceObject {
    public:
        static_assert(std::is_same_v<decltype(std::declval<T>().reset_lose_memory()), void> &&
                  noexcept(std::declval<T>().reset_lose_memory()),
                  "T must have void reset_lose_memory() noexcept");

        enum class RefFlag : uint32_t{
            REF_NONE = 0,
            REF_UNIQUE = 1,
            REF_SHARED = 2,
        };
        struct ReferenceCount {
            RefFlag ref_flag{RefFlag::REF_NONE};
            uint32_t ref_count{0};
            T        object;
        };
        ReferenceObject() = default;
        ~ReferenceObject();

        ReferenceObject reference();

        ReferenceObject copy() const;

        void reset();
    protected:
        void add_ref();
        void release_ref();
    protected:
        ReferenceCount *_ptr{nullptr};
    };

} // namespace fermat
