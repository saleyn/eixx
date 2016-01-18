/*
***** BEGIN LICENSE BLOCK *****

This file is part of the EPI (Erlang Plus Interface) Library.

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
#include <boost/test/unit_test.hpp>
#include "test_alloc.hpp"
#include <eixx/marshal/eterm.hpp>
#include <eixx/marshal/list.hpp>

using namespace eixx;
using eixx::marshal::eterm;
using eixx::marshal::list;

static int g_alloc_count;

template <class T>
struct counted_alloc : public std::allocator<char> {
    counted_alloc() {}
    counted_alloc(const counted_alloc<T>& a) {}

    template <typename _T>
    counted_alloc(const _T&) {}

    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;

    template <typename _T> struct rebind {
        typedef counted_alloc<_T> other;
    };

    void construct(pointer p, const T& value) {
        ::new((void*)p) T(value);
    }

    pointer allocate(size_t sz) {
        ++g_alloc_count;
        return static_cast<pointer>(::operator new(sz * sizeof(T)));
    }

    void deallocate(pointer p, size_t sz) {
        --g_alloc_count;
        ::operator delete(p);
    }

    void destroy(pointer __p) { __p->~T(); }
};

BOOST_AUTO_TEST_CASE( test_refc_format )
{
    typedef counted_alloc<char> my_alloc;
    my_alloc alloc;

    list<my_alloc> lst(nullptr);  // Allocates static global empty list
    BOOST_CHECK_EQUAL(2, g_alloc_count);

    {
        for (int i=0; i < 10; i++) {
            BOOST_CHECK_EQUAL(2, g_alloc_count);
            eterm<my_alloc> term = eterm<my_alloc>::format(alloc,
                "[~i, [{~s, ~i}, {~a, ~i}], {~f, ~i}, ~a]", 
                  1,   "ab", 2,  "xx", 3,   2.1, 10, "abc");
            BOOST_CHECK_EQUAL(LIST, term.type());
            BOOST_CHECK_EQUAL(2+(6*2), g_alloc_count);

            for (int j=0; j <= 10; j++)
                eterm<my_alloc> term = eterm<my_alloc>::format(alloc, 
                    "{~i, [{~s, ~i}, {~a, ~i}], {~f, ~i}, ~a}", 
                      1,   "ab", 2,  "xx", 3,   2.1, 10, "abc");
        }
    }
    BOOST_CHECK_EQUAL(2, g_alloc_count);
}

BOOST_AUTO_TEST_CASE( test_refc_pool_format )
{
    typedef boost::pool_allocator<char> my_alloc;
    my_alloc alloc;
    {
        for (int i=0; i < 10; i++) {
            eterm<my_alloc> term = eterm<my_alloc>::format(alloc, 
                "[~i, [{~s, ~i}, {~a, ~i}], {~f, ~i}, ~a]", 
                  1,   "ab", 2,  "xx", 3,   2.1, 10, "abc");
            BOOST_CHECK_EQUAL(LIST, term.type());

            for (int j=0; j <= 10; j++) {
                eterm<my_alloc> term = eterm<my_alloc>::format(alloc, 
                    "{~i, [{~s, ~i}, {~a, ~i}], {~f, ~i}, ~a}", 
                      1,   "ab", 2,  "xx", 3,   2.1, 10, "abc");
                BOOST_CHECK_EQUAL(TUPLE, term.type());
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( test_refc_list )
{
    typedef counted_alloc<char> my_alloc;
    my_alloc alloc;

    list<my_alloc> lst(nullptr);  // Allocates static global empty list
    BOOST_CHECK_EQUAL(2, g_alloc_count);

    {
        eterm<my_alloc> term = list<my_alloc>({1, 2, 3}, alloc);
        BOOST_CHECK_EQUAL(4, g_alloc_count); // 1 for blob_t, 1 for blob's data
        auto term2 = term;
        BOOST_CHECK_EQUAL(4, g_alloc_count);
    }
    BOOST_CHECK_EQUAL(2, g_alloc_count);

    {
        // Construct a NIL list
        eterm<my_alloc> term = list<my_alloc>(nullptr);
        BOOST_CHECK_EQUAL(2, g_alloc_count);
        auto term2 = term;
        BOOST_CHECK_EQUAL(2, g_alloc_count);
    }
    BOOST_CHECK_EQUAL(2, g_alloc_count);
}