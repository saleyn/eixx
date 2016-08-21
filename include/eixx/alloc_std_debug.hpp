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

Copyright 2010 Serge Aleynikov <saleyn at gmail dot com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

***** END LICENSE BLOCK *****
*/
#ifndef _EIXX_ALLOC_STD_HPP_
#define _EIXX_ALLOC_STD_HPP_

#include <memory>
#include <iostream>

#define EIXX_USE_ALLOCATOR

namespace eixx {

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

} // namespace eixx

#endif // _EIXX_ALLOC_STD_HPP_

