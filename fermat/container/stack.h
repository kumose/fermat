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
#include <initializer_list>
#include <cstddef>


namespace fermat {
    /// stack
    ///
    /// stack is an adapter class provides a LIFO (last-in, first-out) interface
    /// via wrapping a sequence container (https://en.cppreference.com/w/cpp/named_req/SequenceContainer)
    /// that additionally provides the following operations:
    ///     push_back
    ///     pop_back
    ///     back
    ///
    /// In practice this means vector, deque, list, intrusive_list and (the pseudo-container) string.
    ///
    /// Note: the default underlying container is vector, rather than the standard's deque.
    ///
    template<typename T, size_t Alignment, typename Container = fermat::Vector<T, Alignment> >
    class stack {
    public:
        typedef stack<T, Alignment, Container> this_type;
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
        stack();

        // Allocator is templated here because we aren't allowed to infer the allocator_type from the Container, as some containers (e.g. array) don't
        // have allocators. For containers that don't have allocator types, you could use void or char as the Allocator template type.

        template<class Allocator>
        explicit stack(const Allocator &allocator)
            : c(allocator) {
        }

        template<class Allocator>
        stack(const this_type &x, const Allocator &allocator)
            : c(x.c, allocator) {
        }

        template<class Allocator>
        stack(this_type &&x, const Allocator &allocator)
            : c(std::move(x.c), allocator) {
        }

        explicit stack(const container_type &x);

        explicit stack(container_type &&x);

        // Additional C++11 support to consider:
        //
        // template <class Allocator>
        // stack(const container_type& x, const Allocator& allocator);
        //
        // template <class Allocator>
        // stack(container_type&& x, const Allocator& allocator);
        //
        // template <class InputIt>
        // stack(InputIt first, InputIt last);
        //
        // template <class InputIt, class Allocator>
        // stack(InputIt first, InputIt last, const Allocator& allocator);

        stack(std::initializer_list<value_type> ilist);

        // The first item in the initializer list is pushed first. C++11 doesn't specify that std::stack has initializer list support.

        bool empty() const;

        size_type size() const;

        reference top();

        const_reference top() const;

        void push(const value_type &value);

        void push(value_type &&x);

        template<class... Args>
        decltype(auto) emplace(Args &&... args);

        void pop();

        container_type &get_container();

        const container_type &get_container() const;

        void swap(this_type &x) noexcept(std::is_nothrow_swappable<typename this_type::container_type>::value);

        bool validate() const;
    }; // class stack


    ///////////////////////////////////////////////////////////////////////
    // stack
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Container>
    inline stack<T, Alignment, Container>::stack()
        : c()
    // To consider: use c(FERMAT_STACK_DEFAULT_ALLOCATOR) here, though that would add the requirement that the user supplied container support this.
    {
        // Empty
    }


    template<typename T, size_t Alignment, typename Container>
    inline stack<T, Alignment, Container>::stack(const Container &x)
        : c(x) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Container>
    inline stack<T, Alignment, Container>::stack(Container &&x)
        : c(std::move(x)) {
        // Empty
    }


    template<typename T, size_t Alignment, typename Container>
    inline stack<T, Alignment, Container>::stack(std::initializer_list<value_type> ilist)
        : c() // We could alternatively use c(ilist) here, but that would require c to have an ilist constructor.
    {
        // Better solution but requires an insert function.
        // c.insert(ilist.begin(), ilist.end());

        // Possibly slower solution but doesn't require an insert function.
        for (const auto &value: ilist) {
            c.push_back(value);
        }
    }

    template<typename T, size_t Alignment, typename Container>
    inline bool stack<T, Alignment, Container>::empty() const {
        return c.empty();
    }


    template<typename T, size_t Alignment, typename Container>
    inline typename stack<T, Alignment, Container>::size_type
    stack<T, Alignment, Container>::size() const {
        return c.size();
    }


    template<typename T, size_t Alignment, typename Container>
    inline typename stack<T, Alignment, Container>::reference
    stack<T, Alignment, Container>::top() {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("stack::top -- empty container");
#endif

        return c.back();
    }


