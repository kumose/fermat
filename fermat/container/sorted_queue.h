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

#include <fermat/container/construct.h>
#include <fermat/container/vector.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <utility>

namespace fermat {

    /// SortedQueue
    ///
    /// Adapter that keeps elements in sorted order inside an underlying random-access
    /// container. A sliding [start, end) window into `c` leaves slack on both sides so
    /// inserts can shift toward the cheaper end. pop_front() and pop_back() are O(1).
    ///
    /// With the default Compare (std::less), elements are stored in ascending order:
    /// front() is the smallest, back() is the largest. pop_front() removes the smallest;
    /// pop_back() removes the largest.
    ///
    /// When the queue becomes empty it recenters the window in reserved capacity so
    /// subsequent pushes have balanced slack on both sides.
    ///
    /// There is no fixed capacity limit; users that need bounded behaviour can wrap this
    /// type and pop after their own threshold.
    ///
    template<typename T, size_t Alignment, typename Container = fermat::Vector<T, Alignment>,
        typename Compare = std::less<typename Container::value_type> >
    class SortedQueue {
    public:
        typedef SortedQueue<T, Alignment, Container, Compare> this_type;
        typedef Container container_type;
        typedef Compare compare_type;
        typedef typename Container::value_type value_type;
        typedef typename Container::reference reference;
        typedef typename Container::const_reference const_reference;
        typedef typename Container::size_type size_type;
        typedef typename Container::difference_type difference_type;
        typedef typename Container::iterator iterator;
        typedef typename Container::const_iterator const_iterator;

    public:
        container_type c;
        compare_type comp;

    protected:
        iterator start;
        iterator end;
        size_type head_;
        size_type tail_;

        static constexpr size_type kDefaultSlackCapacity = 16;

        void sync_iterators() noexcept {
            start = c.begin() + static_cast<difference_type>(head_);
            end = c.begin() + static_cast<difference_type>(tail_);
        }

        size_type size_impl() const noexcept {
            return tail_ - head_;
        }

        size_type left_slack() const noexcept {
            return head_;
        }

        size_type right_slack() const noexcept {
            return c.capacity() - tail_;
        }

        void ensure_container_size(size_type needed_size) {
            while (c.size() < needed_size)
                (void) c.push_back_uninitialized();
        }

        void reset_if_empty() {
            if (head_ != tail_)
                return;

            size_type cap = c.capacity();
            if (cap < kDefaultSlackCapacity) {
                c.reserve(kDefaultSlackCapacity);
                cap = c.capacity();
            }

            c.clear();
            c.reserve(cap);
            const size_type center = cap / 2;
            c.resize(center);
            head_ = tail_ = center;
            sync_iterators();
        }

        void compact_to_begin() {
            const size_type n = size_impl();
            if (head_ == 0 || n == 0)
                return;

            value_type *const base = c.data();
            if (n > 0)
                std::move(base + head_, base + tail_, base);
            fermat::destroy(base + n, base + tail_);
            head_ = 0;
            tail_ = n;
            while (c.size() > tail_)
                c.pop_back();
            ensure_container_size(tail_);
            sync_iterators();
        }

        void rebalance_if_needed() {
            if (head_ == 0)
                return;
            if (head_ >= tail_ / 2)
                compact_to_begin();
        }

        template<typename U>
        void insert_at(size_type pos_index, U &&value) {
            const size_type left_move = pos_index - head_;
            const size_type right_move = tail_ - pos_index;
            const bool prefer_left = (left_move < right_move) && (head_ > 0);

            if (prefer_left) {
                --head_;
                --pos_index;
                value_type *const base = c.data();
                for (size_type i = pos_index; i > head_; --i)
                    base[i] = std::move(base[i - 1]);
                fermat::construct_at(base + pos_index, std::forward<U>(value));
            } else {
                if (right_slack() == 0) {
                    if (head_ > 0)
                        compact_to_begin();
                    else
                        c.reserve(std::max(c.capacity() * 2, tail_ + 1));
                }

                ++tail_;
                ensure_container_size(tail_);
                value_type *const base = c.data();
                for (size_type i = tail_ - 1; i > pos_index; --i)
                    base[i] = std::move(base[i - 1]);
                fermat::construct_at(base + pos_index, std::forward<U>(value));
            }

            sync_iterators();
            rebalance_if_needed();
        }

