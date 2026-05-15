///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a queue that is just like the C++ std::queue adapter class.
// There are no significant differences between EASTL/queue and std::queue.
// We provide this class for completeness and where std STL may not be available.
///////////////////////////////////////////////////////////////////////////////
#pragma once
#include <fermat/types/internal/config.h>
#include <fermat/types/deque.h>
#include <initializer_list>
#include <cstddef>
#include <fermat/memory/allocator.h>

namespace fermat {
    /// FERMAT_QUEUE_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
#ifndef FERMAT_QUEUE_DEFAULT_NAME
#define FERMAT_QUEUE_DEFAULT_NAME FERMAT_DEFAULT_NAME_PREFIX " queue" // Unless the user overrides something, this is "EASTL queue".


#endif

    /// FERMAT_QUEUE_DEFAULT_ALLOCATOR
    ///
#ifndef FERMAT_QUEUE_DEFAULT_ALLOCATOR
#define FERMAT_QUEUE_DEFAULT_ALLOCATOR allocator_type(FERMAT_QUEUE_DEFAULT_NAME)
#endif


    /// queue
    ///
    /// queue is an adapter class provides a FIFO (first-in, first-out) interface
    /// via wrapping a sequence container (https://en.cppreference.com/w/cpp/named_req/SequenceContainer)
    /// that additionally provides:
    ///     push_back
    ///     pop_front
    ///     front
    ///     back
    ///
    /// In practice this means deque, list, intrusive_list. vector and (the pseudo-container) string
    /// cannot be used because they don't provide pop_front. This is reasonable because
    /// a vector or string pop_front would be inefficient as such an operation would have linear complexity
    /// (to move elements after removing the front element, maintaining ordering).
    ///
    template<typename T, size_t Alignment, typename Container = fermat::deque<T, Alignment, BasicAllocator<T, Alignment>, DEQUE_DEFAULT_SUBARRAY_SIZE(T)> >
    class queue {
    public:
        typedef queue<T, Alignment, Container> this_type;
        typedef Container container_type;
        //typedef typename Container::allocator_type   allocator_type;  // We can't currently declare this because the container may be a type that doesn't have an allocator.
        typedef typename Container::value_type value_type;
        typedef typename Container::reference reference;
        typedef typename Container::const_reference const_reference;
        typedef typename Container::size_type size_type;

    public:
        // We declare public so that global comparison operators can be implemented without adding an inline level and without tripping up GCC 2.x friend declaration failures. GCC (through at least v4.0) is poor at inlining and performance wins over correctness.
        container_type c;
        // The C++ standard specifies that you declare a protected member variable of type Container called 'c'.

    public:
        queue();

        // Allocator is templated here because we aren't allowed to infer the allocator_type from the Container, as some containers (e.g. array) don't
        // have allocators. For containers that don't have allocator types, you could use void or char as the Allocator template type.

        template<class Allocator>
        explicit queue(const Allocator &allocator,
                       typename std::enable_if<fermat::uses_allocator<container_type, Allocator>::value>::type * = NULL)
            : c(allocator) {
        }

        template<class Allocator>
        queue(const this_type &x, const Allocator &allocator,
              typename std::enable_if<fermat::uses_allocator<container_type, Allocator>::value>::type * = NULL)
            : c(x.c, allocator) {
        }

        template<class Allocator>
        queue(this_type &&x, const Allocator &allocator,
              typename std::enable_if<fermat::uses_allocator<container_type, Allocator>::value>::type * = NULL)
            : c(std::move(x.c), allocator) {
        }

        explicit queue(const container_type &x);

        explicit queue(container_type &&x);

        // Additional C++11 support to consider:
        //
        // template <class Allocator>
        // queue(const container_type& x, const Allocator& allocator);
        //
        // template <class Allocator>
        // queue(container_type&& x, const Allocator& allocator);
        //
        // template <class InputIt>
        // queue(InputIt first, InputIt last);
        //
        // template <class InputIt, class Allocator>
        // queue(InputIt first, InputIt last, const Allocator& allocator);

        queue(std::initializer_list<value_type> ilist);

        // C++11 doesn't specify that std::queue has initializer list support.

        bool empty() const;

        size_type size() const;

        reference front();

        const_reference front() const;

        reference back();

        const_reference back() const;

        void push(const value_type &value);

        void push(value_type &&x);

        template<class... Args>
        decltype(auto) emplace(Args &&... args);

        void pop();

        container_type &get_container();

        const container_type &get_container() const;

        void swap(this_type &x) KM_NOEXCEPT_IF((std::is_nothrow_swappable<this_type::container_type>::value));

        bool validate() const;
    }; // class queue


    ///////////////////////////////////////////////////////////////////////
    // queue
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline queue<T, Alignment, Allocator>::queue()
        : c()
    // To consider: use c(FERMAT_QUEUE_DEFAULT_ALLOCATOR) here, though that would add the requirement that the user supplied container support this.
    {
        // Empty
    }


