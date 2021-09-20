//----------------------------------------------------------------------------
/// \file  list.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing a list object of Erlang external 
///        term format. The list can contain any heterogeneous eterm's.
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
#pragma once

#include <list>
#include <boost/static_assert.hpp>
#include <eixx/marshal/defaults.hpp>
#include <eixx/marshal/visit_encode_size.hpp>
#include <initializer_list>

namespace eixx {
namespace marshal {

template <typename Alloc>
struct cons {
    eterm<Alloc> node;
    cons<Alloc>* next;
};

template <typename Alloc>
class list : protected alloc_base<cons<Alloc>, Alloc> {
public:
    typedef cons<Alloc> cons_t;
private:
    typedef alloc_base<cons_t, Alloc> base_t;

    struct header_t {
        header_t()          {}
        header_t(std::nullptr_t)
            : initialized(true)
            , alloc_size (0)
            , size       (0)
            , tail       (nullptr)
        {}
        bool            initialized;
        size_t    alloc_size;
        size_t          size;
        cons_t*         tail;
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        cons_t          head[0];
        #pragma GCC diagnostic pop
    };

    typedef blob<char, Alloc> blob_t;

    blob_t* m_blob;

    /// Returns a pointer to a singleton empty list
    static blob_t* empty_list() {
        auto creator = []() {
            auto p = new blob_t(sizeof(header_t));
            auto h = reinterpret_cast<header_t*>(p->data());
            new (h) header_t(nullptr);
            return p;
        };
        static std::unique_ptr<blob_t> s_empty(creator());
        return s_empty.get();
    }

    header_t* header() {
        BOOST_ASSERT(m_blob); return reinterpret_cast<header_t*>(m_blob->data());
    }
    const header_t* header() const {
        BOOST_ASSERT(m_blob); return reinterpret_cast<const header_t*>(m_blob->data());
    }
    const cons_t* head() const    { return header()->head; }
    cons_t*       head()          { return header()->head; }
    cons_t*       tail()          { return header()->tail; }

    void release() {
        if (!m_blob || m_blob == empty_list())
           return;
        if (m_blob->release(false)) {
            header_t* l_header = header();
            if (l_header->size > 0) {
                for (cons_t* p = head(); p; p = p->next)
                    p->node.~eterm();
                // If there were any allocations after the original 
                // construction of the list head descriptor, deallocate
                // all the following cons.
                if (l_header->alloc_size > 0 && l_header->size > l_header->alloc_size) 
                    for (cons_t* p = (l_header->head + l_header->alloc_size - 1)->next, *q; p; p = q) {
                        q = p->next;
                        this->get_t_allocator().deallocate(p, 1);
                    }
            }
            m_blob->free();
        }
    }

    // For use only from constructors.
    void init(const eterm<Alloc> items[], size_t a_size, const Alloc& alloc);
public:
    class iterator;
    typedef const iterator const_iterator;

    iterator       begin()       { iterator it(empty() ? NULL : head()); return it; }
    iterator       end()         { return iterator::end(); }

    const_iterator begin() const { const_iterator it(empty() ? NULL : head()); return it; }
    const_iterator end()   const { return iterator::end(); }

    explicit list(const Alloc& alloc = Alloc())
        : base_t(alloc)
        , m_blob(NULL)
    {}

    /// Construct an NIL list (initialized list with no elements)
    explicit list(std::nullptr_t) : m_blob(empty_list()) {}

    /// Construct a list with a given estimated size.
    ///
    /// When a_estimated_size is 0, an empty initialized list is created. Otherwise,
    /// the list is not initialized.
    explicit list(int a_estimated_size, const Alloc& alloc = Alloc()) {
        if (a_estimated_size < 0)
            throw err_bad_argument("List too short");
        list((size_t)a_estimated_size, alloc);
    }

    /// Construct a list with a given estimated size.
    ///
    /// When a_estimated_size is 0, an empty initialized list is created. Otherwise,
    /// the list is not initialized.
    explicit list(size_t a_estimated_size, const Alloc& alloc = Alloc())
        : base_t(alloc)
    {
        if (a_estimated_size == 0)
            m_blob = empty_list();
        else {
            m_blob = new blob_t(sizeof(header_t) + a_estimated_size*sizeof(cons_t), alloc);
            header_t* hdr      = header();
            hdr->initialized   = a_estimated_size == 0;
            hdr->alloc_size    = a_estimated_size;
            hdr->size          = 0;
            hdr->tail          = NULL;
        }
    }

    list(const list<Alloc>& a) : base_t(a.get_allocator()), m_blob(a.m_blob) {
        BOOST_ASSERT(a.initialized());
        if (m_blob) m_blob->inc_rc();
    } 

    list(list<Alloc>&& a) : base_t(a.get_allocator()), m_blob(a.m_blob) {
        a.m_blob = nullptr;
    }

    explicit list(const cons_t* a_head, size_t a_len = 0, const Alloc& alloc = Alloc());

    template <size_t N>
    list(const eterm<Alloc> (&items)[N], const Alloc& alloc = Alloc())
        : list(items, N, alloc) {}

    list(const eterm<Alloc>* items, size_t a_size, const Alloc& alloc = Alloc())
        : base_t(alloc) { init(items, a_size, alloc); }

    list(std::initializer_list<eterm<Alloc>> items, const Alloc& alloc = Alloc())
        : list(items.begin(), items.size(), alloc) {}

    /**
     * Decode the list from a binary buffer.
     */
    explicit list(const char* buf, uintptr_t& idx, size_t size, const Alloc& a_alloc = Alloc());