        template<typename U>
        void do_push(U &&value) {
#if FERMAT_EXCEPTIONS_ENABLED
            try {
                if (head_ == tail_) {
                    if (head_ == 0 && tail_ == 0 && c.empty())
                        reset_if_empty();
                    insert_at(head_, std::forward<U>(value));
                } else {
                    value_type *const base = c.data();
                    const size_type pos_index = static_cast<size_type>(
                        std::lower_bound(base + head_, base + tail_, value, comp) - base);
                    insert_at(pos_index, std::forward<U>(value));
                }
            } catch (...) {
                c.clear();
                head_ = tail_ = 0;
                start = end = c.begin();
                throw;
            }
#else
            if (head_ == tail_) {
                if (head_ == 0 && tail_ == 0 && c.empty())
                    reset_if_empty();
                insert_at(head_, std::forward<U>(value));
            } else {
                value_type *const base = c.data();
                const size_type pos_index = static_cast<size_type>(
                    std::lower_bound(base + head_, base + tail_, value, comp) - base);
                insert_at(pos_index, std::forward<U>(value));
            }
#endif
        }

        void init_from_container() {
            if (!c.empty())
                std::sort(c.begin(), c.end(), comp);
            head_ = 0;
            tail_ = c.size();
            sync_iterators();
        }

    public:
        SortedQueue()
            : c(),
              comp(),
              start(c.begin()),
              end(c.begin()),
              head_(0),
              tail_(0) {
        }

        template<class Allocator>
        explicit SortedQueue(const Allocator &allocator)
            : c(allocator),
              comp(),
              start(c.begin()),
              end(c.begin()),
              head_(0),
              tail_(0) {
        }

        template<class Allocator>
        SortedQueue(const this_type &x, const Allocator &allocator)
            : c(x.c, allocator),
              comp(x.comp),
              head_(x.head_),
              tail_(x.tail_) {
            sync_iterators();
        }

        template<class Allocator>
        SortedQueue(this_type &&x, const Allocator &allocator)
            : c(std::move(x.c), allocator),
              comp(x.comp),
              head_(x.head_),
              tail_(x.tail_) {
            sync_iterators();
            x.head_ = x.tail_ = 0;
            x.start = x.end = x.c.begin();
        }

        explicit SortedQueue(const compare_type &compare)
            : c(),
              comp(compare),
              start(c.begin()),
              end(c.begin()),
              head_(0),
              tail_(0) {
        }

        explicit SortedQueue(const compare_type &compare, container_type &&x)
            : c(std::move(x)),
              comp(compare),
              head_(0),
              tail_(0) {
            init_from_container();
        }

        SortedQueue(const compare_type &compare, const container_type &x)
            : c(x),
              comp(compare),
              head_(0),
              tail_(0) {
            init_from_container();
        }

        SortedQueue(std::initializer_list<value_type> ilist, const compare_type &compare = compare_type())
            : c(),
              comp(compare),
              head_(0),
              tail_(0) {
            c.insert(c.end(), ilist.begin(), ilist.end());
            init_from_container();
        }

        template<typename InputIterator>
        SortedQueue(InputIterator first, InputIterator last)
            : c(first, last),
              comp(),
              head_(0),
              tail_(0) {
            init_from_container();
        }

        template<typename InputIterator>
        SortedQueue(InputIterator first, InputIterator last, const compare_type &compare)
            : c(first, last),
              comp(compare),
              head_(0),
              tail_(0) {
            init_from_container();
        }

        template<typename InputIterator>
        SortedQueue(InputIterator first, InputIterator last, const compare_type &compare, const container_type &x)
            : c(x),
              comp(compare),
              head_(0),
              tail_(0) {
            c.insert(c.end(), first, last);
            init_from_container();
        }

        template<typename InputIterator>
        SortedQueue(InputIterator first, InputIterator last, const compare_type &compare, container_type &&x)
            : c(std::move(x)),
              comp(compare),
              head_(0),
              tail_(0) {
            c.insert(c.end(), first, last);
            init_from_container();
        }

        bool empty() const {
            return head_ == tail_;
        }

        size_type size() const {
            return size_impl();
        }

        reference front() {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::front -- empty container");
#endif
            return *start;
        }

        const_reference front() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::front -- empty container");
#endif
            return *start;
        }

        reference back() {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::back -- empty container");
#endif
            return *(end - 1);
        }