    template<typename T, size_t Alignment, typename Container>
    inline queue<T, Alignment, Container>::queue(const Container &x)
        : c(x) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Container>
    inline queue<T, Alignment, Container>::queue(Container &&x)
        : c(std::move(x)) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline queue<T, Alignment, Allocator>::queue(std::initializer_list<value_type> ilist)
        : c() // We could alternatively use c(ilist) here, but that would require c to have an ilist constructor.
    {
        // Better solution but requires an insert function.
        // c.insert(ilist.begin(), ilist.end());

        // Possibly slower solution but doesn't require an insert function.
        for (typename std::initializer_list<value_type>::iterator it = ilist.begin(); it != ilist.end(); ++it) {
            const value_type &value = *it;
            c.push_back(value);
        }
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline bool queue<T, Alignment, Allocator>::empty() const {
        return c.empty();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename queue<T, Alignment, Allocator>::size_type
    queue<T, Alignment, Allocator>::size() const {
        return c.size();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename queue<T, Alignment, Allocator>::reference
    queue<T, Alignment, Allocator>::front() {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("queue::front -- empty container");
#endif

        return c.front();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename queue<T, Alignment, Allocator>::const_reference
    queue<T, Alignment, Allocator>::front() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("queue::front -- empty container");
#endif

        return c.front();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename queue<T, Alignment, Allocator>::reference
    queue<T, Alignment, Allocator>::back() {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("queue::back -- empty container");
#endif

        return c.back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename queue<T, Alignment, Allocator>::const_reference
    queue<T, Alignment, Allocator>::back() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("queue::back -- empty container");
#endif

        return c.back();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void queue<T, Alignment, Allocator>::push(const value_type &value) {
        c.push_back(const_cast<value_type &>(value));
        // const_cast so that intrusive_list can work. We may revisit this.
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void queue<T, Alignment, Allocator>::push(value_type &&x) {
        c.push_back(std::move(x));
    }

    template<typename T, size_t Alignment, typename Allocator>
    template<class... Args>
    inline decltype(auto) queue<T, Alignment, Allocator>::emplace(Args &&... args) {
        return c.emplace_back(std::forward<Args>(args)...);
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline void queue<T, Alignment, Allocator>::pop() {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("queue::pop -- empty container");
#endif

        c.pop_front();
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline typename queue<T, Alignment, Allocator>::container_type &
    queue<T, Alignment, Allocator>::get_container() {
        return c;
    }


    template<typename T, size_t Alignment, typename Allocator>
    inline const typename queue<T, Alignment, Allocator>::container_type &
    queue<T, Alignment, Allocator>::get_container() const {
        return c;
    }


    template<typename T, size_t Alignment, typename Allocator>
    void queue<T, Alignment, Allocator>::swap(
        this_type &x) KM_NOEXCEPT_IF((std::is_nothrow_swappable<this_type::container_type>::value)) {
        using std::swap;
        swap(c, x.c);
    }


    template<typename T, size_t Alignment, typename Allocator>
    bool queue<T, Alignment, Allocator>::validate() const {
        return c.validate();
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator==(const queue<T, Alignment, Allocator> &a, const queue<T, Alignment, Allocator> &b) {
        return (a.c == b.c);
    }
#if defined(KM_COMPILER_HAS_THREE_WAY_COMPARISON)
    template<typename T, size_t Alignment, typename Allocator> requires std::three_way_comparable<Container>

    inline synth_three_way_result<T> operator<=>(const queue<T, Alignment, Allocator> &a, const queue<T, Alignment, Allocator> &b)
	{
		return a.c <=> b.c;
	}
#endif

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator!=(const queue<T, Alignment, Allocator> &a, const queue<T, Alignment, Allocator> &b) {
        return !(a.c == b.c);
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator<(const queue<T, Alignment, Allocator> &a, const queue<T, Alignment, Allocator> &b) {
        return (a.c < b.c);
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator>(const queue<T, Alignment, Allocator> &a, const queue<T, Alignment, Allocator> &b) {
        return (b.c < a.c);
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator<=(const queue<T, Alignment, Allocator> &a, const queue<T, Alignment, Allocator> &b) {
        return !(b.c < a.c);
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline bool operator>=(const queue<T, Alignment, Allocator> &a, const queue<T, Alignment, Allocator> &b) {
        return !(a.c < b.c);
    }

    template<typename T, size_t Alignment, typename Allocator>
    inline void swap(queue<T, Alignment, Allocator> &a,
                     queue<T, Alignment, Allocator> &b) KM_NOEXCEPT_IF(
        (std::is_nothrow_swappable<typename queue<T, Alignment, Allocator>::container_type>::value))
    // EDG has a bug and won't let us use Container in this noexcept statement
    {
        a.swap(b);
    }
} // namespace fermat
