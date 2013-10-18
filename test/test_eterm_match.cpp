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
#include <functional>
#include "test_alloc.hpp"
#include <eixx/eixx.hpp>

using namespace EIXX_NAMESPACE;

BOOST_AUTO_TEST_CASE( test_match1 )
{
    allocator_t alloc;

    // Populate the inner list with: [1,2]
    list l(2, alloc);
    l.push_back(1);
    l.push_back(2);
    l.close();

    // Create the outer list of terms: [test, 123, []]
    eterm ol[] = { 
        eterm(atom("test")),
        eterm(123),
        eterm(l)
    };

    {
        // Create the tuple from list: {test, 123, [1,2]}
        tuple tfl(ol);
        eterm tup(tfl);

        list ll(2, alloc);
        ll.push_back(eterm(1));
        ll.push_back(eterm(var(LONG)));
        ll.close();

        // Add pattern = {test, _::int(), [1, _::int()]}
        tuple ptup(3, alloc);
        ptup.push_back(eterm(atom("test")));
        ptup.push_back(var(LONG));
        ptup.push_back(ll);

        eterm pattern(ptup);
        // Perform pattern match on the tuple
        bool res = tup.match(pattern);
        BOOST_REQUIRE(res);

        varbind vb;
        res = tup.match(pattern, &vb);
        BOOST_REQUIRE(res);

        BOOST_REQUIRE_EQUAL(0u, vb.count());
    }
}

namespace {
    struct cb_t {
        int match[10];
        cb_t() { bzero(match, sizeof(match)); }

        bool operator() (const eterm& a_pattern,
                         const varbind& a_varbind,
                         long a_opaque)
        {
            const eterm* n_var = a_varbind.find("N");
            if (n_var && n_var->type() == LONG) {
                int n = n_var->to_long();
                switch (a_opaque) {
                    case 1:
                        match[0]++;
                        BOOST_REQUIRE_EQUAL(1, n);
                        BOOST_REQUIRE(a_varbind.find("A") != NULL);
                        break;
                    case 2:
                        match[1]++;
                        BOOST_REQUIRE_EQUAL(2, n);
                        BOOST_REQUIRE(a_varbind.find("B") != NULL);
                        break;
                    case 3:
                        match[2]++;
                        BOOST_REQUIRE_EQUAL(3, n);
                        BOOST_REQUIRE(a_varbind.find("Reason") != NULL);
                        BOOST_REQUIRE_EQUAL(ATOM, a_varbind.find("Reason")->type());
                        break;
                    case 4:
                        match[3]++;
                        BOOST_REQUIRE_EQUAL(4, n);
                        BOOST_REQUIRE(a_varbind.find("X") != NULL);
                        BOOST_REQUIRE_EQUAL(TUPLE, a_varbind.find("X")->type());
                        break;
                    default:
                        throw "Invalid opaque value!";
                }
                return true;
            }
            return true;
        }
    };
}

