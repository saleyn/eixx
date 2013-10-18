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

BOOST_AUTO_TEST_CASE( test_encode_string )
{
    eterm t("abc");
    string s(t.encode(0));
    const uint8_t expect[] = {131,107,0,3,97,98,99};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    string t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(3ul, t1.size());
    eterm et(t1);
    std::string str( et.to_string() );
    if (!(et == t))
    BOOST_REQUIRE_EQUAL(et, t);
    if (str != "\"abc\"")
    BOOST_REQUIRE_EQUAL("\"abc\"", str);
}

BOOST_AUTO_TEST_CASE( test_encode_atom )
{
    eterm a(atom("abc"));
    string s(a.encode(0));
    const uint8_t expect[] = {131,100,0,3,97,98,99};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    atom t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(3ul, t1.size());
    BOOST_REQUIRE_EQUAL("abc", eterm(t1).to_string());
}

BOOST_AUTO_TEST_CASE( test_encode_binary )
{
    const char data[] = {1,2,3,4,5,6,7,8,9,10,11,12,13};
    eterm t(binary(data, sizeof(data)));
    string s(t.encode(0));
    const uint8_t expect[] = {131,109,0,0,0,13,1,2,3,4,5,6,7,8,9,10,11,12,13};
    BOOST_REQUIRE(s.equal(expect));
}

BOOST_AUTO_TEST_CASE( test_encode_double )
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

BOOST_AUTO_TEST_CASE( test_encode_emptylist )
{
    list l(0);
    BOOST_REQUIRE(l.initialized());
    eterm t(l);
    string s(t.encode(0));
    const uint8_t expect[] = {131,106};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    eterm t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(LIST, t1.type());
    BOOST_REQUIRE_EQUAL(0ul, t1.to_list().length());
}

BOOST_AUTO_TEST_CASE( test_encode_list )
{
    eterm ll[] = {
        eterm(atom("abc")),
        eterm("ef"),
        eterm(1),
        eterm("gh")
    };
    list l(ll);
    eterm t(l);
    string s(t.encode(0));
    const uint8_t expect[] = {131,108,0,0,0,4,100,0,3,97,98,99,107,0,2,101,102,
                              97,1,107,0,2,103,104,106};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    list t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(4ul, t1.length());
    std::string str(eterm(t1).to_string());
    BOOST_REQUIRE_EQUAL("[abc,\"ef\",1,\"gh\"]", str);
}

BOOST_AUTO_TEST_CASE( test_encode_long )
{
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
    {
        long d = 12345;
        eterm t(d);
        string s(t.encode(0));
        const uint8_t expect[] = {131,98,0,0,48,57};
        BOOST_REQUIRE(s.equal(expect));
        int idx = 1;  // skipping the magic byte
        eterm t1((const char*)expect, idx, sizeof(expect));
        BOOST_REQUIRE_EQUAL(d, t1.to_long());
    }
    {
        long d = 12345678901;
        eterm t(d);
        string s(t.encode(0));
        const uint8_t expect[] = {131,110,5,0,53,28,220,223,2};
        BOOST_REQUIRE(s.equal(expect));
        int idx = 1;  // skipping the magic byte
        eterm t1((const char*)expect, idx, sizeof(expect));
        BOOST_REQUIRE_EQUAL(d, t1.to_long());
    }
}

BOOST_AUTO_TEST_CASE( test_encode_pid )
{
    {
        eterm t(epid("test@host", 1, 2, 0));
        BOOST_REQUIRE_EQUAL("#Pid<test@host.1.2.0>", t.to_string());
        string s(t.encode(0));
        //std::cout << s.to_binary_string() << std::endl;
        const uint8_t expect[] = 
            {131,103,100,0,9,116,101,115,116,64,104,111,115,116,0,0,0,1,0,0,0,2,0};
        BOOST_REQUIRE(s.equal(expect));
        int idx = 1;  // skipping the magic byte
        eterm pid(epid((const char*)expect, idx, sizeof(expect)));
        BOOST_REQUIRE_EQUAL(eterm(pid), t);
    }
    {
        const uint8_t expect[] =
            {131,103,100,0,8,97,98,99,64,102,99,49,50,0,0,0,96,0,0,0,0,3};
        int idx = 1;  // skipping the magic byte
        epid decode_pid((const char*)expect, idx, sizeof(expect));
        epid expect_pid("abc@fc12", 96, 0, 3);
        BOOST_REQUIRE_EQUAL(expect_pid, decode_pid);
    }
}

BOOST_AUTO_TEST_CASE( test_encode_port )
{
    eterm t(port("test@host", 1, 0));
    BOOST_REQUIRE_EQUAL("#Port<test@host.1>", t.to_string());
    string s(t.encode(0));
    //std::cout << s.to_binary_string() << std::endl;
    const uint8_t expect[] =
        {131,102,100,0,9,116,101,115,116,64,104,111,115,116,0,0,0,1,0};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    eterm t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(t1, t);
}

