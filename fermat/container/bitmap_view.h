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

#include <algorithm>
#include <turbo/numeric/bits.h>
#include <bit>

#include <cstddef>
#include <cstring>
#include <turbo/log/check.h>
#include <fermat/container/bitset.h>

namespace fermat {
    template<bool OverFlowAsFalse = true, typename WordType = uint64_t>
    class BitmapView;

    /// BitmapViewBase
    ///
    /// This is a default implementation that works for any number of words.
    ///
    template<bool OverFlowAsFalse, typename WordType>
    // Templated on the number of words used to hold the BitmapView and the word type.
    struct BitmapViewBase {
        typedef WordType word_type;
        typedef BitmapViewBase<OverFlowAsFalse, WordType> this_type;
        typedef size_t size_type;

        enum {
            kBitsPerWord = (8 * sizeof(word_type)),
            kBitsPerWordMask = (kBitsPerWord - 1),
            kBitsPerWordShift = ((kBitsPerWord == 8)
                                     ? 3
                                     : ((kBitsPerWord == 16)
                                            ? 4
                                            : ((kBitsPerWord == 32) ? 5 : (((kBitsPerWord == 64) ? 6 : 7)))))
        };

    public:
        // invariant: we keep any high bits in the last word that are unneeded set to 0
        // so that our to_ulong() conversion can simply copy the words into the target type.
        word_type *_words{nullptr};
        size_type _num_words{0};

    public:
        void operator&=(const this_type &x);

        void operator|=(const this_type &x);

        void operator^=(const this_type &x);

        void operator<<=(size_type n);

        void operator>>=(size_type n);

        void flip();

        void set();

        void set(size_type i, bool value);

        void reset();

        bool operator==(const this_type &x) const;

        [[nodiscard]] bool any() const;

        [[nodiscard]] size_type count() const;

        word_type &DoGetWord(size_type i);

        word_type DoGetWord(size_type i) const;

        [[nodiscard]] size_type DoFindFirst() const;

        [[nodiscard]] size_type DoFindNext(size_type last_find) const;

        [[nodiscard]] size_type DoFindLast() const; // Returns NW * kBitsPerWord (the bit count) if no bits are set.
        [[nodiscard]] size_type DoFindPrev(size_type last_find) const;

        // Returns NW * kBitsPerWord (the bit count) if no bits are set.
    }; // class BitmapViewBase


    /// BitmapView
    ///
    /// Implements a BitmapView much like the C++ std::BitmapView.
    ///
    /// As of this writing we don't implement a specialization of BitmapView<0>,
    /// as it is deemed an academic exercise that nobody would actually
    /// use and it would increase code space and provide little practical
    /// benefit. Note that this doesn't mean BitmapView<0> isn't supported;
    /// it means that our version of it isn't as efficient as it would be
    /// if a specialization was made for it.
    ///
    /// - N can be any unsigned (non-zero) value, though memory usage is
    ///   linear with respect to N, so large values of N use large amounts of memory.
    /// - WordType must be a non-cv qualified unsigned integral other than bool.
    ///   By default the WordType is the largest native register type that the
    ///   target platform supports.
    ///
    // BITSET_WORD_COUNT(N, WordType)
    template<bool OverFlowAsFalse, typename WordType>
    class BitmapView : private BitmapViewBase<OverFlowAsFalse, WordType> {
    public:
        static_assert(detail::is_word_type_v<WordType>,
                      "Word type must be a non-cv qualified, unsigned integral other than bool.");

        typedef BitmapViewBase<OverFlowAsFalse, WordType> base_type;
        typedef BitmapView<OverFlowAsFalse, WordType> this_type;
        typedef WordType word_type;
        typedef typename base_type::size_type size_type;

