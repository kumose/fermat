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

#include <utility>
#include <iterator>
#include <cstring>
#include <numeric>

namespace fermat::container_internal {
    template<class InIt, class OutIt>
    inline std::pair<InIt, OutIt> copy_n(
        InIt b, typename std::iterator_traits<InIt>::difference_type n, OutIt d) {
        for (; n != 0; --n, ++b, ++d) {
            *d = *b;
        }
        return std::make_pair(b, d);
    }

    template<class Pod, class T>
    inline void podFill(Pod *b, Pod *e, T c) {
        assert(b && e && b <= e);
        constexpr auto kUseMemset = sizeof(T) == 1;
        if constexpr (kUseMemset) {
            memset(b, c, size_t(e - b));
        } else {
            auto const ee = b + ((e - b) & ~7u);
            for (; b != ee; b += 8) {
                b[0] = c;
                b[1] = c;
                b[2] = c;
                b[3] = c;
                b[4] = c;
                b[5] = c;
                b[6] = c;
                b[7] = c;
            }
            // Leftovers
            for (; b != e; ++b) {
                *b = c;
            }
        }
    }

    /*
     * Lightly structured memcpy, simplifies copying PODs and introduces
     * some asserts. Unfortunately using this function may cause
     * measurable overhead (presumably because it adjusts from a begin/end
     * convention to a pointer/size convention, so it does some extra
     * arithmetic even though the caller might have done the inverse
     * adaptation outside).
     */
    template<class Pod>
    inline void podCopy(const Pod *b, const Pod *e, Pod *d) {
        assert(b != nullptr);
        assert(e != nullptr);
        assert(d != nullptr);
        assert(e >= b);
        assert(d >= e || d + (e - b) <= b);
        memcpy(d, b, (e - b) * sizeof(Pod));
    }

    /*
     * Lightly structured memmove, simplifies copying PODs and introduces
     * some asserts
     */
    template<class Pod>
    inline void podMove(const Pod *b, const Pod *e, Pod *d) {
        assert(e >= b);
        memmove(d, b, (e - b) * sizeof(*b));
    }

    template<class SizeType, class IntSourceType>
    inline void AssertValueFitsInType(IntSourceType n, const char *assertMessage) {
        TURBO_UNUSED(n);
        TURBO_UNUSED(assertMessage);

        if constexpr (std::is_signed_v<IntSourceType>)
            KCHECK(n >= 0) << "Attempting to initialize/insert a Buffer with a negative number of elements!";

        [[maybe_unused]] constexpr bool kSizeTypeMaxIsEnough =
                static_cast<uintmax_t>(std::numeric_limits<IntSourceType>::max()) <=
                static_cast<uintmax_t>(std::numeric_limits<SizeType>::max());
        KCHECK(
            kSizeTypeMaxIsEnough || static_cast<IntSourceType>(std::numeric_limits<SizeType>::max()) >=
            n) << assertMessage;
    }
} // namespace fermat
