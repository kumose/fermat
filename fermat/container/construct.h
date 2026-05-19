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

#include <memory>

namespace fermat {

    /// std
    using std::uninitialized_copy;
    using std::uninitialized_copy_n;
    using std::uninitialized_move;
    using std::uninitialized_move_n;
    using std::uninitialized_default_construct;
    using std::uninitialized_default_construct_n;
    using std::uninitialized_value_construct;
    using std::uninitialized_value_construct_n;
    using std::destroy_at;
    using std::destroy;
    using std::destroy_n;
    using std::uninitialized_fill;
    using std::uninitialized_fill_n;


#if __cplusplus >= 202002L
    using std::construct_at;
#endif


#if __cplusplus < 202002L
    template <typename T, typename... Args>
    constexpr T* construct_at(T* p, Args&&... args) {
        return ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
    }
#endif

    /// uninitialized_copy_copy
    ///
    /// Copies [first1, last1) into [result, result + (last1 - first1)) then
    /// copies [first2, last2) into [result, result + (last1 - first1) + (last2 - first2)).
    ///
    template<typename InputIterator1, typename InputIterator2, typename ForwardIterator>
    inline ForwardIterator
    uninitialized_copy_copy(InputIterator1 first1, InputIterator1 last1,
                            InputIterator2 first2, InputIterator2 last2,
                            ForwardIterator result) {
        const ForwardIterator mid(std::uninitialized_copy(first1, last1, result));

        return std::uninitialized_copy(first2, last2, mid);
    }
}  // namespace fermat