        enum {
            kBitsPerWord = (8 * sizeof(word_type)),
            kBitsPerWordMask = (kBitsPerWord - 1),
            kBitsPerWordShift = ((kBitsPerWord == 8)
                                     ? 3
                                     : ((kBitsPerWord == 16)
                                            ? 4
                                            : ((kBitsPerWord == 32) ? 5 : (((kBitsPerWord == 64) ? 6 : 7))))),
            kWordSize = sizeof(word_type), // The size of individual words the BitmapView uses to hold the bits.
        };

        // internal implementation details. do not use.
        using base_type::_words;
        using base_type::_num_words;
        using base_type::DoGetWord;
        using base_type::DoFindFirst;
        using base_type::DoFindNext;
        using base_type::DoFindLast;
        using base_type::DoFindPrev;

        using base_type::count;
        using base_type::any;

    public:
        /// Reference
        ///
        /// A Reference is a Reference to a specific bit in the BitmapView.
        /// The C++ standard specifies that this be a nested class,
        /// though it is not clear if a non-nested Reference implementation
        /// would be non-conforming.
        ///
        class Reference {
        protected:
            friend class BitmapView<OverFlowAsFalse, WordType>;

            word_type *mpBitWord;
            size_type mnBitIndex;

            Reference() = default;

        public:
            Reference(const BitmapView &x, size_type i);

            Reference &operator=(bool value);

            Reference &operator=(const Reference &x);

            bool operator~() const;

            operator bool() const // Defined inline because CodeWarrior fails to be able to compile it outside.
            {
                return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) != 0;
            }

            Reference &flip();
        };

    public:
        friend class Reference;

        BitmapView() = default;

        BitmapView(turbo::span<WordType> data, size_t bits_num);

        // We don't define copy constructor and operator= because
        // the compiler-generated versions will suffice.

        this_type &operator&=(const this_type &x);

        this_type &operator|=(const this_type &x);

        this_type &operator^=(const this_type &x);

        this_type &operator<<=(size_type n);

        this_type &operator>>=(size_type n);

        this_type &setup(turbo::span<WordType> data, size_t bits_num);

        this_type &setup(turbo::span<WordType> data);

        this_type &set();

        this_type &set(size_type i, bool value = true);

        this_type &reset();

        this_type &reset(size_type i);

        this_type &flip();

        this_type &flip(size_type i);

        this_type operator~() const;

        Reference operator[](size_type i);

        bool operator[](size_type i) const;

        const word_type *data() const;

        word_type *data();

        //size_type count() const;            // We inherit this from the base class.
        size_type size() const;

        size_type word_size() const;

        bool operator==(const this_type &x) const;

        bool operator!=(const this_type &x) const;

        bool test(size_type i) const;

        //bool any() const;                   // We inherit this from the base class.
        [[nodiscard]] bool all() const;

        [[nodiscard]] bool none() const;

        this_type operator<<(size_type n) const;

        this_type operator>>(size_type n) const;

        // Finds the index of the first "on" bit, returns size() if none are set.
        size_type find_first() const;

        // Finds the index of the next "on" bit after last_find, returns size() if none are set.
        size_type find_next(size_type last_find) const;

        // Finds the index of the last "on" bit, returns size() if none are set.
        size_type find_last() const;

        // Finds the index of the last "on" bit before last_find, returns size() if none are set.
        size_type find_prev(size_type last_find) const;

