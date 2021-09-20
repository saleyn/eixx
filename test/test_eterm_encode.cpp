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
#include <ei.h>

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_encode_string )
{
    eterm t("abc");
    string s(t.encode(0));
    const uint8_t expect[] = {131,107,0,3,97,98,99};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    string t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(3ul, t1.size());
    eterm et(t1);
    std::string str( et.to_string() );
    if (!(et == t))
    BOOST_CHECK_EQUAL(et, t);
    if (str != "\"abc\"")
    BOOST_CHECK_EQUAL("\"abc\"", str);
}

BOOST_AUTO_TEST_CASE( test_encode_atom )
{
    eterm a(atom("abc"));
    string s(a.encode(0));
    const uint8_t expect[] = {131,119,0,3,97,98,99};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    atom t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(3ul, t1.size());
    BOOST_CHECK_EQUAL("abc", eterm(t1).to_string());

    const uint8_t expect2[] = {131,119,2,209,132};
    eterm a2(atom("abc"));
    string s2(a2.encode(0));
    BOOST_CHECK(s2.equal(expect2));
    idx = 1;  // skipping the magic byte
    atom t2((const char*)expect2, idx, sizeof(expect2));
    BOOST_CHECK_EQUAL(2ul, t2.size());
    BOOST_CHECK_EQUAL("ф", eterm(t2).to_string());
}

BOOST_AUTO_TEST_CASE( test_encode_binary )
{
    const char data[] = {1,2,3,4,5,6,7,8,9,10,11,12,13};
    eterm t(binary(data, sizeof(data)));
    string s(t.encode(0));
    const uint8_t expect[] = {131,109,0,0,0,13,1,2,3,4,5,6,7,8,9,10,11,12,13};
    BOOST_CHECK(s.equal(expect));
}

BOOST_AUTO_TEST_CASE( test_encode_double )
{
    double d = 12345.6789;
    eterm t(d);
    string s(t.encode(0));
    const uint8_t expect[] = {131,70,64,200,28,214,230,49,248,161};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    eterm t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(d, t1.to_double());
}

BOOST_AUTO_TEST_CASE( test_encode_emptylist )
{
    list l(0);
    BOOST_CHECK(l.initialized());
    eterm t(l);
    string s(t.encode(0));
    const uint8_t expect[] = {131,106};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    eterm t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(LIST, t1.type());
    BOOST_CHECK_EQUAL(0ul, t1.to_list().length());
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
    const uint8_t expect[] = {131,108,0,0,0,4,ERL_ATOM_UTF8_EXT,0,3,97,98,99,
                              107,0,2,101,102,97,1,107,0,2,103,104,106};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    list t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(4ul, t1.length());
    std::string str(eterm(t1).to_string());
    BOOST_CHECK_EQUAL("[abc,\"ef\",1,\"gh\"]", str);
}

BOOST_AUTO_TEST_CASE( test_encode_long )
{
    {
        long d = 123;
        eterm t(d);
        string s(t.encode(0));
        const uint8_t expect[] = {131,97,123};
        BOOST_CHECK(s.equal(expect));
        uintptr_t idx = 1;  // skipping the magic byte
        eterm t1((const char*)expect, idx, sizeof(expect));
        BOOST_CHECK_EQUAL(d, t1.to_long());
    }
    {
        long d = 12345;
        eterm t(d);
        string s(t.encode(0));
        const uint8_t expect[] = {131,98,0,0,48,57};
        BOOST_CHECK(s.equal(expect));
        uintptr_t idx = 1;  // skipping the magic byte
        eterm t1((const char*)expect, idx, sizeof(expect));
        BOOST_CHECK_EQUAL(d, t1.to_long());
    }
    {
#if EIXX_SIZEOF_LONG >= 8
        long d = 12345678901;
#else
        long d = 0x12345678;
#endif // EIXX_SIZEOF_LONG >= 8
        eterm t(d);
        string s(t.encode(0));
#if EIXX_SIZEOF_LONG >= 8
        const uint8_t expect[] = {131,110,5,0,53,28,220,223,2};
#else
        const uint8_t expect[] = {131,110,4,0,0x78,0x56,0x34,0x12};
#endif // EIXX_SIZEOF_LONG >= 8
        BOOST_CHECK(s.equal(expect));
        uintptr_t idx = 1;  // skipping the magic byte
        eterm t1((const char*)expect, idx, sizeof(expect));
        BOOST_CHECK_EQUAL(d, t1.to_long());
    }
}

BOOST_AUTO_TEST_CASE( test_encode_pid )
{
    {
        eterm t(epid("test@host", 1, 2, 0));
        BOOST_CHECK_EQUAL("#Pid<test@host.1.2>", t.to_string());
        string s(t.encode(0));
        //std::cout << s.to_binary_string() << std::endl;
        const uint8_t expect[] = 
            {131,88,118,0,9,116,101,115,116,64,104,111,115,116,0,0,0,1,0,0,0,2,0,0,0,0};
        BOOST_CHECK(s.equal(expect));
        uintptr_t idx = 1;  // skipping the magic byte
        eterm pid(epid((const char*)expect, idx, sizeof(expect)));
        BOOST_CHECK_EQUAL(eterm(pid), t);
    }
    {
        const uint8_t expect[] =
            {131,88,118,0,9,116,101,115,116,64,104,111,115,116,0,0,0,1,0,0,0,2,0,0,0,3};
        uintptr_t idx = 1;  // skipping the magic byte
        epid decode_pid((const char*)expect, idx, sizeof(expect));
        epid expect_pid("test@host", 1, 2, 3);
        BOOST_CHECK_EQUAL(expect_pid, decode_pid);
    }
}

