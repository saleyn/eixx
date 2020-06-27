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

    using pointer         = T*;
    using const_pointer   = T const*;
    using reference       = T&;
    using const_reference = T const&;
    using value_type      = T;

    template <typename U> struct rebind {
        using other = counted_alloc<U>;
    };

    void construct(pointer p, const T& value) {
        ::new((void*)p) T(value);
    }

    pointer allocate(size_t n) {
        ++g_alloc_count;
        return static_cast<pointer>(::operator new(n * sizeof(T)));
    }

    void deallocate(pointer p, size_t sz) {
        --g_alloc_count;
        ::operator delete(p);
    }

    void destroy(pointer p) { p->~T(); }
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
    using my_alloc = boost::pool_allocator<char>;
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
    using my_alloc = counted_alloc<char>;
    using term     = eterm<my_alloc>;
    my_alloc alloc;

    list<my_alloc> lst(nullptr);  // Allocates static global empty list
    BOOST_CHECK_EQUAL(2, g_alloc_count);

    {
        term et = list<my_alloc>({1, 2, 3}, alloc);
        BOOST_CHECK_EQUAL(4, g_alloc_count); // 1 for blob_t, 1 for blob's data
        auto am = et;
        BOOST_CHECK_EQUAL(4, g_alloc_count);
    }
    BOOST_CHECK_EQUAL(2, g_alloc_count);

    {
        // Construct a nil list
        term et = list<my_alloc>(nullptr);
        BOOST_CHECK_EQUAL(2, g_alloc_count);
        auto am = et;
        BOOST_CHECK_EQUAL(2, g_alloc_count);
    }
    BOOST_CHECK_EQUAL(2, g_alloc_count);
}
