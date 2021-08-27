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

namespace eixx {
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
        return static_cast<size_t>(m_blob->data()[m_blob->size()-1].to_long()-1);
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
        #ifndef __clang__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wclass-memaccess"
        #endif
        memset(m_blob->data(), 0, sizeof(eterm<Alloc>)*m_blob->size());
        #ifndef __clang__
        #pragma GCC diagnostic pop
        #endif
        set_init_size(0);
    }

    tuple(const tuple<Alloc>& a) : m_blob(a.m_blob) {
        BOOST_ASSERT(a.initialized());
        m_blob->inc_rc();
    }

    tuple(tuple<Alloc>&& a) : m_blob(a.m_blob) {
        a.m_blob = nullptr;
    }

    template <size_t N>
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
    tuple(const char* buf, uintptr_t& idx, size_t size, const Alloc& a_alloc = Alloc());

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

    bool operator< (const tuple<Alloc>& rhs) const {
        if (size() < rhs.size())
            return true;
        if (size() > rhs.size())
            return false;
        for(const_iterator it1 = begin(),
            it2 = rhs.begin(), iend = end(); it1 != iend; ++it1, ++it2)
        {
            if (!(*it1 < *it2))
                return false;
        }
        return true;
    }

    const eterm<Alloc>& operator[] (size_t idx) const {
        BOOST_ASSERT(m_blob && idx < size());
        return m_blob->data()[idx];
    }

    eterm<Alloc>& operator[] (size_t idx) {
        BOOST_ASSERT(m_blob && idx < size());
        return m_blob->data()[idx];
    }

    template <typename T>
    void push_back(const T& t) {
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

    void encode(char* buf, uintptr_t& idx, size_t size) const;

    bool subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const;

    bool match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const;

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
} // namespace eixx

namespace std {

    template <class Alloc>
    ostream& operator<< (ostream& out, const eixx::marshal::tuple<Alloc>& a) {
        return a.dump(out);
    }

} // namespace std

#include <eixx/marshal/tuple.hxx>

#endif // _IMPL_TUPLE_HPP_
