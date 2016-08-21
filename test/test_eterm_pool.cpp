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


