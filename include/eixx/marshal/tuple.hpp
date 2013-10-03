//----------------------------------------------------------------------------
/// \file  tuple.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an tuple object of Erlang external 
///        term format. It is a vector of non-homogeneous eterm types.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) Library.

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
#ifndef _IMPL_TUPLE_HPP_
#define _IMPL_TUPLE_HPP_

#include <ostream>
#include <initializer_list>
#include <boost/static_assert.hpp>
#include <eixx/marshal/alloc_base.hpp>
#include <eixx/marshal/varbind.hpp>
#include <eixx/marshal/visit.hpp>
#include <eixx/marshal/visit_encode_size.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <typename Alloc> class eterm;

template <typename Alloc>
class tuple {
    blob<eterm<Alloc>, Alloc>* m_blob;

    // We store tuple's effective size in last element. The size is value+1, so 
    // that we can distinguish initialized empty tuple {} from an uinitialized tuple.
    void   set_init_size(size_t n) {
        BOOST_ASSERT(m_blob);
        new (&m_blob->data()[m_blob->size()-1]) eterm<Alloc>(n+1);
    }
    size_t get_init_size() const {
        BOOST_ASSERT(m_blob);
        return m_blob->data()[m_blob->size()-1].to_long()-1;
    }

    void release() { release(m_blob); m_blob = nullptr; }

    void release(blob<eterm<Alloc>, Alloc>* p) {
        if (p && p->release(false)) {
            for(size_t i=0, n=size(); i < n; i++)
                p->data()[i].~eterm();
            p->free();
        }
    }

protected:
    template <typename V>
    void init_element(size_t i, const V& v) {
        BOOST_ASSERT(m_blob && i < size());
        new (&m_blob->data()[i]) eterm<Alloc>(v);
    }

    void set_initialized() { set_init_size(m_blob->size()-1); }

public:
    typedef eterm<Alloc>*       iterator;
    typedef const eterm<Alloc>* const_iterator;

    tuple() : m_blob(NULL) {}

    explicit tuple(size_t arity, const Alloc& alloc = Alloc())
        : m_blob(new blob<eterm<Alloc>, Alloc>(arity+1, alloc))
    {
        memset(m_blob->data(), 0, sizeof(eterm<Alloc>)*m_blob->size());
        set_init_size(0);
    }

    tuple(const tuple<Alloc>& a) : m_blob(a.m_blob) {
        BOOST_ASSERT(a.initialized());
        m_blob->inc_rc();
    }

    tuple(tuple<Alloc>&& a) : m_blob(a.m_blob) {
        a.m_blob = nullptr;
    }

    template <int N>
    tuple(const eterm<Alloc> (&items)[N], const Alloc& alloc = Alloc())
        : tuple(items, N, alloc) {}

    tuple(const eterm<Alloc>* items, size_t a_size, const Alloc& alloc = Alloc())
        : m_blob(new blob<eterm<Alloc>, Alloc>(a_size+1, alloc)) {
        for(size_t i=0; i < a_size; i++) {
            new (&m_blob->data()[i]) eterm<Alloc>(items[i]);
        }
        set_init_size(a_size);
    }

    tuple(std::initializer_list<eterm<Alloc>> list, const Alloc& alloc = Alloc())
        : tuple(list.begin(), list.size(), alloc) {}

    /**
     * Decode the tuple from a binary buffer.
     */
    tuple(const char* buf, int& idx, size_t size, const Alloc& a_alloc = Alloc())
        throw(err_decode_exception);

    ~tuple() {
        release();
    }

    tuple<Alloc>& operator= (const tuple<Alloc>& rhs) {
        if (this != &rhs) {
            auto p = m_blob;
            m_blob = rhs.m_blob;
            if (m_blob) m_blob->inc_rc();
            release(p);
        }
        return *this;
    }

    tuple<Alloc>& operator= (tuple<Alloc>&& rhs) {
        if (this != &rhs) {
            auto p = m_blob;
            m_blob = rhs.m_blob;
            rhs.m_blob = nullptr;
            release(p);
        }
        return *this;
    }

    bool operator== (const tuple<Alloc>& rhs) const {
        if (size() != rhs.size())
            return false;
        for(const_iterator it1 = begin(),
            it2 = rhs.begin(), iend = end(); it1 != iend; ++it1, ++it2)
        {
            if (!(*it1 == *it2))
                return false;
        }
        return true;
    }

    const eterm<Alloc>& operator[] (int idx) const {
        BOOST_ASSERT(m_blob && (size_t)idx < size());
        return m_blob->data()[idx];
    }

    eterm<Alloc>& operator[] (int idx) {
        BOOST_ASSERT(m_blob && (size_t)idx < size());
        return m_blob->data()[idx];
    }

    template <typename T>
    void push_back(const T& t) throw (err_invalid_term) {
        BOOST_ASSERT(m_blob);
        if (initialized())
            throw err_invalid_term("Attempt to change immutable tuple!");
        size_t i = get_init_size();
        new (&m_blob->data()[i]) eterm<Alloc>(t);
        set_init_size(i+1);
    }

    size_t size() const { BOOST_ASSERT(m_blob); return m_blob->size()-1; } // Last element is filled size

    bool   initialized()   const   { return size() == get_init_size(); }

    iterator       begin()         { BOOST_ASSERT(m_blob); return m_blob->data();   }
    iterator       end()           { BOOST_ASSERT(m_blob); return &m_blob->data()[m_blob->size()-1]; }
    const_iterator begin() const   { BOOST_ASSERT(m_blob); return m_blob->data();   }
    const_iterator end()   const   { BOOST_ASSERT(m_blob); return &m_blob->data()[m_blob->size()-1]; }

    size_t encode_size() const {
        BOOST_ASSERT(initialized());
        size_t result = size() <= 0xff ? 2 : 5;
        for (const_iterator it = begin(), iend = end(); it != iend; ++it)
            result += visit_eterm_encode_size_calc<Alloc>().apply_visitor(*it);
        return result;
    }

    void encode(char* buf, int& idx, size_t size) const;

    bool subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
        throw (err_unbound_variable);

    bool match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const
        throw (err_invalid_term, err_unbound_variable);

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* vars = NULL) const;

    template <class T1>
    static tuple<Alloc> make(T1 t1, const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2>
    static tuple<Alloc> make(T1 t1, T2 t2,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4, class T5>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, 
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6),
                             eterm<Alloc>::cast(t7) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4,
              class T5, class T6, class T7, class T8>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4,
                             T5 t5, T6 t6, T7 t7, T8 t8,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6),
                             eterm<Alloc>::cast(t7),
                             eterm<Alloc>::cast(t8) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4,
              class T5, class T6, class T7, class T8,
              class T9>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4,
                             T5 t5, T6 t6, T7 t7, T8 t8,
                             T9 t9,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6),
                             eterm<Alloc>::cast(t7),
                             eterm<Alloc>::cast(t8),
                             eterm<Alloc>::cast(t9) };
        return tuple<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4,
              class T5, class T6, class T7, class T8,
              class T9, class T10>
    static tuple<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4,
                             T5 t5, T6 t6, T7 t7, T8 t8,
                             T9 t9, T10 t10,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6),
                             eterm<Alloc>::cast(t7),
                             eterm<Alloc>::cast(t8),
                             eterm<Alloc>::cast(t9),
                             eterm<Alloc>::cast(t10) };
        return tuple<Alloc>(l, a);
    }

};

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {

    template <class Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::tuple<Alloc>& a) {
        return a.dump(out);
    }

} // namespace std

#include <eixx/marshal/tuple.ipp>

#endif // _IMPL_TUPLE_HPP_
