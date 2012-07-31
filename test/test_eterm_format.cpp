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
#include <eixx/eixx.hpp>
#include <eixx/marshal/eterm_format.hpp>

using namespace EIXX_NAMESPACE;

BOOST_AUTO_TEST_CASE( test_eterm_format_string )
{
    allocator_t alloc;

    eterm et( eterm::format(alloc, "~s", "abc") );

    BOOST_REQUIRE_EQUAL(STRING, et.type());
    BOOST_REQUIRE_EQUAL("abc", et.to_str());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_atom )
{
    allocator_t alloc;

    eterm et( eterm::format(alloc, "~a", "abc") );

    BOOST_REQUIRE_EQUAL(ATOM, et.type());
    BOOST_REQUIRE_EQUAL("abc", et.to_atom());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_long )
{
    allocator_t alloc;

    eterm et( eterm::format(alloc, "~i", 10) );

    BOOST_REQUIRE_EQUAL(LONG, et.type());
    BOOST_REQUIRE_EQUAL(10, et.to_long());

    et.set( eterm::format(alloc, "~l", -100l) );

    BOOST_REQUIRE_EQUAL(LONG, et.type());
    BOOST_REQUIRE_EQUAL(-100, et.to_long());

    et.set( eterm::format(alloc, "~u", 1000ul) );

    BOOST_REQUIRE_EQUAL(LONG, et.type());
    BOOST_REQUIRE_EQUAL(1000, et.to_long());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_double )
{
    allocator_t alloc;

    eterm et( eterm::format("~f", 2.0) );

    BOOST_REQUIRE_EQUAL(DOUBLE, et.type());
    BOOST_REQUIRE_EQUAL(2.0, et.to_double());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_tuple )
{
    allocator_t alloc;

    eterm et( eterm::format("{~i, ~f, ~a}", 1, 2.1, "abc") );

    BOOST_REQUIRE_EQUAL(TUPLE, et.type());
    BOOST_REQUIRE_EQUAL("{1,2.1,abc}", et.to_string());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_list )
{
    allocator_t alloc;

    eterm et( eterm::format("[~i, ~f, ~a]", 1, 2.1, "abc") );

    BOOST_REQUIRE_EQUAL(LIST, et.type());
    BOOST_REQUIRE_EQUAL("[1,2.1,abc]", et.to_string());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_const )
{
    allocator_t alloc;

    eterm et = eterm::format(alloc, 
        "[~i, 10, 2.5, abc, \"efg\", {~f, ~i}, ~a]", 
          1,                         2.1, 10, "xx");

    BOOST_REQUIRE_EQUAL(LIST, et.type());
    BOOST_REQUIRE_EQUAL("[1,10,2.5,abc,\"efg\",{2.1,10},xx]", et.to_string());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_compound )
{
    allocator_t alloc;

    eterm a(atom("xyz"));

    eterm et = eterm::format(alloc, 
        "[~i, [{~s, ~i}, {~a, ~i}], {~f, ~i}, ~w, ~a]", 
          1,   "ab", 2,  "xx", 3,   2.1, 10, &a, "abc");

    BOOST_REQUIRE_EQUAL(LIST, et.type());
    BOOST_REQUIRE_EQUAL("[1,[{\"ab\",2},{xx,3}],{2.1,10},xyz,abc]", et.to_string());
}


