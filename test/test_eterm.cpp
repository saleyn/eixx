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
#define BOOST_TEST_MODULE test_eterm

#include <boost/test/unit_test.hpp>
#include "test_alloc.hpp"
#include <eixx/eixx.hpp>
#include <set>

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_atomable )
{
	util::atom_table<> t(10);
	BOOST_REQUIRE_EQUAL(0, t.lookup(std::string()));
	BOOST_REQUIRE_EQUAL(0, t.lookup(""));
	int n = t.lookup("abc");
	BOOST_REQUIRE(0 < n);
	BOOST_REQUIRE(0 < t.lookup("aaaaa"));
	BOOST_REQUIRE_EQUAL(n, t.lookup("abc"));
}

BOOST_AUTO_TEST_CASE( test_atom )
{
    {
        atom a("");
        BOOST_REQUIRE_EQUAL(0, a.index());
        BOOST_REQUIRE_EQUAL(atom(), a);
    }
    {
        atom et1("Abc");
        BOOST_REQUIRE(et1.index() > 0);
        atom et2("aBc");
        BOOST_REQUIRE_NE(et1, et2);
        atom et3("Abc");
        BOOST_REQUIRE_EQUAL(et1, et3);
        BOOST_REQUIRE_EQUAL(et1.index(), et3.index());
    }

    {
        const uint8_t buf[] = {ERL_ATOM_EXT,0,3,97,98,99};
        int i = 0;
        atom atom((const char*)buf, i, sizeof(buf));
        BOOST_REQUIRE_EQUAL(6, i);
        BOOST_REQUIRE_EQUAL("abc", atom);
        BOOST_REQUIRE_EQUAL(std::string("abc"), atom.c_str());
        BOOST_REQUIRE_EQUAL(atom.c_str(), std::string("abc"));
        eterm et1(atom);
        BOOST_REQUIRE_EQUAL(std::string("abc"), et1.to_string());
        eterm et2(marshal::atom("Abc"));
        BOOST_REQUIRE_EQUAL("'Abc'", et2.to_string());

        BOOST_REQUIRE_EQUAL("a", et1.to_string(1));
    }
}