    template<typename T, size_t Alignment, typename Container>
    inline typename stack<T, Alignment, Container>::const_reference
    stack<T, Alignment, Container>::top() const {
#if FERMAT_ASSERT_ENABLED && FERMAT_EMPTY_REFERENCE_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("stack::top -- empty container");
#endif

        return c.back();
    }


    template<typename T, size_t Alignment, typename Container>
    inline void stack<T, Alignment, Container>::push(const value_type &value) {
        c.push_back(const_cast<value_type &>(value));
        // const_cast so that intrusive_list can work. We may revisit this.
    }


    template<typename T, size_t Alignment, typename Container>
    inline void stack<T, Alignment, Container>::push(value_type &&x) {
        c.push_back(std::move(x));
    }


    template<typename T, size_t Alignment, typename Container>
    template<class... Args>
    inline decltype(auto) stack<T, Alignment, Container>::emplace(Args &&... args) {
        return c.emplace_back(std::forward<Args>(args)...);
    }


    template<typename T, size_t Alignment, typename Container>
    inline void stack<T, Alignment, Container>::pop() {
#if FERMAT_ASSERT_ENABLED
        if (FERMAT_UNLIKELY(c.empty()))
            FERMAT_FAIL_MSG("stack::pop -- empty container");
#endif

        c.pop_back();
    }


    template<typename T, size_t Alignment, typename Container>
    inline typename stack<T, Alignment, Container>::container_type &
    stack<T, Alignment, Container>::get_container() {
        return c;
    }


    template<typename T, size_t Alignment, typename Container>
    inline const typename stack<T, Alignment, Container>::container_type &
    stack<T, Alignment, Container>::get_container() const {
        return c;
    }


    template<typename T, size_t Alignment, typename Container>
    void stack<T, Alignment, Container>::swap(
        this_type &x) noexcept (std::is_nothrow_swappable<typename this_type::container_type>::value) {
        using std::swap;
        swap(c, x.c);
    }


    template<typename T, size_t Alignment, typename Container>
    bool stack<T, Alignment, Container>::validate() const {
        return c.validate();
    }


    ///////////////////////////////////////////////////////////////////////
    // global operators
    ///////////////////////////////////////////////////////////////////////

    template<typename T, size_t Alignment, typename Container>
    inline bool operator==(const stack<T, Alignment, Container> &a, const stack<T, Alignment, Container> &b) {
        return (a.c == b.c);
    }

#if defined(KM_COMPILER_HAS_THREE_WAY_COMPARISON)
    template<typename T, size_t Alignment, typename Container> requires std::three_way_comparable<Container>
    inline synth_three_way_result<T> operator<=>(const stack<T, Alignment, Container> &a, const stack<T, Alignment,
        Container> &b)
	{
		return a.c <=> b.c;
	}
#endif

    template<typename T, size_t Alignment, typename Container>
    inline bool operator!=(const stack<T, Alignment, Container> &a, const stack<T, Alignment, Container> &b) {
        return !(a.c == b.c);
    }


    template<typename T, size_t Alignment, typename Container>
    inline bool operator<(const stack<T, Alignment, Container> &a, const stack<T, Alignment, Container> &b) {
        return (a.c < b.c);
    }


    template<typename T, size_t Alignment, typename Container>
    inline bool operator>(const stack<T, Alignment, Container> &a, const stack<T, Alignment, Container> &b) {
        return (b.c < a.c);
    }


    template<typename T, size_t Alignment, typename Container>
    inline bool operator<=(const stack<T, Alignment, Container> &a, const stack<T, Alignment, Container> &b) {
        return !(b.c < a.c);
    }


    template<typename T, size_t Alignment, typename Container>
    inline bool operator>=(const stack<T, Alignment, Container> &a, const stack<T, Alignment, Container> &b) {
        return !(a.c < b.c);
    }
} // namespace fermat

namespace std {
    template<typename T, size_t Alignment, typename Container>
    inline void swap(fermat::stack<T, Alignment, Container> &a,
                     fermat::stack<T, Alignment, Container> &b) noexcept((std::is_nothrow_swappable<typename
        fermat::stack<T, Alignment,
            Container>::container_type>::value)) {
        a.swap(b);
    }
}
