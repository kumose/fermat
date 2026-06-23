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
#include <turbo/base/macros.h>
#include <fermat/container/vector.h>

namespace fermat {
    /// radix_sort
    ///
    /// Implements a classic LSD (least significant digit) radix sort.
    /// See http://en.wikipedia.org/wiki/Radix_sort.
    /// This sort requires that the sorted data be of a type that has a member
    /// radix_type typedef and an mKey member of that type. The type must be
    /// an integral type. This limits what can be sorted, but radix_sort is
    /// very fast -- typically faster than any other sort.
    /// For example:
    ///     struct Sortable {
    ///         typedef int radix_type;
    ///         radix_type mKey;
    ///         // User data goes here, or the user can inherit from Sortable.
    ///     };
    /// or, more generally:
    ///     template <typname Integer>
    ///     struct Sortable {
    ///         typedef Integer radix_type;
    ///         Integer mKey;
    ///     };
    ///
    /// Example usage:
    ///     struct Element {
    ///         typedef uint16_t radix_type;
    ///         uint16_t mKey;
    ///         uint16_t mUserData;
    ///     };
    ///
    ///     Element elementArray[100];
    ///     Element buffer[100];
    ///
    ///     radix_sort<Element*, extract_radix_key<Element> >(elementArray, elementArray + 100, buffer);
    ///
    /// To consider: A static linked-list implementation may be faster than the version here.

    /// extract_radix_key
    ///
    /// Default radix sort integer value reader. It expects the sorted elements
    /// to have an integer member of type radix_type and of name "mKey".
    ///
    template<typename Node>
    struct extract_radix_key {
        typedef typename Node::radix_type radix_type;

        const radix_type operator()(const Node &x) const { return x.mKey; }
    };

    namespace Internal {
        // The radix_sort implementation uses two optimizations that are not part of a typical radix sort implementation.
        // 1. Computing a histogram (i.e. finding the number of elements per bucket) for the next pass is done in parallel with the loop that "scatters"
        //    elements in the current pass.  The advantage is that it avoids the memory traffic / cache pressure of reading keys in a separate operation.
        //    Note: It would also be possible to compute all histograms in a single pass.  However, that would increase the amount of stack space used and
        //    also increase cache pressure slightly.  However, it could still be faster under some situations.
        // 2. If all elements are mapped to a single bucket, then there is no need to perform a scatter operation.  Instead the elements are left in place
        //    and only copied if they need to be copied to the final output buffer.
        template<typename RandomAccessIterator, typename ExtractKey, int DigitBits, typename IntegerType>
        void radix_sort_impl(RandomAccessIterator first,
                             RandomAccessIterator last,
                             RandomAccessIterator buffer,
                             ExtractKey extractKey,
                             IntegerType) {
            RandomAccessIterator srcFirst = first;
            constexpr size_t numBuckets = 1 << DigitBits;
            constexpr IntegerType bucketMask = numBuckets - 1;

            // The alignment of this variable isn't required; it merely allows the code below to be faster on some platforms.
            uint32_t TURBO_CACHELINE_ALIGNED bucketSize[numBuckets];
            uint32_t TURBO_CACHELINE_ALIGNED bucketPosition[numBuckets];

            RandomAccessIterator temp;
            uint32_t i;
            bool doSeparateHistogramCalculation = true;

            constexpr uint32_t kMaxDigitBits = 8 * sizeof(IntegerType);
            for (uint32_t j = 0; j < kMaxDigitBits; j += DigitBits) {
                if (doSeparateHistogramCalculation) {
                    memset(bucketSize, 0, sizeof(bucketSize));
                    // Calculate histogram for the first scatter operation
                    for (temp = srcFirst; temp != last; ++temp)
                        ++bucketSize[(extractKey(*temp) >> j) & bucketMask];
                }

                // If a single bucket contains all of the elements, then don't bother redistributing all elements to the
                // same bucket.
                if (bucketSize[((extractKey(*srcFirst) >> j) & bucketMask)] == uint32_t(last - srcFirst)) {
                    // Set flag to ensure histogram is computed for next digit position.
                    doSeparateHistogramCalculation = true;
                } else {
                    // The histogram is either not needed or it will be calculated in parallel with the scatter operation below for better cache efficiency.
                    doSeparateHistogramCalculation = false;

                    // If this is the last digit position, then don't calculate a histogram
                    const uint32_t jNext = j + DigitBits;
                    if (jNext >= kMaxDigitBits) {
                        bucketPosition[0] = 0;
                        for (i = 0; i < numBuckets - 1; i++) {
                            bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];
                        }

                        for (temp = srcFirst; temp != last; ++temp) {
                            IntegerType key = extractKey(*temp);
                            const size_t digit = (key >> j) & bucketMask;
                            buffer[bucketPosition[digit]++] = *temp;
                        }
                    }
                    // Compute the histogram while performing the scatter operation
                    else {
                        bucketPosition[0] = 0;
                        for (i = 0; i < numBuckets - 1; i++) {
                            bucketPosition[i + 1] = bucketPosition[i] + bucketSize[i];
                            bucketSize[i] = 0; // Clear the bucket for the next pass
                        }
                        bucketSize[numBuckets - 1] = 0;

                        for (temp = srcFirst; temp != last; ++temp) {
                            const IntegerType key = extractKey(*temp);
                            const size_t digit = (key >> j) & bucketMask;
                            buffer[bucketPosition[digit]++] = *temp;

                            // Update histogram for the next scatter operation
                            ++bucketSize[(key >> jNext) & bucketMask];
                        }
                    }

                    last = buffer + (last - srcFirst);
                    temp = srcFirst;
                    srcFirst = buffer;
                    buffer = temp;
                }
            }

