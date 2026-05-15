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


#include <fermat/container/vector.h>
#include <fermat/container/heap.h>
#include <functional>
#include <initializer_list>
#include <cstddef>

namespace fermat {

    /// PriorityQueue
    ///
    /// The behaviour of this class is just like the std::PriorityQueue
    /// class and you can refer to std documentation on it.
    ///
    /// A PriorityQueue is an adapter container which implements a
    /// queue-like container whereby pop() returns the item of highest
    /// priority. The entire queue isn't necessarily sorted; merely the
    /// first item in the queue happens to be of higher priority than
    /// other items. You can read about priority_queues in many books
    /// on algorithms, such as "Algorithms" by Robert Sedgewick.
    ///
    /// The Container type is a container which is random access and
    /// supports empty(), size(), clear(), insert(), front(),
    /// push_back(), and pop_back(). You would typically use vector
    /// or deque.
    ///
    /// Note that we don't provide functions in the PriorityQueue
    /// interface for working with allocators or names. The reason for
    /// this is that PriorityQueue is an adapter class which can work
    /// with any standard sequence and not necessarily just a sequence
    /// provided by this library. So what we do is provide a member
    /// accessor function get_container() which allows the user to
    /// manipulate the sequence as needed. The user needs to be careful
    /// not to change the container's contents, however.
    ///
    /// Classic heaps allow for the concept of removing arbitrary items
    /// and changing the priority of arbitrary items, though the C++
    /// std heap (and thus PriorityQueue) functions don't support
    /// these operations. We have extended the heap algorithms and the
    /// PriorityQueue implementation to support these operations.
    ///
    ///////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Container = fermat::Vector<T, Alignment>, typename Compare = std::less<typename
        Container::value_type> >
    class PriorityQueue {
    public:
        typedef PriorityQueue<T, Alignment, Container, Compare> this_type;
        typedef Container container_type;
        typedef Compare compare_type;
        //typedef typename Container::allocator_type           allocator_type;  // We can't currently declare this because the container may be a type that doesn't have an allocator.
        typedef typename Container::value_type value_type;
        typedef typename Container::reference reference;
        typedef typename Container::const_reference const_reference;
        typedef typename Container::size_type size_type;
        typedef typename Container::difference_type difference_type;

    public:
        // We declare public so that global comparison operators can be implemented without adding an inline level and without tripping up GCC 2.x friend declaration failures. GCC (through at least v4.0) is poor at inlining and performance wins over correctness.
        container_type c;
        // The C++ standard specifies that you declare a protected member variable of type Container called 'c'.
        compare_type comp;
        // The C++ standard specifies that you declare a protected member variable of type Compare called 'comp'.

    public:
        PriorityQueue();

        // Allocator is templated here because we aren't allowed to infer the allocator_type from the Container, as some containers (e.g. array) don't
        // have allocators. For containers that don't have allocator types, you could use void or char as the Allocator template type.

        template<class Allocator>
        explicit PriorityQueue(const Allocator &allocator)
            : c(allocator), comp() {
        }

        template<class Allocator>
        PriorityQueue(const this_type &x, const Allocator &allocator)
            : c(x.c, allocator), comp(x.comp) {
            fermat::make_heap(c.begin(), c.end(), comp);
        }

        template<class Allocator>
        PriorityQueue(this_type &&x, const Allocator &allocator)
            : c(std::move(x.c), allocator), comp(x.comp) {
            fermat::make_heap(c.begin(), c.end(), comp);
        }

        explicit PriorityQueue(const compare_type &compare);

        explicit PriorityQueue(const compare_type &compare, container_type &&x);

        PriorityQueue(const compare_type &compare, const container_type &x);

        PriorityQueue(std::initializer_list<value_type> ilist, const compare_type &compare = compare_type());

        // C++11 doesn't specify that std::PriorityQueue has initializer list support.

        template<typename InputIterator>
        PriorityQueue(InputIterator first, InputIterator last);

        template<typename InputIterator>
        PriorityQueue(InputIterator first, InputIterator last, const compare_type &compare);

        template<typename InputIterator>
        PriorityQueue(InputIterator first, InputIterator last, const compare_type &compare, const container_type &x);

        template<class InputIterator>
        PriorityQueue(InputIterator first, InputIterator last, const compare_type &compare, container_type &&x);

