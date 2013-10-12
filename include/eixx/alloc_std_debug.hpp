//----------------------------------------------------------------------------
/// \file  alloc_std_debug.hpp
//----------------------------------------------------------------------------
/// \brief Allocator for debugging memory related issues. It is verbose and
///        it has a counter for counting allocated objects.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-08-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) Library.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

***** END LICENSE BLOCK *****
*/
#ifndef _EIXX_ALLOC_STD_HPP_
#define _EIXX_ALLOC_STD_HPP_

#include <eixx/namespace.hpp>  // definition of EIXX_NAMESPACE
#include <memory>
#include <iostream>

#define EIXX_USE_ALLOCATOR

namespace EIXX_NAMESPACE {

namespace detail {
    template <typename T>
    class debug_allocator : public std::allocator<T> {
        typedef std::allocator<T> base_t;
        int count(int add = 0) {
            static int s_count;
            if (add) s_count += add;
            return s_count;
        }

    public:
        debug_allocator() throw() {}
        debug_allocator(const base_t& a) throw() : base_t(a) {}
        debug_allocator(const debug_allocator& a) throw() : base_t(a) {}
        template <typename U>
        debug_allocator(const debug_allocator<U>&) throw() {}

        template<typename U> struct rebind { 
            typedef debug_allocator<U> other; 
        };

        T* allocate(size_t sz) {
            T* p = base_t::allocate(sz);
            std::cout << "Allocated " << sizeof(T)*sz << " bytes [" << (void*)p << "] count=" 
                      << count(1) << std::endl;
            return p;
        }

        void deallocate(T* p, size_t sz) {
            std::cout << "Freed [" << (void*)p << "] sz=" << sizeof(T)*sz 
                      << ", count=" << count(-1) << std::endl;
            base_t::deallocate(p, sz);
        }

        void construct(void* p, const T& v) {
            std::cout << "Constructing " << p << " size=" << sizeof(T) << std::endl;
            ::new (p) T(v);
        }
    };
}

struct allocator_t : public detail::debug_allocator<char>
{
    typedef detail::debug_allocator<char> base_t;

    allocator_t() throw() {}
    allocator_t(const base_t& a) throw() : base_t(a) {}
    allocator_t(const allocator_t& a) throw() : base_t(a) {}

    template<typename T> struct rebind { 
        typedef detail::debug_allocator<T> other; 
    };
};

} // namespace EIXX_NAMESPACE

#endif // _EIXX_ALLOC_STD_HPP_

