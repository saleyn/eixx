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

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_eterm_format_string )
{
    allocator_t alloc;

    eterm et( eterm::format(alloc, "~s", "abc") );

    BOOST_REQUIRE_EQUAL(STRING, et.type());
    BOOST_REQUIRE_EQUAL("abc", et.to_str());
}

BOOST_AUTO_TEST_CASE( test_eterm_format_binary )
{
    allocator_t alloc;

    eterm et = eterm::format(alloc, "<<\"abc\">>");

    BOOST_REQUIRE_EQUAL(BINARY, et.type());
    BOOST_REQUIRE_EQUAL("abc", std::string(et.to_binary().data(), et.to_binary().size()));

    et = eterm::format(alloc, "<<65,66, 67>>");
    BOOST_REQUIRE_EQUAL(BINARY, et.type());
    BOOST_REQUIRE_EQUAL("ABC", std::string(et.to_binary().data(), et.to_binary().size()));

    et = eterm::format(alloc, "<<>>");
    BOOST_REQUIRE_EQUAL(BINARY, et.type());
    BOOST_REQUIRE_EQUAL("", std::string(et.to_binary().data(), et.to_binary().size()));

    et = eterm::format(alloc, "<<\"\">>");
    BOOST_REQUIRE_EQUAL(BINARY, et.type());
    BOOST_REQUIRE_EQUAL("", std::string(et.to_binary().data(), et.to_binary().size()));

    BOOST_CHECK_THROW(eterm::format("<<-1>>"), err_format_exception);
    BOOST_CHECK_THROW(eterm::format("<<1,2 3>>"), err_format_exception);
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

BOOST_AUTO_TEST_CASE( test_eterm_var_type )
{
    BOOST_REQUIRE_EQUAL(UNDEFINED,  eterm::format("B").to_var().type());
    BOOST_REQUIRE_EQUAL(LONG,       eterm::format("B::int()").to_var().type());
    BOOST_REQUIRE_EQUAL(LONG,       eterm::format("B::byte()").to_var().type());
    BOOST_REQUIRE_EQUAL(LONG,       eterm::format("B::char()").to_var().type());
    BOOST_REQUIRE_EQUAL(LONG,       eterm::format("B::integer()").to_var().type());
    BOOST_REQUIRE_EQUAL(STRING,     eterm::format("B::string()").to_var().type());
    BOOST_REQUIRE_EQUAL(ATOM,       eterm::format("B::atom()").to_var().type());
    BOOST_REQUIRE_EQUAL(DOUBLE,     eterm::format("B::float()").to_var().type());
    BOOST_REQUIRE_EQUAL(DOUBLE,     eterm::format("B::double()").to_var().type());
    BOOST_REQUIRE_EQUAL(BINARY,     eterm::format("B::binary()").to_var().type());
    BOOST_REQUIRE_EQUAL(BOOL,       eterm::format("B::bool()").to_var().type());
    BOOST_REQUIRE_EQUAL(BOOL,       eterm::format("B::boolean()").to_var().type());
    BOOST_REQUIRE_EQUAL(LIST,       eterm::format("B::list()").to_var().type());
    BOOST_REQUIRE_EQUAL(TUPLE,      eterm::format("B::tuple()").to_var().type());
    BOOST_REQUIRE_EQUAL(PID,        eterm::format("B::pid()").to_var().type());
    BOOST_REQUIRE_EQUAL(REF,        eterm::format("B::ref()").to_var().type());
    BOOST_REQUIRE_EQUAL(REF,        eterm::format("B::reference()").to_var().type());
    BOOST_REQUIRE_EQUAL(PORT,       eterm::format("B::port()").to_var().type());
}

BOOST_AUTO_TEST_CASE( test_eterm_mfa_format )
{
    atom m, f;
    eterm args;

    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b()"));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b()."));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b()\t"));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b()\t ."));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b() "));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b()\n."));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b( %comment\n)."));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b().%comment"));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b(10)"));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b(10)."));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "aa:bb(10)"));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b(10,20)."));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b(~i).", 10));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b(~f,~i).", 20.0, 10));
    BOOST_REQUIRE_NO_THROW(eterm::format(m, f, args, "a:b([~i,1], {ok,'a'}).", 10));
}

BOOST_AUTO_TEST_CASE( test_eterm_mfa_format_bad )
{
    atom m, f;
    eterm args;

    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b(1, %comment\n"), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b(1, %comment 2)."), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "("), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, ")"), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "."), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "aa"), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a("), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b("), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a.b()"), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b(10 20)"), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b(10. 20)"), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b(10.(20)"), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b(~i,~i]", 10, 20), err_format_exception);
    BOOST_REQUIRE_THROW(eterm::format(m, f, args, "a:b([[~i,20],]", 10), err_format_exception);
}