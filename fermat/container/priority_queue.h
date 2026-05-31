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
#include <algorithm>
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

    template<typename T, size_t Alignment, typename Container = fermat::Vector<T, Alignment>, typename Compare =
        std::less<typename
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

        void push(value_type &&x) noexcept;

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
            (std::is_nothrow_swappable<typename this_type::container_type>::value && std::is_nothrow_swappable<typename
                 this_type::
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
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(
        const compare_type &compare, const container_type &x)
        : c(x), comp(compare) {
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(
        const compare_type &compare, container_type &&x)
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
                                                                          const compare_type &compare,
                                                                          const container_type &x)
        : c(x), comp(compare) {
        c.insert(c.end(), first, last);
        fermat::make_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    template<typename InputIterator>
    inline PriorityQueue<T, Alignment, Container, Compare>::PriorityQueue(InputIterator first, InputIterator last,
                                                                          const compare_type &compare,
                                                                          container_type &&x)
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
        return c.front();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::push(const value_type &value) {
        c.push_back(value);
        fermat::push_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::push(value_type &&value) noexcept {
        c.push_back(std::move(value));
        fermat::push_heap(c.begin(), c.end(), comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    template<class... Args>
    inline void PriorityQueue<T, Alignment, Container, Compare>::emplace(Args &&... args) {
        push(value_type(std::forward<Args>(args)...));
        // The C++11 Standard 23.6.4/1 states that c.emplace is used, but also declares that c doesn't need to have an emplace function.
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::pop() {
        fermat::pop_heap(c.begin(), c.end(), comp);
        c.pop_back();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::pop(value_type &value) {
        value = std::move(c.front()); // To consider: value = move_if_noexcept_assignable(c.front());
        pop();
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::change(size_type n) {
        // This function is not in the STL std::PriorityQueue.

        fermat::change_heap(c.begin(), c.size(), n, comp);
    }


    template<typename T, size_t Alignment, typename Container, typename Compare>
    inline void PriorityQueue<T, Alignment, Container, Compare>::remove(size_type n) {
        // This function is not in the STL std::PriorityQueue.
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
    bool operator==(const PriorityQueue<T, Alignment, Container, Compare> &a,
                    const PriorityQueue<T, Alignment, Container, Compare> &b) {
        return (a.c == b.c);
    }

    template<typename T, size_t Alignment, typename Container, typename Compare>
    bool operator<(const PriorityQueue<T, Alignment, Container, Compare> &a,
                   const PriorityQueue<T, Alignment, Container, Compare> &b) {
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
    inline void swap(PriorityQueue<T, Alignment, Container, Compare> &a,
                     PriorityQueue<T, Alignment, Container, Compare> &b) noexcept(
        (std::is_nothrow_swappable<typename PriorityQueue<T, Alignment, Container, Compare>::container_type>::value &&
         std::is_nothrow_swappable<typename PriorityQueue<T, Alignment, Container, Compare>::compare_type>::value))
    // EDG has a bug and won't let us use Container in this noexcept statement
    {
        a.swap(b);
    }


    /// MaxKQueue – maintains the largest K elements.
    /// Uses a min‑heap (Compare = std::greater) so that the smallest among the K largest is at the top.
    /// @tparam T        element type
    /// @tparam Compare  comparison functor (default = std::greater<T>)
    /// @tparam Alignment memory alignment for underlying vector
    /// @tparam Container underlying container type (must support random access, default = fermat::Vector<T, Alignment>)
    template<typename T,
        typename Compare = std::greater<T>,
        size_t Alignment = 0,
        typename Container = fermat::Vector<T, Alignment> >
    class MaxKQueue {
    public:
        using value_type = T;
        using compare_type = Compare;
        using container_type = Container;
        using size_type = typename container_type::size_type;

        MaxKQueue() = default;

        /// Constructor with capacity K.
        explicit MaxKQueue(size_type k, const compare_type &comp = compare_type())
            : _capacity(k), _comp(comp) {
            // Pre-allocate memory to avoid reallocation
            _container.reserve(k);
        }

        void set_capacity(size_type k) {
            _capacity = k;
            _container.reserve(k);
            while (_container.size() > _capacity) {
                pop();
            }
        }

        /// Pushes a value into the queue.
        void push(const value_type &value) {
            if (_container.size() < _capacity) {
                _container.push_back(value);
                if (_container.size() == 1) return;
                // Adjust the heap bottom-up using push_heap
                fermat::push_heap(_container.begin(), _container.end(), _comp);
            } else if (!_container.empty() && _comp(value, _container.front())) {
                pop();
                push(value);
            }
        }

        /// Pushes a value (move version).
        void push(value_type &&value) {
            if (_container.size() < _capacity) {
                _container.push_back(std::move(value));
                if (_container.size() == 1) return;
                fermat::push_heap(_container.begin(), _container.end(), _comp);
            } else if (_comp(value, _container.front())) {
                pop();
                push(std::move(value));
            }
        }

        /// Returns the smallest element among the K largest (top of the min‑heap).
        const value_type &top() const {
            // Assumes non-empty
            return _container.front();
        }

        /// Removes the smallest element among the K largest.
        void pop() {
            fermat::pop_heap(_container.begin(), _container.end(), _comp);
            _container.pop_back();
        }

        /// Checks if the queue is empty.
        [[nodiscard]] bool empty() const { return _container.empty(); }

        /// Returns the current number of stored elements.
        size_type size() const { return _container.size(); }

        /// Returns the maximum number of elements that can be stored.
        size_type capacity() const { return _capacity; }

        /// Provides direct access to the underlying container.
        container_type &get_container() { return _container; }

        /// Provides const access to the underlying container.
        const container_type &get_container() const { return _container; }

        /// Swaps two MaxKQueue objects.
        void swap(MaxKQueue &other) noexcept {
            using std::swap;
            swap(_container, other._container);
            swap(_capacity, other._capacity);
            swap(_comp, other._comp);
        }

        /// Moves out the underlying container sorted ascending by _comp.
        container_type aes() && {
            container_type c = std::move(_container);
            _capacity = 0;
            fermat::sort_heap(c.begin(), c.end(), _comp);
            return c;
        }

        /// Moves out the underlying container sorted descending by _comp.
        container_type des() && {
            container_type c = std::move(_container);
            _capacity = 0;
            fermat::sort_heap(c.begin(), c.end(), _comp);
            std::reverse(c.begin(), c.end());
            return c;
        }

    private:
        container_type _container;
        size_type _capacity;
        compare_type _comp;
    };

    /// MinKQueue – maintains the smallest K elements.
    /// Uses a max‑heap (Compare = std::less) so that the largest among the K smallest is at the top.
    /// @tparam T        element type
    /// @tparam Compare  comparison functor (default = std::less<T>)
    /// @tparam Alignment memory alignment for underlying vector
    /// @tparam Container underlying container type
    template<typename T,
        typename Compare = std::less<T>,
        size_t Alignment = 0,
        typename Container = fermat::Vector<T, Alignment> >
    class MinKQueue {
    public:
        using value_type = T;
        using compare_type = Compare;
        using container_type = Container;
        using size_type = typename container_type::size_type;

        MinKQueue() = default;

        /// Constructor with capacity K.
        explicit MinKQueue(size_type k, const compare_type &comp = compare_type())
            : _capacity(k), _comp(comp) {
            _container.reserve(k);
        }

        void set_capacity(size_type k) {
            _capacity = k;
            _container.reserve(k);
            while (_container.size() > _capacity) {
                pop();
            }
        }

        /// Pushes a value into the queue.
        void push(const value_type &value) {
            if (_container.size() < _capacity) {
                _container.push_back(value);
                if (_container.size() == 1) return;
                fermat::push_heap(_container.begin(), _container.end(), _comp);
            } else if (_comp(value, _container.front())) {
                pop();
                push(value);
            }
        }

        /// Pushes a value (move version).
        void push(value_type &&value) {
            if (_container.size() < _capacity) {
                _container.push_back(std::move(value));
                if (_container.size() == 1) return;
                fermat::push_heap(_container.begin(), _container.end(), _comp);
            } else if (!_container.empty() && _comp(value, _container.front())) {
                pop();
                push(std::move(value));
            }
        }

        /// Returns the largest element among the K smallest (top of the max‑heap).
        const value_type &top() const {
            return _container.front();
        }

        /// Removes the largest element among the K smallest.
        void pop() {
            fermat::pop_heap(_container.begin(), _container.end(), _comp);
            _container.pop_back();
        }

        [[nodiscard]] bool empty() const { return _container.empty(); }
        size_type size() const { return _container.size(); }
        size_type capacity() const { return _capacity; }

        container_type &get_container() { return _container; }
        const container_type &get_container() const { return _container; }

        void swap(MinKQueue &other) noexcept {
            using std::swap;
            swap(_container, other._container);
            swap(_capacity, other._capacity);
            swap(_comp, other._comp);
        }

        /// Moves out the underlying container sorted ascending by _comp.
        container_type aes() && {
            container_type c = std::move(_container);
            _capacity = 0;
            fermat::sort_heap(c.begin(), c.end(), _comp);
            return c;
        }

        /// Moves out the underlying container sorted descending by _comp.
        container_type des() && {
            container_type c = std::move(_container);
            _capacity = 0;
            fermat::sort_heap(c.begin(), c.end(), _comp);
            std::reverse(c.begin(), c.end());
            return c;
        }

    private:
        container_type _container;
        size_type _capacity;
        compare_type _comp;
    };

    // Free swap functions
    template<typename T, typename Compare, size_t Alignment, typename Container>
    void swap(MaxKQueue<T, Compare, Alignment, Container> &a,
              MaxKQueue<T, Compare, Alignment, Container> &b) noexcept {
        a.swap(b);
    }

    template<typename T, typename Compare, size_t Alignment, typename Container>
    void swap(MinKQueue<T, Compare, Alignment, Container> &a,
              MinKQueue<T, Compare, Alignment, Container> &b) noexcept {
        a.swap(b);
    }
} // namespace fermat