BOOST_AUTO_TEST_CASE( test_encode_ref )
{
    uint32_t ids[] = {1,2,3};
    eterm t(ref("test@host", ids, 0));
    BOOST_REQUIRE_EQUAL("#Ref<test@host.1.2.3>", t.to_string());
    string s(t.encode(0));
    //std::cout << s.to_binary_string() << std::endl;
    const uint8_t expect[] = {131,114,0,3,100,0,9,116,101,115,116,64,104,111,115,
                              116,0,0,0,0,1,0,0,0,2,0,0,0,3};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    ref t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(eterm(t1), t);
    {
        ref t(atom("abc@fc12"), 993, 0, 0, 2);
        const uint8_t expect[] =
            {131,114,0,3,100,0,8,97,98,99,64,102,99,49,50,2,0,0,3,225,0,0,0,0,0,0,0,0};
        int idx = 1;  // skipping the magic byte
        ref t1((const char*)expect, idx, sizeof(expect));
        BOOST_REQUIRE_EQUAL(t1, t);
    }
}

BOOST_AUTO_TEST_CASE( test_encode_tuple )
{
    eterm list_inner[] = {
        eterm(atom("a")),
        eterm("xx"),
        eterm(123.1),
        eterm(5),
    };
    eterm list[] = {
        eterm(atom("b")),
        eterm("ef"),
        eterm(1),
        eterm(tuple(list_inner)),
        eterm("gh")
    };
    tuple tup(list);
    eterm t(tup);
    string s(t.encode(0));
    //std::cout << s.to_binary_string() << std::endl;
    const uint8_t expect[] = {131,104,5,100,0,3,97,98,99,107,0,2,101,102,97,1,
                              104,4,100,0,1,97,107,0,2,120,120,70,64,94,198,102,
                              102,102,102,102,97,5,107,0,2,103,104};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    tuple t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(5ul, t1.size());
    BOOST_REQUIRE_EQUAL("{abc,\"ef\",1,{a,\"xx\",123.1,5},\"gh\"}", eterm(t1).to_string());
}

BOOST_AUTO_TEST_CASE( test_encode_trace )
{
    epid  self("abc@fc12",96,0,3);
    trace tr(1,2,3,self,4);
    eterm t(tr);
    string s(t.encode(0));
    //std::cout << s.to_binary_string() << std::endl;
    const uint8_t expect[] = {131,104,5,97,1,97,2,97,3,103,100,0,8,97,98,99,64,
                              102,99,49,50,0,0,0,96,0,0,0,0,3,97,4};
    BOOST_REQUIRE(s.equal(expect));
    int idx = 1;  // skipping the magic byte
    trace t1((const char*)expect, idx, sizeof(expect));
    BOOST_REQUIRE_EQUAL(5ul, t1.size());
    BOOST_REQUIRE_EQUAL(1,   t1.flags());
    BOOST_REQUIRE_EQUAL(2,   t1.label());
    BOOST_REQUIRE_EQUAL(3,   t1.serial());
    BOOST_REQUIRE(self == t1.from());
    BOOST_REQUIRE_EQUAL(4,   t1.prev());
    BOOST_REQUIRE_EQUAL("{1,2,3,#Pid<abc@fc12.96.0.3>,4}", eterm(t1).to_string());
}

BOOST_AUTO_TEST_CASE( test_encode_rpc )
{
    static const unsigned char s_expected[] = {
        131,104,2,103,100,0,14,69,67,71,46,72,49,46,48,48,49,64,102,49,54,0,0,0,1,
        0,0,0,0,0,104,5,100,0,4,99,97,108,108,100,0,7,101,99,103,95,97,112,105,100,
        0,11,114,101,103,95,112,114,111,99,101,115,115,108,0,0,0,5,100,0,3,69,67,71,
        100,0,10,69,67,71,46,72,49,46,48,48,49,103,100,0,14,69,67,71,46,72,49,46,48,
        48,49,64,102,49,54,0,0,0,1,0,0,0,0,0,107,0,12,101,120,97,109,112,108,101,95,
        99,111,114,101,98,0,0,7,208,106,100,0,4,117,115,101,114
    };

    epid l_pid("ECG.H1.001@f16", 1, 0, 0);
    eterm l_term =
        tuple::make(l_pid,
            tuple::make(atom("call"), atom("ecg_api"), atom("reg_process"),
                list::make(atom("ECG"), atom("ECG.H1.001"), l_pid, "example_core", 2000),
                atom("user")));

    //{#Pid<'ECG.H1.001@f16'.1.0.0>,
    //  {call,ecg_api,reg_process,
    //    ['ECG','ECG.H1.001',#Pid<'ECG.H1.001@f16'.1.0.0>,"example_core",2000],user}}
    char l_buf[256];
    size_t l_sz = l_term.encode_size(0, true);
    BOOST_REQUIRE(l_sz < sizeof(l_buf));
    l_term.encode(l_buf, l_sz, 0, true);
    std::string s = std::string(l_buf, l_sz);
    std::string s_exp = std::string(l_buf, sizeof(s_expected));
    if (s != s_exp)
        std::cout << "String: " << to_binary_string(s.c_str(), s.size()) << std::endl;
    BOOST_REQUIRE_EQUAL(s_exp, s);
}