            if (srcFirst != first) {
                // Copy values back into the expected buffer
                for (temp = srcFirst; temp != last; ++temp)
                    *buffer++ = *temp;
            }
        }
    } // namespace Internal

    template<typename RandomAccessIterator, typename ExtractKey, int DigitBits = 8>
    void radix_sort(RandomAccessIterator first, RandomAccessIterator last, RandomAccessIterator buffer) {
        static_assert(DigitBits > 0, "DigitBits must be > 0");
        static_assert(DigitBits <= (sizeof(typename ExtractKey::radix_type) * 8),
                      "DigitBits must be <= the size of the key (in bits)");
        if (first == last) {
            return;
        }
        fermat::Internal::radix_sort_impl<RandomAccessIterator, ExtractKey, DigitBits>(
            first, last, buffer, ExtractKey(), typename ExtractKey::radix_type());
    }


    /// @brief Radix sort for integral types (in-place, requires temporary buffer).
    /// @tparam RandomAccessIterator Random access iterator whose value_type is integral.
    /// @tparam DigitBits Number of bits per digit (default 8, i.e., radix 256).
    /// @param first Start iterator.
    /// @param last End iterator.
    template<typename RandomAccessIterator,
        int DigitBits = 8,
        typename = std::enable_if_t<std::is_integral_v<typename std::iterator_traits<
            RandomAccessIterator>::value_type> > >
    void radix_sort(RandomAccessIterator first, RandomAccessIterator last) {
        using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
        using unsigned_type = std::make_unsigned_t<value_type>;

        auto n = static_cast<size_t>(last - first);
        if (n <= 1) return;

        // Convert to raw pointers (assumes contiguous memory, e.g., vector, array).
        value_type *ptr_begin = &*first;
        value_type *ptr_end = ptr_begin + n;

        // Allocate temporary buffer of raw pointers.
        fermat::Vector<value_type> buffer(n);
        value_type *buf_ptr = buffer.data();

        struct SelfEx {
            unsigned_type operator()(value_type x) const { return static_cast<unsigned_type>(x); }
        };

        // Call impl with raw pointers (all iterators are value_type*).
        fermat::Internal::radix_sort_impl<value_type *, SelfEx, DigitBits>(
            ptr_begin, ptr_end, buf_ptr, SelfEx(), unsigned_type());
    }

    /// @brief Radix sort for user-defined types with a key extractor.
    /// @tparam RandomAccessIterator Random access iterator.
    /// @tparam ExtractKey Callable type that accepts value_type and returns an integral key.
    /// @tparam DigitBits Number of bits per digit (default 8).
    /// @param first Start iterator.
    /// @param last End iterator.
    /// @param ex ExtractKey instance.
    template<typename RandomAccessIterator, typename ExtractKey,
         int DigitBits = 8,
         typename = std::enable_if_t<!std::is_integral_v<typename std::iterator_traits<RandomAccessIterator>::value_type>>>
void radix_sort(RandomAccessIterator first, RandomAccessIterator last, ExtractKey&& ex) {
        using value_type = typename std::iterator_traits<RandomAccessIterator>::value_type;
        using key_type = typename std::invoke_result_t<ExtractKey, value_type>;
        static_assert(std::is_integral_v<key_type>, "ExtractKey must return an integral type");
        using unsigned_key_type = std::make_unsigned_t<key_type>;

        auto n = static_cast<size_t>(last - first);
        if (n <= 1) return;

        value_type* ptr_begin = &*first;
        value_type* ptr_end = ptr_begin + n;

        fermat::Vector<value_type> buffer(n);
        value_type* buf_ptr = buffer.data();

        struct Wrapper {
            ExtractKey extract;
            unsigned_key_type operator()(const value_type& val) const {
                return static_cast<unsigned_key_type>(extract(val));
            }
        };

        fermat::Internal::radix_sort_impl<value_type*, Wrapper, DigitBits>(
            ptr_begin, ptr_end, buf_ptr, Wrapper{std::forward<ExtractKey>(ex)}, unsigned_key_type());
    }
} // namespace fermat