        // Additional C++11 support to consider:
        //
        // template <class Allocator>
        // PriorityQueue(const Compare&, const Allocator&);
        //
        // template <class Allocator>
        // PriorityQueue(const Compare&, const container_type&, const Allocator&);
        //
        // template <class Allocator>
        // PriorityQueue(const Compare&, container_type&&, const Allocator&);
        //
        // template <typename InputIterator, class Allocator>
        // PriorityQueue(InputIterator first, InputIterator last, const Allocator& allocator);
        //
        // template <typename InputIterator, class Allocator>
        // PriorityQueue(InputIterator first, InputIterator last, const compare_type& compare, const Allocator& allocator);
        //
        // template <typename InputIterator, class Allocator>
        // PriorityQueue(InputIterator first, InputIterator last, const compare_type& compare, const container_type& x, const Allocator& allocator);
        //
        // template <typename InputIterator, class Allocator>
        // PriorityQueue(InputIterator first, InputIterator last, const compare_type& compare, container_type&& x, const Allocator& allocator);

        bool empty() const;

        size_type size() const;

        const_reference top() const;

        void push(const value_type &value);

        void push(value_type &&x);

        template<class... Args>
        void emplace(Args &&... args);

        void pop();

        void pop(value_type &value);

        // Extension to the C++11 Standard that allows popping a move-only type (e.g. unique_ptr).

        void change(size_type n);

        /// Extension to the C++ Standard. Moves the item at the given array index to a new location based on its current priority.
        void remove(size_type n); /// Extension to the C++ Standard. Removes the item at the given array index.

        container_type &get_container();

        const container_type &get_container() const;

        void swap(this_type &x) noexcept(
            (std::is_nothrow_swappable<typename this_type::container_type>::value && std::is_nothrow_swappable<typename this_type::
                compare_type>::value));

        bool validate() const;
    }; // class PriorityQueue