BOOST_AUTO_TEST_CASE( test_match2 )
{
    allocator_t alloc;

    eterm_pattern_matcher etm;
    cb_t cb;

    // Add some patterns to check 
    // Capital literals represent variable names that will be bound 
    // if a match is successful. When some pattern match succeeds
    // cb_t::operator() is called with the second parameter containing
    // a varbind map of bound variables with their values.
    // Each pattern contains a variable N that will be bound with
    // the "pattern number" in each successful match.
    etm.push_back(eterm::format("{test, N, A}"), 
        std::bind(&cb_t::operator(), &cb,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 1);
    etm.push_back(eterm::format("{ok, N, B, _}"),
        std::bind(&cb_t::operator(), &cb,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 2);
    etm.push_back(eterm::format("{error, N, Reason}"),
        std::bind(&cb_t::operator(), &cb,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 3);
    // Remember the reference to last pattern. We'll try to delete it later.
    const eterm_pattern_action& action =
        etm.push_back(eterm::format("{xxx, [_, _, {c, N}], \"abc\", X}"),
            std::bind(&cb_t::operator(), &cb,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), 4);

    // Make sure we registered 4 pattern above.
    BOOST_REQUIRE_EQUAL(4u, etm.size());

    static const eterm t0 = atom("test");
    eterm f0 = eterm::format("test");
    bool  m0 = t0.match(f0);
    BOOST_REQUIRE(m0);  // N = 1

    // Match the following terms against registered patters.
    auto f1 = eterm::format("{test, 1, 123}");
    auto m1 = etm.match(f1);
    BOOST_REQUIRE(m1);  // N = 1
    BOOST_REQUIRE(etm.match(eterm::format("{test, 1, 234}")));  // N = 1

    BOOST_REQUIRE(etm.match(eterm::format("{ok, 2, 3, 4}")));   // N = 2
    BOOST_REQUIRE(!etm.match(eterm::format("{ok, 2}")));

    BOOST_REQUIRE(etm.match(eterm::format("{error, 3, not_found}"))); // N = 3

    BOOST_REQUIRE(etm.match(
        eterm::format("{xxx, [{a, 1}, {b, 2}, {c, 4}], \"abc\", {5,6,7}}"))); // N = 4
    BOOST_REQUIRE(!etm.match(
        eterm::format("{xxx, [1, 2, 3, {c, 4}], \"abc\", 5}")));

    // Verify number of successful matches for each pattern.
    BOOST_REQUIRE_EQUAL(2, cb.match[0]);
    BOOST_REQUIRE_EQUAL(1, cb.match[1]);
    BOOST_REQUIRE_EQUAL(1, cb.match[2]);
    BOOST_REQUIRE_EQUAL(1, cb.match[3]);

    // Make sure we registered 4 pattern above.
    BOOST_REQUIRE_EQUAL(4u, etm.size());

    // Test pattern deletion.
    etm.erase(action);
    // Make sure we registered 4 pattern above.
    BOOST_REQUIRE_EQUAL(3u, etm.size());
}

BOOST_AUTO_TEST_CASE( test_match3 )
{
    {
        auto p = eterm::format("{ok, N, A}");
        auto e = eterm::format("{ok, 1, 2}");
        varbind b;
        auto m = e.match(p, &b);
        BOOST_REQUIRE(m);
    }

    {
        auto e = eterm::format("{snap, x12, []}");
        auto p = eterm::format("{snap, N, L}");
        varbind binding;
        BOOST_REQUIRE(p.match(e, &binding));
        const eterm* n = binding.find("N");
        const eterm* l = binding.find("L");
        BOOST_REQUIRE(n);
        BOOST_REQUIRE(l);
        BOOST_REQUIRE_EQUAL(ATOM,    n->type());
        BOOST_REQUIRE_EQUAL(LIST,    l->type());
        BOOST_REQUIRE_EQUAL(atom("x12"), n->to_atom());
        BOOST_REQUIRE_EQUAL(list(),  l->to_list());
    }

    {
        auto e = eterm::format("{1, 8#16, $a, 'Xbc', [{x, 2.0}]}");
        auto p = eterm::format("{A::int(), B::int(), C::char(), Q::atom(), D::list()}");
        varbind b;
        bool m = e.match(p, &b);
        BOOST_REQUIRE(m);
        auto va = b.find("A");
        auto vb = b.find("B");
        auto vc = b.find("C");
        auto vd = b.find("D");
        auto vq = b.find("Q");
        BOOST_REQUIRE(va);
        BOOST_REQUIRE(vb);
        BOOST_REQUIRE(vc);
        BOOST_REQUIRE(vd);
        BOOST_REQUIRE_EQUAL(LONG, va->type());
        BOOST_REQUIRE_EQUAL(1,    va->to_long());
        BOOST_REQUIRE_EQUAL(LONG, vb->type());
        BOOST_REQUIRE_EQUAL(14,   vb->to_long());
        BOOST_REQUIRE_EQUAL(LONG, vc->type());
        BOOST_REQUIRE_EQUAL('a',  vc->to_long());
        BOOST_REQUIRE_EQUAL(ATOM, vq->type());
        BOOST_REQUIRE_EQUAL(atom("Xbc"), vq->to_atom());
        BOOST_REQUIRE_EQUAL(LIST, vd->type());
        BOOST_REQUIRE_EQUAL(1u,    vd->to_list().length());
        m = eterm::format("[{x, 2.0}]").match(*vd);
        BOOST_REQUIRE(m);
    }

    {
        auto t   = eterm::format("[1,a,$b,\"xyz\",{1,10.0},[]]");
        auto pat = eterm::format("[A,B,C,D,E,F]");
        varbind p;
        BOOST_REQUIRE(pat.match(t, &p));
        auto a = p.find("A");
        auto b = p.find("B");
        auto c = p.find("C");
        auto f = p.find("F");
        auto d = p.find("D");
        auto e = p.find("E");
        BOOST_REQUIRE(a);
        BOOST_REQUIRE(b);
        BOOST_REQUIRE(c);
        BOOST_REQUIRE(d);
        BOOST_REQUIRE(e);
        BOOST_REQUIRE(f);
        BOOST_REQUIRE_EQUAL(eterm(1),  a->to_long());
        BOOST_REQUIRE_EQUAL(atom("a"), b->to_atom());
        BOOST_REQUIRE_EQUAL(eterm('b'), *c);
        BOOST_REQUIRE_EQUAL(string("xyz"), d->to_str());
        BOOST_REQUIRE_EQUAL(eterm::format("{1,10.0}"), *e);
        BOOST_REQUIRE_EQUAL(list(), f->to_list());
    }
}

BOOST_AUTO_TEST_CASE( test_initializer_list )
{
    const atom am_abc("abc");

    {
        eterm t{1, 20.0, am_abc, "xxx"};
        BOOST_REQUIRE_EQUAL(TUPLE,  t.type());
        BOOST_REQUIRE_EQUAL(4u,     t.to_tuple().size());
        BOOST_REQUIRE_EQUAL(1,      t.to_tuple()[0].to_long());
        BOOST_REQUIRE_EQUAL(20.0,   t.to_tuple()[1].to_double());
        BOOST_REQUIRE_EQUAL(am_abc, t.to_tuple()[2].to_atom());
        BOOST_REQUIRE_EQUAL("xxx",  t.to_tuple()[3].to_str());

        t = {eterm(1), eterm(20.0), eterm(atom("abc")), eterm("xxx")};
        BOOST_REQUIRE_EQUAL(TUPLE,  t.type());
        BOOST_REQUIRE_EQUAL(4u,     t.to_tuple().size());
        BOOST_REQUIRE_EQUAL(1,      t.to_tuple()[0].to_long());
        BOOST_REQUIRE_EQUAL(20.0,   t.to_tuple()[1].to_double());
        BOOST_REQUIRE_EQUAL(am_abc, t.to_tuple()[2].to_atom());
        BOOST_REQUIRE_EQUAL("xxx",  t.to_tuple()[3].to_str());

        t = {1, 20.0, am_abc, "xxx", {12, am_abc}};
        BOOST_REQUIRE_EQUAL(TUPLE,  t.type());
        BOOST_REQUIRE_EQUAL(5u,     t.to_tuple().size());
        BOOST_REQUIRE_EQUAL(1,      t.to_tuple()[0].to_long());
        BOOST_REQUIRE_EQUAL(20.0,   t.to_tuple()[1].to_double());
        BOOST_REQUIRE_EQUAL(am_abc, t.to_tuple()[2].to_atom());
        BOOST_REQUIRE_EQUAL(2u,     t.to_tuple()[4].to_tuple().size());
        BOOST_REQUIRE_EQUAL(12,     t.to_tuple()[4].to_tuple()[0].to_long());
        BOOST_REQUIRE_EQUAL(am_abc, t.to_tuple()[4].to_tuple()[1].to_atom());
    }

    {
        eterm t = list{1, 20.0, am_abc, "xxx"};
        BOOST_REQUIRE_EQUAL(LIST,  t.type());
        BOOST_REQUIRE_EQUAL(4u,     t.to_list().length());
        auto it = t.to_list().begin();
        BOOST_REQUIRE_EQUAL(1,      it->to_long());
        BOOST_REQUIRE_EQUAL(20.0,   (++it)->to_double());
        BOOST_REQUIRE_EQUAL(am_abc, (++it)->to_atom());
        BOOST_REQUIRE_EQUAL("xxx",  (++it)->to_str());

        t = list{eterm(1), eterm(20.0), eterm(atom("abc")), eterm("xxx")};
        it = t.to_list().begin();
        BOOST_REQUIRE_EQUAL(LIST,   t.type());
        BOOST_REQUIRE_EQUAL(4u,     t.to_list().length());
        BOOST_REQUIRE_EQUAL(1,      it->to_long());
        BOOST_REQUIRE_EQUAL(20.0,   (++it)->to_double());
        BOOST_REQUIRE_EQUAL(am_abc, (++it)->to_atom());
        BOOST_REQUIRE_EQUAL("xxx",  (++it)->to_str());

        t = list{1, 20.0, am_abc, "xxx", {12, am_abc}, list{3,4}};
        it = t.to_list().begin();
        BOOST_REQUIRE_EQUAL(LIST,   t.type());
        BOOST_REQUIRE_EQUAL(6u,     t.to_list().length());
        BOOST_REQUIRE_EQUAL(1,      it->to_long());
        BOOST_REQUIRE_EQUAL(20.0,   (++it)->to_double());
        BOOST_REQUIRE_EQUAL(am_abc, (++it)->to_atom());
        BOOST_REQUIRE_EQUAL("xxx",  (++it)->to_str());
        BOOST_REQUIRE_EQUAL(TUPLE,  (++it)->type());
        auto tt = it->to_tuple();
        BOOST_REQUIRE_EQUAL(2u,     tt.size());
        BOOST_REQUIRE_EQUAL(12,     tt[0].to_long());
        BOOST_REQUIRE_EQUAL(am_abc, tt[1].to_atom());
        BOOST_REQUIRE_EQUAL(LIST,   (++it)->type());
        auto l = it->to_list().begin();
        BOOST_REQUIRE_EQUAL(2u,     it->to_list().length());
        BOOST_REQUIRE_EQUAL(3,      (l++)->to_long());
        BOOST_REQUIRE_EQUAL(4,      (l++)->to_long());
    }
}

static void run(int n)
{
    static const int iterations = ::getenv("ITERATIONS") ? atoi(::getenv("ITERATIONS")) : 1;
    static cb_t cb;
    static allocator_t alloc;
    static eterm_pattern_matcher::init_struct list[] = {
        { eterm::format(alloc, "{ok, N, A}"),     1 },
        { eterm::format(alloc, "{error, N, B}"),  2 },
        { eterm(atom("some_atom")),               3 },
        { eterm(12345),                           4 }
    };

    static eterm_pattern_matcher sp(list, boost::ref(cb), alloc);

    BOOST_REQUIRE_EQUAL(n == 1 ? 4u : 3u, sp.size());

    for(int i=0; i < iterations; i++) {
        BOOST_REQUIRE(!sp.match(eterm::format(alloc, "{ok, 1, 3, 4}")));
        auto f1 = eterm::format(alloc, "{ok, 1, 2}");
        auto m1 = sp.match(f1);
        BOOST_REQUIRE_EQUAL((n == 1), m1);   // N = 1

        BOOST_REQUIRE(sp.match(eterm::format(alloc, "{error, 2, not_found}"))); // N = 2
        BOOST_REQUIRE(!sp.match(eterm::format(alloc, "{test, 3}")));
    }

    // Verify number of successful matches for each pattern.
    BOOST_REQUIRE_EQUAL(iterations,   cb.match[0]);
    BOOST_REQUIRE_EQUAL(n*iterations, cb.match[1]);

    if (n == 1) {
        sp.erase(sp.front());
        BOOST_REQUIRE_EQUAL(3u, sp.size());   // N = 1
    }
}

BOOST_AUTO_TEST_CASE( test_match_static )
{
    run(1);
    run(2);
    run(3);
}

BOOST_AUTO_TEST_CASE( test_subst )
{
    eterm p = eterm::format("{perc, ID, List}");
    varbind binding;
    binding.bind("ID", eterm(123));
    list t(4);
    t.push_back(eterm(4));
    t.push_back(eterm(2.0));
    t.push_back(eterm(string("test")));
    t.push_back(eterm(atom("abcd")));
    t.close();
    binding.bind("List", eterm(t));

    eterm p1;
    BOOST_REQUIRE(p.subst(p1, &binding));
    BOOST_REQUIRE_EQUAL("{perc,123,[4,2.0,\"test\",abcd]}", p1.to_string());
}

BOOST_AUTO_TEST_CASE( test_match_list )
{
    const eterm data = eterm::format("{add_symbols, ['QQQQ', 'IBM']}");
    static eterm s_set_status  = eterm::format("{set_status,  I}");
    static eterm s_add_symbols = eterm::format("{add_symbols, S}");

    eixx::varbind l_binding;

    BOOST_REQUIRE(s_set_status.match(data, &l_binding) == false);
    bool b = s_add_symbols.match(data, &l_binding);
    BOOST_REQUIRE(b);
}

BOOST_AUTO_TEST_CASE( test_match_list_tail )
{
    BOOST_WARN_MESSAGE(false, "SKIPPING test_match_list_tail - needs extension to list matching!");
    BOOST_REQUIRE(true); // don't print the warning
    return;

    static const atom A("A");
    static const atom O("O");
    static const atom T("T");

    eterm pattern = eterm::format("[{A, O} | T]");
    eterm term    = eterm::format("[{a, 1}, {b, ok}, {c, 2.0}]");

    varbind vars;
    // Match [{a, 1} | T]
    BOOST_REQUIRE(pattern.match(term, &vars));
    const eterm* name  = vars.find(A);
    const eterm* value = vars.find(O);
    const eterm* tail  = vars.find(T);

    BOOST_REQUIRE(name  != NULL);
    BOOST_REQUIRE(value != NULL);
    BOOST_REQUIRE(tail  != NULL);

    BOOST_REQUIRE_EQUAL(ATOM,    name->type());
    BOOST_REQUIRE_EQUAL("a",     name->to_string());
    BOOST_REQUIRE_EQUAL(LONG,    value->type());
    BOOST_REQUIRE_EQUAL(LIST,    tail->type());

    // Match [{b, ok} | T]
    vars.clear();
    BOOST_REQUIRE(term.match(*tail, &vars));
    name  = vars.find(A);
    value = vars.find(O);
    tail  = vars.find(T);

    BOOST_REQUIRE(name  != NULL);
    BOOST_REQUIRE(value != NULL);
    BOOST_REQUIRE(tail  != NULL);

    BOOST_REQUIRE_EQUAL(ATOM,    name->type());
    BOOST_REQUIRE_EQUAL("b",     name->to_string());
    BOOST_REQUIRE_EQUAL(ATOM,    value->type());
    BOOST_REQUIRE_EQUAL(LIST,    tail->type());

    // Match [{c, 2.0} | T]
    vars.clear();
    BOOST_REQUIRE(term.match(*tail, &vars));
    name  = vars.find(A);
    value = vars.find(O);
    tail  = vars.find(T);

    BOOST_REQUIRE(name  != NULL);
    BOOST_REQUIRE(value != NULL);
    BOOST_REQUIRE(tail  != NULL);

    BOOST_REQUIRE_EQUAL(ATOM,    name->type());
    BOOST_REQUIRE_EQUAL("c",     name->to_string());
    BOOST_REQUIRE_EQUAL(DOUBLE,  value->type());
    BOOST_REQUIRE_EQUAL(LIST,    tail->type());
}

BOOST_AUTO_TEST_CASE( test_eterm_var_match )
{
    BOOST_REQUIRE(eterm(1)              .match(eterm::format("B")));
    BOOST_REQUIRE(eterm(10)             .match(eterm::format("B::int()")));
    BOOST_REQUIRE(eterm('c')            .match(eterm::format("B::int()")));
    BOOST_REQUIRE(eterm('c')            .match(eterm::format("B::byte()")));
    BOOST_REQUIRE(eterm('c')            .match(eterm::format("B::char()")));
    BOOST_REQUIRE(eterm("abc")          .match(eterm::format("B")));
    BOOST_REQUIRE(eterm("abc")          .match(eterm::format("B::string()")));
    BOOST_REQUIRE(eterm(atom("abc"))    .match(eterm::format("B")));
    BOOST_REQUIRE(eterm(atom("abc"))    .match(eterm::format("B::atom()")));
    BOOST_REQUIRE(eterm(10.123)         .match(eterm::format("B::float()")));
    BOOST_REQUIRE(eterm(10.123)         .match(eterm::format("B::double()")));
    BOOST_REQUIRE(eterm(binary{1,2,3})  .match(eterm::format("B::binary()")));
    BOOST_REQUIRE(eterm(true)           .match(eterm::format("B::boolean()")));
    BOOST_REQUIRE(eterm(false)          .match(eterm::format("B::bool()")));
    BOOST_REQUIRE(eterm(list{1,2.0,"a"}).match(eterm::format("B::list()")));
    BOOST_REQUIRE(eterm({1,2.0,"a"})    .match(eterm::format("B::tuple()")));
    BOOST_REQUIRE(eterm(epid())         .match(eterm::format("B::pid()")));
    BOOST_REQUIRE(eterm(port())         .match(eterm::format("B::port()")));
    BOOST_REQUIRE(eterm(ref())          .match(eterm::format("B::ref()")));
    BOOST_REQUIRE(eterm(ref())          .match(eterm::format("B::reference()")));
}