    private:
        size_type _bits_number{0};
    }; // BitmapView

    ///////////////////////////////////////////////////////////////////////////
    // BitmapViewBase
    //
    // We tried two forms of array access here:
    //     for(word_type *pWord(_words), *pWordEnd(_words + NW); pWord < pWordEnd; ++pWord)
    //         *pWord = ...
    // and
    //     for(size_t i = 0; i < NW; i++)
    //         _words[i] = ...
    //
    // For our tests (~NW < 16), the latter (using []) access resulted in faster code.
    ///////////////////////////////////////////////////////////////////////////


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::operator&=(const this_type &x) {
        for (size_t i = 0; i < _num_words; i++)
            _words[i] &= x._words[i];
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::operator|=(const this_type &x) {
        for (size_t i = 0; i < _num_words; i++)
            _words[i] |= x._words[i];
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::operator^=(const this_type &x) {
        for (size_t i = 0; i < _num_words; i++)
            _words[i] ^= x._words[i];
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::operator<<=(size_type n) {
        auto const nWordShift = (size_type) (n >> kBitsPerWordShift);

        if (nWordShift) {
            for (int i = (int) (_num_words - 1); i >= 0; --i)
                _words[i] = (nWordShift <= (size_type) i) ? _words[i - nWordShift] : (word_type) 0;
        }

        if (n &= kBitsPerWordMask) {
            for (size_t i = (_num_words - 1); i > 0; --i)
                _words[i] = (word_type) ((_words[i] << n) | (_words[i - 1] >> (kBitsPerWord - n)));
            _words[0] <<= n;
        }

        // We let the parent class turn off any upper bits.
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::operator>>=(size_type n) {
        auto const nWordShift = (size_type) (n >> kBitsPerWordShift);

        if (nWordShift) {
            for (size_t i = 0; i < _num_words; ++i)
                _words[i] = ((nWordShift < (_num_words - i)) ? _words[i + nWordShift] : (word_type) 0);
        }

        if (n &= kBitsPerWordMask) {
            for (size_t i = 0; i < (_num_words - 1); ++i)
                _words[i] = (word_type) ((_words[i] >> n) | (_words[i + 1] << (kBitsPerWord - n)));
            _words[_num_words - 1] >>= n;
        }
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::flip() {
        for (size_t i = 0; i < _num_words; i++)
            _words[i] = ~_words[i];
        // We let the parent class turn off any upper bits.
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::set() {
        for (size_t i = 0; i < _num_words; i++)
            _words[i] = static_cast<word_type>(~static_cast<word_type>(0));
        // We let the parent class turn off any upper bits.
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::set(size_type i, bool value) {
        if (value)
            _words[i >> kBitsPerWordShift] |= (static_cast<word_type>(1) << (i & kBitsPerWordMask));
        else
            _words[i >> kBitsPerWordShift] &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline void BitmapViewBase<OverFlowAsFalse, WordType>::reset() {
        if (_num_words > 16) // This is a constant expression and should be optimized away.
        {
            // This will be fastest if compiler intrinsic function optimizations are enabled.
            memset(_words, 0, _num_words * sizeof(word_type));
        } else {
            for (size_t i = 0; i < _num_words; i++)
                _words[i] = 0;
        }
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapViewBase<OverFlowAsFalse, WordType>::operator==(const this_type &x) const {
        for (size_t i = 0; i < _num_words; i++) {
            if (_words[i] != x._words[i])
                return false;
        }
        return true;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapViewBase<OverFlowAsFalse, WordType>::any() const {
        for (size_t i = 0; i < _num_words; i++) {
            if (_words[i])
                return true;
        }
        return false;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapViewBase<OverFlowAsFalse, WordType>::size_type
    BitmapViewBase<OverFlowAsFalse, WordType>::count() const {
        size_type n = 0;
        for (size_t i = 0; i < _num_words; i++)
            n += turbo::popcount(_words[i]);
        return n;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapViewBase<OverFlowAsFalse, WordType>::word_type &
    BitmapViewBase<OverFlowAsFalse, WordType>::DoGetWord(size_type i) {
        return _words[i >> kBitsPerWordShift];
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapViewBase<OverFlowAsFalse, WordType>::word_type
    BitmapViewBase<OverFlowAsFalse, WordType>::DoGetWord(size_type i) const {
        return _words[i >> kBitsPerWordShift];
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapViewBase<OverFlowAsFalse, WordType>::size_type
    BitmapViewBase<OverFlowAsFalse, WordType>::DoFindFirst() const {
        for (size_type word_index = 0; word_index < _num_words; ++word_index) {
            const size_type fbiw = turbo::countr_zero(_words[word_index]);

            if (fbiw != kBitsPerWord)
                return (word_index * kBitsPerWord) + fbiw;
        }

        return (size_type) _num_words * kBitsPerWord;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapViewBase<OverFlowAsFalse, WordType>::size_type
    BitmapViewBase<OverFlowAsFalse, WordType>::DoFindNext(size_type last_find) const {
        // Start looking from the next bit.
        ++last_find;

        // Set initial state based on last find.
        auto word_index = static_cast<size_type>(last_find >> kBitsPerWordShift);
        auto bit_index = static_cast<size_type>(last_find & kBitsPerWordMask);

        // To do: There probably is a more elegant way to write looping below.
        if (word_index < _num_words) {
            // Mask off previous bits of the word so our search becomes a "find first".
            word_type this_word = _words[word_index] & (static_cast<word_type>(~0) << bit_index);

            for (;;) {
                const size_type fbiw = turbo::countr_zero(this_word);

                if (fbiw != kBitsPerWord)
                    return (word_index * kBitsPerWord) + fbiw;

                if (++word_index < _num_words)
                    this_word = _words[word_index];
                else
                    break;
            }
        }

        return (size_type) _num_words * kBitsPerWord;
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapViewBase<OverFlowAsFalse, WordType>::size_type
    BitmapViewBase<OverFlowAsFalse, WordType>::DoFindLast() const {
        for (auto word_index = (size_type) _num_words; word_index > 0; --word_index) {
            const size_type lbiw = GetLastBit(_words[word_index - 1]);

            if (lbiw != kBitsPerWord)
                return ((word_index - 1) * kBitsPerWord) + lbiw;
        }

        return (size_type) _num_words * kBitsPerWord;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapViewBase<OverFlowAsFalse, WordType>::size_type
    BitmapViewBase<OverFlowAsFalse, WordType>::DoFindPrev(size_type last_find) const {
        if (last_find > 0) {
            // Set initial state based on last find.
            auto word_index = static_cast<size_type>(last_find >> kBitsPerWordShift);
            auto bit_index = static_cast<size_type>(last_find & kBitsPerWordMask);

            // Mask off subsequent bits of the word so our search becomes a "find last".
            // We do two shifts here because it's undefined behaviour to right shift greater than or equal to the number of bits in the integer.
            //
            // Note: operator~() is an arithmetic operator and performs integral promotions, ie. small integrals are promoted to an int.
            // Because the promotion is before applying operator~() we need to cast back to our word type otherwise we end up with extraneous set bits.
            word_type mask = (static_cast<word_type>(~static_cast<word_type>(0)) >> (kBitsPerWord - 1 - bit_index)) >>
                             1;
            word_type this_word = _words[word_index] & mask;

            for (;;) {
                const size_type lbiw = GetLastBit(this_word);

                if (lbiw != kBitsPerWord)
                    return (word_index * kBitsPerWord) + lbiw;

                if (word_index > 0)
                    this_word = _words[--word_index];
                else
                    break;
            }
        }

        return (size_type) _num_words * kBitsPerWord;
    }


    ///////////////////////////////////////////////////////////////////////////
    // BitmapView::Reference
    ///////////////////////////////////////////////////////////////////////////

    template<bool OverFlowAsFalse, typename WordType>
    inline BitmapView<OverFlowAsFalse, WordType>::Reference::Reference(const BitmapView &x, size_type i)
        : mpBitWord(&const_cast<BitmapView &>(x).DoGetWord(i)),
          mnBitIndex(i & kBitsPerWordMask) {
        // We have an issue here because the above is casting away the const-ness of the source BitmapView.
        // Empty
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::Reference &
    BitmapView<OverFlowAsFalse, WordType>::Reference::operator=(bool value) {
        if (value)
            *mpBitWord |= (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        else
            *mpBitWord &= ~(static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::Reference &
    BitmapView<OverFlowAsFalse, WordType>::Reference::operator=(const Reference &x) {
        if (TURBO_UNLIKELY(this == &x)) {
            return *this;
        }
        if (*x.mpBitWord & (static_cast<word_type>(1) << (x.mnBitIndex & kBitsPerWordMask)))
            *mpBitWord |= (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        else
            *mpBitWord &= ~(static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask));
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapView<OverFlowAsFalse, WordType>::Reference::operator~() const {
        return (*mpBitWord & (static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask))) == 0;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::Reference &
    BitmapView<OverFlowAsFalse, WordType>::Reference::flip() {
        *mpBitWord ^= static_cast<word_type>(1) << (mnBitIndex & kBitsPerWordMask);
        return *this;
    }


    ///////////////////////////////////////////////////////////////////////////
    // BitmapView
    ///////////////////////////////////////////////////////////////////////////

    template<bool OverFlowAsFalse, typename WordType>
    inline BitmapView<OverFlowAsFalse, WordType>::BitmapView(
        turbo::span<WordType> data, size_t bits_num) {
        setup(data, bits_num);
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::operator&=(const this_type &x) {
        base_type::operator&=(x);
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::operator|=(const this_type &x) {
        base_type::operator|=(x);
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::operator^=(const this_type &x) {
        base_type::operator^=(x);
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::operator<<=(size_type n) {
        if (TURBO_LIKELY((intptr_t) n < (intptr_t) _bits_number)) {
            base_type::operator<<=(n);
            if ((_bits_number & kBitsPerWordMask) || (_bits_number == 0))
                // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when _bits_number == 32.
                _words[_num_words - 1] &= ~(static_cast<word_type>(~static_cast<word_type>(0)) << (
                                                _bits_number & kBitsPerWordMask));
            // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        } else
            base_type::reset();
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::operator>>=(size_type n) {
        if (TURBO_LIKELY(n < _bits_number))
            base_type::operator>>=(n);
        else
            base_type::reset();
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::set() {
        base_type::set(); // This sets all bits.
        if ((_bits_number & kBitsPerWordMask) || (_bits_number == 0))
            // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when _bits_number == 32.
            _words[_num_words - 1] &= ~(static_cast<word_type>(~static_cast<word_type>(0)) << (
                                            _bits_number & kBitsPerWordMask));
        // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        return *this;
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &BitmapView<OverFlowAsFalse, WordType>::setup(
        turbo::span<WordType> data, size_t bits_num) {
        _bits_number = bits_num;
        _num_words = BITSET_WORD_COUNT(bits_num, WordType);
        _words = data.data();
        KCHECK(_num_words<=data.size());
        return *this;
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &BitmapView<OverFlowAsFalse, WordType>::setup(
        turbo::span<WordType> data) {
        return setup(data, data.size() * kBitsPerWord);
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::set(size_type i, bool value) {
        if (TURBO_LIKELY(i < _bits_number)) {
            base_type::set(i, value);
        } else {
            if constexpr (!OverFlowAsFalse) {
                KCHECK(false) << "BitmapView::set -- out of range";
            }
        }

        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::reset() {
        base_type::reset();
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::reset(size_type i) {
        if (TURBO_LIKELY(i < _bits_number)) {
            DoGetWord(i) &= ~(static_cast<word_type>(1) << (i & kBitsPerWordMask));
        } else {
            if constexpr (!OverFlowAsFalse) {
                KCHECK(false) << "BitmapView::reset -- out of range";
            }
        }

        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::flip() {
        base_type::flip();
        if ((_bits_number & kBitsPerWordMask) || (_bits_number == 0))
            // If there are any high bits to clear... (If we didn't have this check, then the code below would do the wrong thing when _bits_number == 32.
            _words[_num_words - 1] &= ~(static_cast<word_type>(~static_cast<word_type>(0)) << (
                                            _bits_number & kBitsPerWordMask));
        // This clears any high unused bits. We need to do this so that shift operations proceed correctly.
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type &
    BitmapView<OverFlowAsFalse, WordType>::flip(size_type i) {
        if (TURBO_LIKELY(i < _bits_number))
            DoGetWord(i) ^= (static_cast<word_type>(1) << (i & kBitsPerWordMask));
        else {
            if constexpr (!OverFlowAsFalse) {
                KCHECK(false) << "BitmapView::flip -- out of range";
            }
        }
        return *this;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type
    BitmapView<OverFlowAsFalse, WordType>::operator~() const {
        return this_type(*this).flip();
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::Reference
    BitmapView<OverFlowAsFalse, WordType>::operator[](size_type i) {
        if (TURBO_LIKELY(i < _bits_number)) {
            return Reference(*this, i);
        }
        KCHECK(false) << "BitmapView::operator[] -- out of range";
        TURBO_UNREACHABLE();
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapView<OverFlowAsFalse, WordType>::operator[](size_type i) const {
        if (TURBO_LIKELY(i < _bits_number))
            return (DoGetWord(i) & (static_cast<word_type>(1) << (i & kBitsPerWordMask))) != 0;
        if constexpr (!OverFlowAsFalse) {
            KCHECK(false) << "BitmapView::operator[] -- out of range";
        }
        return false;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline const typename BitmapView<OverFlowAsFalse, WordType>::word_type *BitmapView<OverFlowAsFalse,
        WordType>::data() const {
        return base_type::_words;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::word_type *BitmapView<OverFlowAsFalse, WordType>::data() {
        return base_type::_words;
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::size_type
    BitmapView<OverFlowAsFalse, WordType>::size() const {
        return (size_type) _bits_number;
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::size_type
    BitmapView<OverFlowAsFalse, WordType>::word_size() const {
        return (size_type) _num_words;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapView<OverFlowAsFalse, WordType>::operator==(const this_type &x) const {
        return base_type::operator==(x);
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapView<OverFlowAsFalse, WordType>::operator!=(const this_type &x) const {
        return !base_type::operator==(x);
    }

    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapView<OverFlowAsFalse, WordType>::test(size_type i) const {
        if (TURBO_LIKELY(i < _bits_number))
            return (DoGetWord(i) & (static_cast<word_type>(1) << (i & kBitsPerWordMask))) != 0;

        if constexpr (!OverFlowAsFalse) {
            KCHECK(false) << "BitmapView::test -- out of range";
        }

        return false;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapView<OverFlowAsFalse, WordType>::all() const {
        return count() == size();
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline bool BitmapView<OverFlowAsFalse, WordType>::none() const {
        return !base_type::any();
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type
    BitmapView<OverFlowAsFalse, WordType>::operator<<(size_type n) const {
        return this_type(*this).operator<<=(n);
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::this_type
    BitmapView<OverFlowAsFalse, WordType>::operator>>(size_type n) const {
        return this_type(*this).operator>>=(n);
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::size_type
    BitmapView<OverFlowAsFalse, WordType>::find_first() const {
        const size_type i = base_type::DoFindFirst();

        if (i < _bits_number)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return _bits_number;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::size_type
    BitmapView<OverFlowAsFalse, WordType>::find_next(size_type last_find) const {
        const size_type i = base_type::DoFindNext(last_find);

        if (i < _bits_number)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return _bits_number;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::size_type
    BitmapView<OverFlowAsFalse, WordType>::find_last() const {
        const size_type i = base_type::DoFindLast();

        if (i < _bits_number)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return _bits_number;
    }


    template<bool OverFlowAsFalse, typename WordType>
    inline typename BitmapView<OverFlowAsFalse, WordType>::size_type
    BitmapView<OverFlowAsFalse, WordType>::find_prev(size_type last_find) const {
        const size_type i = base_type::DoFindPrev(last_find);

        if (i < _bits_number)
            return i;
        // Else i could be the base type bit count, so we clamp it to our size.

        return _bits_number;
    }

} // namespace fermat