        const_reference back() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::back -- empty container");
#endif
            return *(end - 1);
        }

        void push(const value_type &value) {
            do_push(value);
        }

        void push(value_type &&value) {
            do_push(std::move(value));
        }

        template<class... Args>
        void emplace(Args &&... args) {
            push(value_type(std::forward<Args>(args)...));
        }

        void pop_front() {
#if FERMAT_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::pop_front -- empty container");
#endif
            ++head_;
            sync_iterators();
            reset_if_empty();
        }

        void pop_front(value_type &value) {
#if FERMAT_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::pop_front -- empty container");
#endif
            value = std::move(*start);
            pop_front();
        }

        void pop_back() {
#if FERMAT_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::pop_back -- empty container");
#endif
            --tail_;
            fermat::destroy_at(c.data() + tail_);
            sync_iterators();
            reset_if_empty();
        }

        void pop_back(value_type &value) {
#if FERMAT_ASSERT_ENABLED
            if (FERMAT_UNLIKELY(empty()))
                FERMAT_FAIL_MSG("SortedQueue::pop_back -- empty container");
#endif
            value = std::move(*(end - 1));
            pop_back();
        }

        void clear() {
            fermat::destroy(c.data() + head_, c.data() + tail_);
            c.clear();
            head_ = tail_ = 0;
            sync_iterators();
        }

        container_type &get_container() {
            return c;
        }

        const container_type &get_container() const {
            return c;
        }

        void swap(this_type &x) noexcept(
            (std::is_nothrow_swappable<typename this_type::container_type>::value &&
                std::is_nothrow_swappable<typename this_type::compare_type>::value)) {
            using std::swap;
            swap(c, x.c);
            swap(comp, x.comp);
            swap(head_, x.head_);
            swap(tail_, x.tail_);
            sync_iterators();
            x.sync_iterators();
        }

        bool validate() const {
            if (!c.validate())
                return false;
            if (head_ > tail_ || tail_ > c.size())
                return false;
            if (head_ == tail_)
                return true;
            return std::is_sorted(start, end, comp);
        }

        template<typename U, size_t A, typename C, typename Co>
        friend bool operator==(const SortedQueue<U, A, C, Co> &a, const SortedQueue<U, A, C, Co> &b);

        template<typename U, size_t A, typename C, typename Co>
        friend bool operator<(const SortedQueue<U, A, C, Co> &a, const SortedQueue<U, A, C, Co> &b);
    }; // class SortedQueue


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator==(const SortedQueue<T, Alignment, Container, Compare> &a,
                           const SortedQueue<T, Alignment, Container, Compare> &b) {
        if (a.size() != b.size())
            return false;
        return std::equal(a.c.begin() + static_cast<typename Container::difference_type>(a.head_),
                          a.c.begin() + static_cast<typename Container::difference_type>(a.tail_),
                          b.c.begin() + static_cast<typename Container::difference_type>(b.head_));
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator<(const SortedQueue<T, Alignment, Container, Compare> &a,
                          const SortedQueue<T, Alignment, Container, Compare> &b) {
        return std::lexicographical_compare(
            a.c.begin() + static_cast<typename Container::difference_type>(a.head_),
            a.c.begin() + static_cast<typename Container::difference_type>(a.tail_),
            b.c.begin() + static_cast<typename Container::difference_type>(b.head_),
            b.c.begin() + static_cast<typename Container::difference_type>(b.tail_));
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator!=(const SortedQueue<T, Alignment, Container, Compare> &a,
                           const SortedQueue<T, Alignment, Container, Compare> &b) {
        return !(a == b);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator>(const SortedQueue<T, Alignment, Container, Compare> &a,
                          const SortedQueue<T, Alignment, Container, Compare> &b) {
        return b < a;
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator<=(const SortedQueue<T, Alignment, Container, Compare> &a,
                           const SortedQueue<T, Alignment, Container, Compare> &b) {
        return !(b < a);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator>=(const SortedQueue<T, Alignment, Container, Compare> &a,
                           const SortedQueue<T, Alignment, Container, Compare> &b) {
        return !(a < b);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void swap(SortedQueue<T, Alignment, Container, Compare> &a,
                     SortedQueue<T, Alignment, Container, Compare> &b) noexcept(
        (std::is_nothrow_swappable<typename SortedQueue<T, Alignment, Container, Compare>::container_type>::value &&
            std::is_nothrow_swappable<typename SortedQueue<T, Alignment, Container, Compare>::compare_type>::value)) {
        a.swap(b);
    }
} // namespace fermat
