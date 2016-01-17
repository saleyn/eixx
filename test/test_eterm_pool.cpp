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

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_encode_atom_t )
{
    atom a("abc");
    string s(eterm(a).encode(0));
    const uint8_t expect[] = {131,100,0,3,97,98,99};
    BOOST_REQUIRE(s.equal(expect));
}

BOOST_AUTO_TEST_CASE( test_encode_binary_t )
{
    const char data[] = {1,2,3,4,5,6,7,8,9,10,11,12,13};
    binary t(data, sizeof(data));
    string s(eterm(t).encode(0));
    const uint8_t expect[] = {131,109,0,0,0,13,1,2,3,4,5,6,7,8,9,10,11,12,13};
    BOOST_REQUIRE(s.equal(expect));
}

BOOST_AUTO_TEST_CASE( test_encode_double_t )
{
    double d = 12345.6789;
    eterm t(d);
    string s(t.encode(0));
    const uint8_t expect[] = {131,70,64,200,28,214,230,49,248,161};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    eterm t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(d, t1.to_double());
}

BOOST_AUTO_TEST_CASE( test_encode_long_t )
{
    long d = 123;
    eterm t(d);
    string s(t.encode(0));
    const uint8_t expect[] = {131,97,123};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    eterm t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(d, t1.to_long());
}