BOOST_AUTO_TEST_CASE( test_encode_port )
{
    eterm t(port("test@host", 1, 0));
    BOOST_CHECK_EQUAL("#Port<test@host.1>", t.to_string());
    string s(t.encode(0));
    //std::cout << s.to_binary_string() << std::endl;
    const uint8_t expect[] =
        {131,102,ERL_ATOM_UTF8_EXT,0,9,116,101,115,116,64,104,111,115,116,0,0,0,1,0};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    eterm t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(t1, t);
}

BOOST_AUTO_TEST_CASE( test_encode_ref )
{
    uint32_t ids[] = {1,2,3};
    eterm t(ref("test@host", ids, 0));
    BOOST_CHECK_EQUAL("#Ref<test@host.1.2.3>", t.to_string());
    string s(t.encode(0));
    //std::cout << s.to_binary_string() << std::endl;
    const uint8_t expect[] =
        {131,90,0,3,100,0,9,116,101,115,116,64,104,111,115,116,0,0,0,0,0,0,0,1,0,0,0,2,0,0,0,3};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    ref t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(eterm(t1), t);
    {
        ref t2(atom("abc@fc12"), 993, 0, 0, 2);
        //std::cout << string(eterm(t).encode(0)).to_binary_string() << std::endl;
        const uint8_t expect2[] =
            {131,90,0,3,118,0,8,97,98,99,64,102,99,49,50,0,0,0,2,0,0,3,225,0,0,0,0,0,0,0,0};
        uintptr_t idx2 = 1;  // skipping the magic byte
        ref t3((const char*)expect2, idx2, sizeof(expect2));
        BOOST_CHECK_EQUAL(t2, t3);
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
    const uint8_t expect[] = {131,104,5,ERL_ATOM_UTF8_EXT,0,3,97,98,99,107,0,2,101,102,97,1,
                              104,4,ERL_ATOM_UTF8_EXT,0,1,97,107,0,2,120,120,70,64,94,198,102,
                              102,102,102,102,97,5,107,0,2,103,104};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    tuple t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(5ul, t1.size());
    BOOST_CHECK_EQUAL("{abc,\"ef\",1,{a,\"xx\",123.1,5},\"gh\"}", eterm(t1).to_string());
}

BOOST_AUTO_TEST_CASE( test_encode_trace )
{
    epid  self("abc@fc12",96,0,3);
    trace tr(1,2,3,self,4);
    eterm t(tr);
    string s(t.encode(0));
    //std::cout << to_binary_string(s) << std::endl;
    const uint8_t expect[] = 
        {131,104,5,97,1,97,2,97,3,88,118,0,8,97,98,99,64,102,99,49,50,0,0,0,96,0,0,0,0,0,0,0,3,97,4};
    BOOST_CHECK(s.equal(expect));
    uintptr_t idx = 1;  // skipping the magic byte
    trace t1((const char*)expect, idx, sizeof(expect));
    BOOST_CHECK_EQUAL(5ul, t1.size());
    BOOST_CHECK_EQUAL(1,   t1.flags());
    BOOST_CHECK_EQUAL(2,   t1.label());
    BOOST_CHECK_EQUAL(3,   t1.serial());
    BOOST_CHECK(self == t1.from());
    BOOST_CHECK_EQUAL(4,   t1.prev());
    BOOST_CHECK_EQUAL("{1,2,3,#Pid<abc@fc12.96.0,3>,4}", eterm(t1).to_string());
}

BOOST_AUTO_TEST_CASE( test_encode_rpc )
{
    static const unsigned char s_expected[] = {
        131,104,2,88,118,0,14,69,67,71,46,72,49,46,48,48,49,64,102,49,54,0,0,0,1,0,
        0,0,0,0,0,0,0,104,5,118,0,4,99,97,108,108,118,0,7,101,99,103,95,97,112,105,
        118,0,11,114,101,103,95,112,114,111,99,101,115,115,108,0,0,0,5,118,0,3,69,
        67,71,118,0,10,69,67,71,46,72,49,46,48,48,49,88,118,0,14,69,67,71,46,72,49,
        46,48,48,49,64,102,49,54,0,0,0,1,0,0,0,0,0,0,0,0,107,0,12,101,120,97,109,
        112,108,101,95,99,111,114,101,98,0,0,7,208,106,118,0,4,117,115,101,114
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
    BOOST_CHECK(l_sz < sizeof(l_buf));
    l_term.encode(l_buf, l_sz, 0, true);
    std::string s = std::string(l_buf, l_sz);
    std::string s_exp = std::string(l_buf, sizeof(s_expected));
    if (s != s_exp)
        std::cout << "String: " << to_binary_string(s.c_str(), s.size()) << std::endl;
    BOOST_CHECK_EQUAL(s_exp, s);
}