    ///////////////////////////////////////////////////////////////////////
    // PriorityQueue
    ///////////////////////////////////////////////////////////////////////


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue()
        : c(),
          // To consider: use c(FERMAT_PRIORITY_QUEUE_DEFAULT_ALLOCATOR) here, though that would add the requirement that the user supplied container support this.
          comp() {
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(const compare_type &compare)
        : c(),
          // To consider: use c(FERMAT_PRIORITY_QUEUE_DEFAULT_ALLOCATOR) here, though that would add the requirement that the user supplied container support this.
          comp(compare) {
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(const compare_type &compare, const container_type &x)
        : c(x), comp(compare) {
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(const compare_type &compare, container_type &&x)
        : c(std::move(x)), comp(compare) {
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(std::initializer_list<value_type> ilist,
                                                                 const compare_type &compare)
        : c(), comp(compare) {
        c.insert(c.end(), ilist.begin(), ilist.end());
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    template<typename InputIterator>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(InputIterator first, InputIterator last)
        : c(first, last), comp() {
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    template<typename InputIterator>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(InputIterator first, InputIterator last,
                                                                 const compare_type &compare)
        : c(first, last), comp(compare) {
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    template<typename InputIterator>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(InputIterator first, InputIterator last,
                                                                 const compare_type &compare, const container_type &x)
        : c(x), comp(compare) {
        c.insert(c.end(), first, last);
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    template<typename InputIterator>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(InputIterator first, InputIterator last,
                                                                 const compare_type &compare, container_type &&x)
        : c(std::move(x)), comp(compare) {
        c.insert(c.end(), first, last);
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool PriorityQueue<T, Alignment, Container, Compare>::empty() const {
        return c.empty();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline typename PriorityQueue<T, Alignment, Container, Compare>::size_type
    PriorityQueue<T, Alignment, Container, Compare>::size() const {
        return c.size();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline typename PriorityQueue<T, Alignment, Container, Compare>::const_reference
    PriorityQueue<T, Alignment, Container, Compare>::top() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("PriorityQueue::top -- empty container");
#endif

        return c.front();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::push(const value_type &value) {
#if FERMAT_EXCEPTIONS_ENABLED
        try {
            c.push_back(value);
            fermat::push_heap(c.begin(), c.end(), comp);
        } catch (...) {
            c.clear();
            throw;
        }
#else
        c.push_back(value);
        fermat::push_heap(c.begin(), c.end(), comp);
#endif
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::push(value_type &&value) {
#if FERMAT_EXCEPTIONS_ENABLED
        try {
            c.push_back(std::move(value));
            fermat::push_heap(c.begin(), c.end(), comp);
        } catch (...) {
            c.clear();
            throw;
        }
#else
        c.push_back(std::move(value));
        fermat::push_heap(c.begin(), c.end(), comp);
#endif
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    template<class... Args>
    inline void PriorityQueue<T, Alignment, Container, Compare>::emplace(Args &&... args) {
        push(value_type(std::forward<Args>(args)...));
        // The C++11 Standard 23.6.4/1 states that c.emplace is used, but also declares that c doesn't need to have an emplace function.
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::pop() {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("PriorityQueue::pop -- empty container");
#endif

#if FERMAT_EXCEPTIONS_ENABLED
        try {
            fermat::pop_heap(c.begin(), c.end(), comp);
            c.pop_back();
        } catch (...) {
            c.clear();
            throw;
        }
#else
        fermat::pop_heap(c.begin(), c.end(), comp);
        c.pop_back();
#endif
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::pop(value_type &value) {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("PriorityQueue::pop -- empty container");
#endif

        value = std::move(c.front()); // To consider: value = move_if_noexcept_assignable(c.front());
        pop();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::change(size_type n)
    // This function is not in the STL std::PriorityQueue.
    {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(n >= c.size()))
            FERMAT_FAIL_MSG("PriorityQueue::change -- out of range");
#endif

        fermat::change_heap(c.begin(), c.size(), n, comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::remove(size_type n)
    // This function is not in the STL std::PriorityQueue.
    {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(n >= c.size()))
            FERMAT_FAIL_MSG("PriorityQueue::remove -- out of range");
#endif

        fermat::remove_heap(c.begin(), c.size(), n, comp);
        c.pop_back();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline typename PriorityQueue<T, Alignment, Container, Compare>::container_type &
    PriorityQueue<T, Alignment, Container, Compare>::get_container() {
        return c;
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline const typename PriorityQueue<T, Alignment, Container, Compare>::container_type &
    PriorityQueue<T, Alignment, Container, Compare>::get_container() const {
        return c;
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::swap(this_type &x) noexcept (
        (std::is_nothrow_swappable<typename this_type::container_type>::value &&
            std::is_nothrow_swappable<typename this_type::compare_type>::value)) {
        using std::swap;
        swap(c, x.c);
        swap(comp, x.comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool
    PriorityQueue<T, Alignment, Container, Compare>::validate() const {
        return c.validate() && fermat::is_heap(c.begin(), c.end(), comp);
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Container, typename Compare>
    bool operator==(const PriorityQueue<T, Alignment, Container, Compare> &a, const PriorityQueue<T, Alignment, Container, Compare> &b) {
        return (a.c == b.c);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    bool operator<(const PriorityQueue<T, Alignment, Container, Compare> &a, const PriorityQueue<T, Alignment, Container, Compare> &b) {
        return (a.c < b.c);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator!=(const PriorityQueue<T, Alignment, Container, Compare> &a,
                           const PriorityQueue<T, Alignment, Container, Compare> &b) {
        return !(a.c == b.c);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator>(const PriorityQueue<T, Alignment, Container, Compare> &a,
                          const PriorityQueue<T, Alignment, Container, Compare> &b) {
        return (b.c < a.c);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator<=(const PriorityQueue<T, Alignment, Container, Compare> &a,
                           const PriorityQueue<T, Alignment, Container, Compare> &b) {
        return !(b.c < a.c);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline bool operator>=(const PriorityQueue<T, Alignment, Container, Compare> &a,
                           const PriorityQueue<T, Alignment, Container, Compare> &b) {
        return !(a.c < b.c);
    }


    template<class T, size_t Alignment, class Container, class Compare>
    inline void swap(PriorityQueue<T, Alignment, Container, Compare> &a, PriorityQueue<T, Alignment, Container, Compare> &b) noexcept(
        (std::is_nothrow_swappable<typename PriorityQueue<T, Alignment, Container, Compare>::container_type>::value &&
            std::is_nothrow_swappable<typename PriorityQueue<T, Alignment, Container, Compare>::compare_type>::value))
    // EDG has a bug and won't let us use Container in this noexcept statement
    {
        a.swap(b);
    }
} // namespace fermat