    ~list() { release(); }

    /**
     * Add a term to list. Tuples added must be fully filled
     * with all elements, and lists added must be properly closed (initialized).
     */
    void push_back(const eterm<Alloc>& a);

    template <typename G>
    void push_back(const G& a) {
        eterm<Alloc> t(a);
        push_back(t);
    }

    /**
     * Closes the list.
     * A list must be closed before it can be copied or included into other terms.
     */
    void    close() {
        if (!m_blob || m_blob == empty_list()) return;
        header()->initialized = true;
    }

    /// Return list length. This method has O(1) complexity.
    size_t  length()        const { return  m_blob ?  header()->size :  0; }
    bool    empty()         const { return !m_blob || header()->size == 0; }
    bool    initialized()   const { return  m_blob && header()->initialized; }

    /// Return pointer to the N'th element in the list. This method has
    /// O(N) complexity.
    const eterm<Alloc>& nth(size_t n) const {
        if (n > length())
            throw err_bad_argument("Index out of bounds", n);
        size_t i = 0;
        auto it = begin();
        for(auto endit = end(); it != endit && i < n; ++it, ++i);
        return *it;
    }

    list<Alloc> tail(size_t idx) const;

    list<Alloc>& operator= (const list<Alloc>& rhs) {
        BOOST_ASSERT(rhs.initialized());
        release();
        m_blob = rhs.m_blob;
        if (m_blob) m_blob->inc_rc();
        return *this;
    }

    bool operator== (const list<Alloc>& rhs) const {
        const_iterator it1  = begin(), it2  = rhs.begin(),
                       end1 = end(),   end2 = rhs.end();
        for(; it1 != end1 && it2 != end2; ++it1, ++it2) {
            if (!(*it1 == *it2))
                return false;
        }
        return it1 == end1 && it2 == end2;
    }

    bool operator< (const list<Alloc>& rhs) const {
        const_iterator it1  = begin(), it2  = rhs.begin(),
                       end1 = end(),   end2 = rhs.end();
        for(; it1 != end1 && it2 != end2; ++it1, ++it2) {
            if (!(*it1 < *it2))
                return false;
        }
        return it1 == end1 && it2 != end2;
    }

    size_t encode_size() const {
        if (length() == 0)
            return 1;
        size_t result = 5 + 1 /* 1 byte for ERL_NIL_EXT */;
        const header_t* hd = header();
        BOOST_ASSERT(hd->initialized);
        for (const cons_t* it=hd->head; it != NULL; it = it->next) {
            visit_eterm_encode_size_calc<Alloc> visitor;
            result += visitor.apply_visitor(it->node);
        }
        return result;
    }

    void encode(char* buf, uintptr_t& idx, size_t size) const;

    bool subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const;

    bool match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const;

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* vars = NULL) const;

    static list<Alloc> make(const Alloc& a = Alloc()) {
        return list<Alloc>(0, a);
    }

    template <class T1>
    static list<Alloc> make(T1 t1, const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1) };
        return list<Alloc>(l, a);
    }

    template <class T1, class T2>
    static list<Alloc> make(T1 t1, T2 t2,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2) };
        return list<Alloc>(l, a);
    }

    template <class T1, class T2, class T3>
    static list<Alloc> make(T1 t1, T2 t2, T3 t3,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3) };
        return list<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4>
    static list<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4) };
        return list<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4, class T5>
    static list<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5) };
        return list<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6>
    static list<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, 
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6) };
        return list<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    static list<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6),
                             eterm<Alloc>::cast(t7) };
        return list<Alloc>(l, a);
    }

    template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    static list<Alloc> make(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8,
        const Alloc& a = Alloc()) {
        eterm<Alloc> l[] = { eterm<Alloc>::cast(t1),
                             eterm<Alloc>::cast(t2),
                             eterm<Alloc>::cast(t3),
                             eterm<Alloc>::cast(t4),
                             eterm<Alloc>::cast(t5),
                             eterm<Alloc>::cast(t6),
                             eterm<Alloc>::cast(t7),
                             eterm<Alloc>::cast(t8) };
        return list<Alloc>(l, a);
    }
};

template <typename Alloc>
class list<Alloc>::iterator {
    mutable cons_t* cursor;
public:
    iterator(const cons_t* a_cur) : cursor(const_cast<cons_t*>(a_cur)) {}
    static iterator end() { iterator it(NULL); return it; }
    iterator&       operator++()        { cursor = cursor->next; return *this; }
    const iterator& operator++() const  { cursor = cursor->next; return *this; }
    const iterator  operator++(int)const{ iterator it(cursor); cursor = cursor->next; return it; }
    iterator        operator++(int)     { iterator it(cursor); cursor = cursor->next; return it; }
    const eterm<Alloc>& operator*()  const { BOOST_ASSERT(cursor); return cursor->node; }
    eterm<Alloc>&   operator*()         { BOOST_ASSERT(cursor); return cursor->node; }
    eterm<Alloc>*   operator->()        { BOOST_ASSERT(cursor); return &cursor->node; }
    const eterm<Alloc>* operator->() const { BOOST_ASSERT(cursor); return &cursor->node; }
    bool            operator==(const iterator& rhs) const { return cursor == rhs.cursor; }
    bool            operator!=(const iterator& rhs) const { return cursor != rhs.cursor; }
};

} // namespace marshal
} // namespace eixx

namespace std {

    template <class Alloc>
    ostream& operator<< (ostream& out, const eixx::marshal::list<Alloc>& a) {
        return a.dump(out);
    }

} // namespace std

#include <eixx/marshal/list.hxx>