BOOST_AUTO_TEST_CASE( test_bool )
{
    allocator_t alloc;
    {
        eterm et(true);
        BOOST_REQUIRE(et.initialized());
        BOOST_REQUIRE_EQUAL(BOOL, et.type());
    }

    {
        const uint8_t buf[] = {ERL_ATOM_EXT,0,4,116,114,117,101};
        int i = 0;
        eterm t((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(true, t.to_bool());
        BOOST_REQUIRE_EQUAL(std::string("true"), t.to_string());
    }
    {
        const uint8_t buf[] = {ERL_ATOM_EXT,0,5,102,97,108,115,101};
        int i = 0;
        eterm t((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(sizeof(buf), (size_t)i);
        BOOST_REQUIRE_EQUAL(false, t.to_bool());
        BOOST_REQUIRE_EQUAL(std::string("false"), t.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_binary )
{
    allocator_t alloc;
    { binary et("Abc", 3, alloc); }
    { binary et("Abc", 3); }
    { binary et("Abc", 3, alloc); }
    {
        binary et{1,2,109};
        BOOST_REQUIRE_EQUAL(3u, et.size());
        BOOST_REQUIRE_EQUAL("<<1,2,109>>", eterm(et).to_string());
        BOOST_REQUIRE_EQUAL("<<>>", eterm(binary({}, alloc)).to_string());
    }

    {
        const uint8_t buf[] = {ERL_BINARY_EXT,0,0,0,3,97,98,99};
        int i = 0;
        binary term1((const char*)buf, i, sizeof(buf), alloc);
        i = 0;
        binary term2((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(term1, term2);
        eterm et(term1);
        BOOST_REQUIRE_EQUAL("<<\"abc\">>", et.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_list )
{
    allocator_t alloc;
    { list et; }
    { list et(10); }
    { list et(10, alloc); }
    {
        eterm l[] = { 
            eterm(atom("abc")),
            eterm(atom("efg"))
        };
        eterm et = list(l);
        BOOST_REQUIRE(et.initialized());
    }
    {
        eterm items[] = { 
            eterm(atom("abc")),
            eterm(atom("efg"))
        };
        list l(2, alloc);
        l.push_back(items[0]);
        l.push_back(items[1]);
        BOOST_REQUIRE(!l.initialized());
        l.close();
        BOOST_REQUIRE(l.initialized());
        BOOST_REQUIRE_EQUAL(2u, l.length());
        eterm et(l);
        BOOST_REQUIRE_EQUAL(2u, et.to_list().length());
    }
    {
        eterm items[] = { eterm(atom("abc")), eterm(atom("efg")) };
        list l(items, alloc);
        BOOST_REQUIRE(l.initialized());
        BOOST_REQUIRE_EQUAL(2u, l.length());
        list::iterator it = l.begin();
        BOOST_REQUIRE_EQUAL("efg", (++it)->to_string());
        eterm et(l);
        BOOST_REQUIRE_EQUAL("[abc,efg]", et.to_string());
    }
    {
        eterm items[] = { 
            eterm(1),
            eterm(2),
            eterm(3)
        };
        list et(items, alloc);
        BOOST_REQUIRE_EQUAL(3u, et.length());

        const list& cp1 = et.tail(0);
        BOOST_REQUIRE_EQUAL(2u, cp1.length());
        list::const_iterator it = cp1.begin();
        BOOST_REQUIRE_EQUAL(LONG, it->type());
        BOOST_REQUIRE_EQUAL(2, (it++)->to_long());
        BOOST_REQUIRE_EQUAL(LONG, it->type());
        BOOST_REQUIRE_EQUAL(3, (it++)->to_long());
        BOOST_REQUIRE(cp1.end() == it);
    }
}

BOOST_AUTO_TEST_CASE( test_list3 )
{
    allocator_t alloc;
    {
        list t = list::make(1, alloc);
        BOOST_REQUIRE_EQUAL(1ul, t.length());
        BOOST_REQUIRE_EQUAL(1,   t.nth(0).to_long());
    }
    {
        const list& t = list::make(1, 2, alloc);
        BOOST_REQUIRE_EQUAL(2ul, t.length());
        BOOST_REQUIRE_EQUAL(1,   t.nth(0).to_long());
        BOOST_REQUIRE_EQUAL(2,   t.nth(1).to_long());
    }
    {
        const list& t = list::make(1,2,3, alloc);
        BOOST_REQUIRE_EQUAL(3ul, t.length());
        BOOST_REQUIRE_EQUAL(1,   t.nth(0).to_long());
        BOOST_REQUIRE_EQUAL(2,   t.nth(1).to_long());
        BOOST_REQUIRE_EQUAL(3,   t.nth(2).to_long());
    }
    {
        const list& t = list::make(1,2,3,4, alloc);
        BOOST_REQUIRE_EQUAL(4ul, t.length());
        BOOST_REQUIRE_EQUAL(1,   t.nth(0).to_long());
        BOOST_REQUIRE_EQUAL(2,   t.nth(1).to_long());
        BOOST_REQUIRE_EQUAL(3,   t.nth(2).to_long());
        BOOST_REQUIRE_EQUAL(4,   t.nth(3).to_long());
    }
    {
        const list& t = list::make(1,2,3,4,5, alloc);
        BOOST_REQUIRE_EQUAL(5ul, t.length());
        BOOST_REQUIRE_EQUAL(1,   t.nth(0).to_long());
        BOOST_REQUIRE_EQUAL(2,   t.nth(1).to_long());
        BOOST_REQUIRE_EQUAL(3,   t.nth(2).to_long());
        BOOST_REQUIRE_EQUAL(4,   t.nth(3).to_long());
        BOOST_REQUIRE_EQUAL(5,   t.nth(4).to_long());
    }
    {
        const list& t = list::make(1,2,3,4,5,6, alloc);
        BOOST_REQUIRE_EQUAL(6ul, t.length());
        BOOST_REQUIRE_EQUAL(1,   t.nth(0).to_long());
        BOOST_REQUIRE_EQUAL(2,   t.nth(1).to_long());
        BOOST_REQUIRE_EQUAL(3,   t.nth(2).to_long());
        BOOST_REQUIRE_EQUAL(4,   t.nth(3).to_long());
        BOOST_REQUIRE_EQUAL(5,   t.nth(4).to_long());
        BOOST_REQUIRE_EQUAL(6,   t.nth(5).to_long());
    }
}

BOOST_AUTO_TEST_CASE( test_list4 )
{
    list l;
    for (int i=0; i<2; ++i)
        l.push_back(eterm(atom("abc")));
    l.close();
    BOOST_REQUIRE_EQUAL(2u, l.length());
}

BOOST_AUTO_TEST_CASE( test_double )
{
    allocator_t alloc;
    {
        eterm et1(10.0);
        BOOST_REQUIRE_EQUAL(DOUBLE, et1.type());
        BOOST_REQUIRE(et1.initialized());
    }

    {
        const uint8_t buf[] = {ERL_FLOAT_EXT,49,46,48,48,48,48,48,48,48,
                               48,48,48,48,48,48,48,48,48,48,48,48,48,101,
                               43,48,48,0,0,0,0,0};
        int i = 0;
        eterm term((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(32, i);
        BOOST_REQUIRE_EQUAL(1.0, term.to_double());
    }
    {
        const uint8_t buf[] = {NEW_FLOAT_EXT,63,240,0,0,0,0,0,0};
        int i = 0;
        eterm term((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(9, i);
        BOOST_REQUIRE_EQUAL(1.0, term.to_double());
        BOOST_REQUIRE_EQUAL("1.0", term.to_string());
    }
    {
        eterm term(90.0);
        BOOST_REQUIRE_EQUAL("90.0", term.to_string());
    }
    {
        eterm term(900.0);
        BOOST_REQUIRE_EQUAL("900.0", term.to_string());
    }
    {
        eterm term(90.010000);
        BOOST_REQUIRE_EQUAL("90.01", term.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_long )
{
    allocator_t alloc;
    {
        eterm et(100l * 1024 * 1024 * 1024);
        BOOST_REQUIRE_EQUAL(LONG, et.type());
        BOOST_REQUIRE_EQUAL(100l * 1024 * 1024 * 1024, et.to_long());
    }
    {
        eterm et(1); 
        BOOST_REQUIRE(et.initialized());
    }
    {
        const uint8_t buf[] = {ERL_INTEGER_EXT,7,91,205,21};
        int i = 0;
        eterm term((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(5, i);
        BOOST_REQUIRE_EQUAL (123456789,  term.to_long());
        BOOST_REQUIRE_EQUAL("123456789", term.to_string());
    }
    {
        const uint8_t buf[] = {ERL_SMALL_BIG_EXT,4,1,210,2,150,73};
        int i = 0;
        eterm term((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(7, i);
        BOOST_REQUIRE_EQUAL (-1234567890,  term.to_long());
        BOOST_REQUIRE_EQUAL("-1234567890", term.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_string )
{
    allocator_t alloc;
    {
        eterm et("Abc", alloc);
        BOOST_REQUIRE(et.initialized());
        BOOST_REQUIRE_EQUAL(STRING, et.type());
    }

    {
        string s("a", alloc);
        std::string test("abcd");
        s = test;
        BOOST_REQUIRE_EQUAL(s, "abcd");
    }

    {
        const uint8_t buf[] = {ERL_STRING_EXT,0,3,97,98,99};
        int i = 0;
        eterm term((const char*)buf, i, sizeof(buf), alloc);
        BOOST_REQUIRE_EQUAL(6, i);
        BOOST_REQUIRE_EQUAL("abc", term.to_str());
        BOOST_REQUIRE_EQUAL(term.to_str(), "abc");
        BOOST_REQUIRE_EQUAL(std::string("abc"), term.to_str());
        BOOST_REQUIRE_EQUAL(term.to_str(), std::string("abc"));
        BOOST_REQUIRE_EQUAL(std::string("\"abc\""), term.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_pid )
{
    allocator_t alloc;
    {
        epid et("abc@fc12", 1, 2, 3, alloc);
        BOOST_REQUIRE_EQUAL(atom("abc@fc12"), et.node());
        BOOST_REQUIRE_EQUAL(1, et.id());
        BOOST_REQUIRE_EQUAL(2, et.serial());
        BOOST_REQUIRE_EQUAL(3, et.creation());

        et = epid("abc@fc12", 1, 2, 4, alloc);
        BOOST_REQUIRE_EQUAL(0, et.creation());

        eterm t(et);
        BOOST_REQUIRE(t.initialized());
        BOOST_REQUIRE_EQUAL(PID, t.type());
        BOOST_REQUIRE_EQUAL("#Pid<abc@fc12.1.2.0>", t.to_string());
    }

    {
        epid p1("a@fc12", 1, 2, 3, alloc);
        epid p2("a@fc12", 1, 2, 3, alloc);
        BOOST_REQUIRE_EQUAL(p1, p2);
        epid p3("a@fc", 1, 2, 3, alloc);
        BOOST_REQUIRE_NE(p1, p3);
        epid p4("a@fc12", 4, 2, 3, alloc);
        BOOST_REQUIRE_NE(p1, p4);
        epid p5("a@fc12", 1, 4, 3, alloc);
        BOOST_REQUIRE_NE(p1, p5);
        epid p6("a@fc12", 1, 2, 4, alloc);
        BOOST_REQUIRE_NE(p1, p6);
    }
}

BOOST_AUTO_TEST_CASE( test_less_then )
{
    {
        std::set<atom>();
        std::set<binary>();
        std::set<list>();
        std::set<epid>();
        std::set<port>();
        std::set<ref>();
        std::set<string>();
        std::set<trace>();
        std::set<tuple>();
    }

    {
        allocator_t alloc;
        std::set<epid> ss;

        epid et1("abc@fc12", 1, 2, 3, alloc);
        epid et2("abc@fc12", 1, 4, 3, alloc);

        ss.insert(et1);
        ss.insert(et2);
        ss.insert(et1);
        BOOST_REQUIRE_EQUAL(2ul, ss.size());
    }
}

BOOST_AUTO_TEST_CASE( test_port )
{
    allocator_t alloc;
    {
        port et("abc@fc12", 1, 2, alloc);
        BOOST_REQUIRE_EQUAL(atom("abc@fc12"), et.node());
        BOOST_REQUIRE_EQUAL(1, et.id());
        BOOST_REQUIRE_EQUAL(2, et.creation());
        eterm t(et);
        BOOST_REQUIRE(t.initialized());
        BOOST_REQUIRE_EQUAL(PORT, t.type());
        BOOST_REQUIRE_EQUAL("#Port<abc@fc12.1>", t.to_string());
    }

    {
        port p1("a@fc12", 1, 2, alloc);
        port p2("a@fc12", 1, 2);
        BOOST_REQUIRE_EQUAL(p1, p2);
        port p3("a@fc", 1, 2, alloc);
        BOOST_REQUIRE_NE(p1, p3);
        port p4("a@fc12", 4, 2, alloc);
        BOOST_REQUIRE_NE(p1, p4);
        port p5("a@fc12", 1, 4, alloc);
        BOOST_REQUIRE_NE(p1, p5);
    }
}

BOOST_AUTO_TEST_CASE( test_ref )
{
    allocator_t alloc;
    {
        uint32_t ids[] = {5,6,7};
        ref et("abc@fc12", ids, 3, alloc);
        BOOST_REQUIRE_EQUAL(atom("abc@fc12"), et.node());
        BOOST_REQUIRE_EQUAL(5u, et.id(0));
        BOOST_REQUIRE_EQUAL(6u, et.id(1));
        BOOST_REQUIRE_EQUAL(7u, et.id(2));
        BOOST_REQUIRE_EQUAL(3, et.creation());

        et = ref("abc@fc12", ids, 4, alloc);
        BOOST_REQUIRE_EQUAL(0, et.creation());

        eterm t(et);
        BOOST_REQUIRE(t.initialized());
        BOOST_REQUIRE_EQUAL(REF, t.type());
        BOOST_REQUIRE_EQUAL("#Ref<abc@fc12.5.6.7>", t.to_string());
    }

    {
        uint32_t ids[] = {1,2,3};
        ref p1("abc@fc12", ids, 4, alloc);
        ref p2("abc@fc12", ids, 4);
        BOOST_REQUIRE_EQUAL(p1, p2);
        ids[0] = 4;
        ref p3("abc@fc12", ids, 4);
        BOOST_REQUIRE_NE(p1, p3);
        ids[0] = 1;
        ids[1] = 4;
        ref p4("abc@fc12", ids, 4);
        BOOST_REQUIRE_NE(p1, p4);
        ids[1] = 2;
        ids[2] = 4;
        ref p5("abc@fc12", ids, 4);
        BOOST_REQUIRE_NE(p1, p5);
        ids[2] = 3;
        ref p6("abc@fc12", ids, 4);
        BOOST_REQUIRE_EQUAL(p1, p6);
        ref p7("abc@fc12", ids, 5);
        BOOST_REQUIRE_NE(p1, p7);
    }
}

BOOST_AUTO_TEST_CASE( test_tuple )
{
    allocator_t alloc;
    {
        tuple et2(10, alloc);
        BOOST_REQUIRE(!et2.initialized());
    }
    {
        eterm l[] = { 
            eterm(atom("abc")),
            eterm(atom("efg"))
        };
        eterm et = tuple(l);
        BOOST_REQUIRE(et.initialized());
    }

    eterm l[] = { 
        eterm(atom("abc")),
        eterm(atom("efg")),
        eterm(atom("eee")),
        eterm(atom("fff"))
    };

    tuple et(sizeof(l)/sizeof(eterm), alloc);

    for (size_t i=0; i < sizeof(l)/sizeof(eterm); i++)
        et.push_back(l[i]);

    // Note that now the tuple owns previously dangling eterm pointers
    // in the list[] array.
    BOOST_REQUIRE(et.initialized());
    BOOST_REQUIRE_EQUAL(4u, et.size());
    BOOST_REQUIRE_EQUAL("efg", et[1].to_string());
}

BOOST_AUTO_TEST_CASE( test_tuple2 )
{
    allocator_t alloc;
    for (int i=0; i < 3; i++) {
        eterm items[] = { eterm(atom("Abc")), eterm(atom("efg")) };
        tuple et(2, alloc);
        et.push_back(items[0]);
        et.push_back(items[1]);
        BOOST_REQUIRE(et.initialized());
        BOOST_REQUIRE_EQUAL(2u, et.size());
        BOOST_REQUIRE_EQUAL("efg", et[1].to_string());
        eterm term(et);
        BOOST_REQUIRE_EQUAL("{'Abc',efg}", term.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_tuple3 )
{
    allocator_t alloc;
    {
        tuple t = tuple::make(1, alloc);
        BOOST_REQUIRE_EQUAL(1ul, t.size());
        BOOST_REQUIRE_EQUAL(1,   t[0].to_long());
    }
    {
        const tuple& t = tuple::make(1, 2, alloc);
        BOOST_REQUIRE_EQUAL(2ul, t.size());
        BOOST_REQUIRE_EQUAL(1,   t[0].to_long());
        BOOST_REQUIRE_EQUAL(2,   t[1].to_long());
    }
    {
        const tuple& t = tuple::make(1,2,3, alloc);
        BOOST_REQUIRE_EQUAL(3ul, t.size());
        BOOST_REQUIRE_EQUAL(1,   t[0].to_long());
        BOOST_REQUIRE_EQUAL(2,   t[1].to_long());
        BOOST_REQUIRE_EQUAL(3,   t[2].to_long());
    }
    {
        const tuple& t = tuple::make(1,2,3,4, alloc);
        BOOST_REQUIRE_EQUAL(4ul, t.size());
        BOOST_REQUIRE_EQUAL(1,   t[0].to_long());
        BOOST_REQUIRE_EQUAL(2,   t[1].to_long());
        BOOST_REQUIRE_EQUAL(3,   t[2].to_long());
        BOOST_REQUIRE_EQUAL(4,   t[3].to_long());
    }
    {
        const tuple& t = tuple::make(1,2,3,4,5, alloc);
        BOOST_REQUIRE_EQUAL(5ul, t.size());
        BOOST_REQUIRE_EQUAL(1,   t[0].to_long());
        BOOST_REQUIRE_EQUAL(2,   t[1].to_long());
        BOOST_REQUIRE_EQUAL(3,   t[2].to_long());
        BOOST_REQUIRE_EQUAL(4,   t[3].to_long());
        BOOST_REQUIRE_EQUAL(5,   t[4].to_long());
    }
    {
        const tuple& t = tuple::make(1,2,3,4,5,6, alloc);
        BOOST_REQUIRE_EQUAL(6ul, t.size());
        BOOST_REQUIRE_EQUAL(1,   t[0].to_long());
        BOOST_REQUIRE_EQUAL(2,   t[1].to_long());
        BOOST_REQUIRE_EQUAL(3,   t[2].to_long());
        BOOST_REQUIRE_EQUAL(4,   t[3].to_long());
        BOOST_REQUIRE_EQUAL(5,   t[4].to_long());
        BOOST_REQUIRE_EQUAL(6,   t[5].to_long());
    }
}

BOOST_AUTO_TEST_CASE( test_trace )
{
    allocator_t alloc;
    {
        trace tr1(1, 2, 3, epid("a@host",5,1,0,alloc), 4, alloc);
        eterm et1(tr1);
        trace tr2(1, 6, 3, epid("a@host",5,1,0,alloc), 4, alloc);
        eterm et2(tr2);
        trace tr3(1, 2, 6, epid("a@host",5,1,0,alloc), 4, alloc);
        eterm et3(tr3);
        trace tr4(1, 2, 3, epid("a@host",6,1,0,alloc), 4, alloc);
        eterm et4(tr4);
        trace tr5(1, 2, 3, epid("a@host",5,1,0,alloc), 6, alloc);
        eterm et5(tr5);
        BOOST_REQUIRE(et1.initialized());
        BOOST_REQUIRE_EQUAL(TRACE, et1.type());
        BOOST_REQUIRE(et1 == et1);
        BOOST_REQUIRE(et1 != et2);
        BOOST_REQUIRE(et1 != et3);
        BOOST_REQUIRE(et1 != et4);
        BOOST_REQUIRE(et1 != et5);
        BOOST_REQUIRE_EQUAL("{1,2,3,#Pid<a@host.5.1.0>,4}", et1.to_string());
    }
}

BOOST_AUTO_TEST_CASE( test_varbind )
{
    allocator_t alloc;

    varbind binding1;
    binding1.bind(atom("Name"),  20.0);
    binding1.bind(atom("Long"),  123);
    varbind binding2;
    binding2.bind(atom("Name"),  atom("test"));
    binding2.bind(atom("Other"), "vasya");

    binding1.merge(binding2);

    BOOST_REQUIRE_EQUAL(3ul, binding1.count());
}

eterm f() {
    eterm a("abcd");
    return a;
}

BOOST_AUTO_TEST_CASE( test_assign )
{
    {
        eterm a = f();
        BOOST_REQUIRE_EQUAL(STRING, a.type());
        BOOST_REQUIRE_EQUAL("abcd", a.to_str());
    }

    eterm a;
    {
        a.set( f() );
        BOOST_REQUIRE_EQUAL(STRING, a.type());
        BOOST_REQUIRE_EQUAL("abcd", a.to_str());
    }
    {
        a = f();
        BOOST_REQUIRE_EQUAL(STRING, a.type());
        BOOST_REQUIRE_EQUAL("abcd", a.to_str());
    }
    {
        eterm b("abcd");
        eterm c = b;
        BOOST_REQUIRE_EQUAL(STRING, c.type());
        BOOST_REQUIRE_EQUAL("abcd", c.to_str());
        c = "ddd";
        BOOST_REQUIRE_EQUAL(STRING, c.type());
        BOOST_REQUIRE_EQUAL("ddd", c.to_str());
        c.set( f() );
        BOOST_REQUIRE_EQUAL(STRING, c.type());
        BOOST_REQUIRE_EQUAL("abcd", c.to_str());
    }
}

BOOST_AUTO_TEST_CASE( test_cast )
{
    {
        allocator_t alloc;

        eterm items[] = { eterm(true) };

        {
            list l(items, alloc);
        }

        eterm ll[] = { 
            eterm(list(items)),
            eterm(tuple(items)),
            eterm(atom("test")),
            eterm(123),
            eterm(1.0),
            eterm(true),
            eterm("ABC")
        };

        const list&  l = ll[0].to_list();
        const tuple& t = ll[1].to_tuple();

        BOOST_REQUIRE_EQUAL(true, t[0].to_bool());
        BOOST_REQUIRE_EQUAL(true, l.begin()->to_bool());

        tuple et(ll, alloc);
        BOOST_REQUIRE_EQUAL(sizeof(ll) / sizeof(eterm), et.size());

        BOOST_REQUIRE_EQUAL(1u,     ll[0].to_list().length());
        BOOST_REQUIRE_EQUAL(1u,     ll[1].to_tuple().size());
        BOOST_REQUIRE_EQUAL("test", ll[2].to_atom());
        BOOST_REQUIRE_EQUAL(123,    ll[3].to_long());
        BOOST_REQUIRE_EQUAL(1.0,    ll[4].to_double());
        BOOST_REQUIRE_EQUAL(true,   ll[5].to_bool());
        BOOST_REQUIRE_EQUAL("ABC",  ll[6].to_str());
    }
}

BOOST_AUTO_TEST_CASE( test_cast2 )
{
    allocator_t alloc;
    { eterm t( eterm::cast(1)   ); BOOST_REQUIRE_EQUAL(LONG,   t.type()); }
    { eterm t( eterm::cast(1.0) ); BOOST_REQUIRE_EQUAL(DOUBLE, t.type()); }
    { eterm t( eterm::cast(true)); BOOST_REQUIRE_EQUAL(BOOL,   t.type()); }
    { eterm t( eterm::cast("ab")); BOOST_REQUIRE_EQUAL(STRING, t.type()); }
}


