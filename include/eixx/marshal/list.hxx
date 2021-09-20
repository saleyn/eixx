//----------------------------------------------------------------------------
/// \file  list.ipp
//----------------------------------------------------------------------------
/// \brief Implementation of the list member functions.
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

#include <eixx/marshal/endian.hpp>
#include <eixx/marshal/visit_to_string.hpp>
#include <eixx/marshal/visit_encode_size.hpp>
#include <eixx/marshal/visit_encoder.hpp>
#include <eixx/marshal/visit_subst.hpp>
#include <ei.h>

namespace eixx    {
namespace marshal {

template <class Alloc>
void list<Alloc>::init(const eterm<Alloc>* items, size_t N, const Alloc& alloc) {
    size_t n = N > 0 ? N : 1;
    m_blob = new blob_t(sizeof(header_t) + n*sizeof(cons_t), alloc);

    header_t* l_header      = header();
    cons_t*   hd            = l_header->head;

    l_header->initialized   = true;
    l_header->alloc_size    = n;
    l_header->size          = N;

    for(auto p = items, end = items+N; p != end; ++p, ++hd) {
        BOOST_ASSERT(p->initialized());
        new (&hd->node) eterm<Alloc>(*p);
        hd->next = hd+1;
    }

    if (N == 0)
        l_header->tail = NULL;
    else {
        l_header->tail = hd-1;
        l_header->tail->next = NULL;
    }
}

template <class Alloc>
list<Alloc>::list(const cons_t* a_head, size_t a_len, const Alloc& alloc)
    : base_t(alloc)
{
    size_t alloc_size;

    if (a_len > 0) {
        alloc_size = a_len;
    } else {
        for (const cons_t* p = a_head; p; p = p->next)
            a_len++;
        alloc_size = a_len;
    }

    // If this is an empty list - no allocation is needed
    if (alloc_size == 0) {
        m_blob = empty_list();
        return;
    }

    m_blob = new blob_t(sizeof(header_t) + alloc_size*sizeof(cons_t), alloc);
    header_t* l_header      = header();
    l_header->initialized   = true;
    l_header->alloc_size    = alloc_size;
    l_header->size          = alloc_size;
    if (alloc_size == 0)
        l_header->tail = NULL;
    else {
        cons_t* p = l_header->head;
        l_header->tail = &p[alloc_size-1];
        for (; a_head; p++, a_head = a_head->next) {
            new (&p->node) eterm<Alloc>(a_head->node);
            p->next = p+1;
        }
        l_header->tail->next = NULL;
    }
}

template <class Alloc>
list<Alloc>::list(const char *buf, uintptr_t& idx, size_t size, const Alloc& a_alloc)
    : base_t(a_alloc)
{
    BOOST_ASSERT(idx <= INT_MAX);
    int n;
    if (ei_decode_list_header(buf, (int*)&idx, &n) < 0)
        err_decode_exception("Error decoding list header", idx);

    size_t arity = static_cast<size_t>(n);
    // If this is an empty list - no allocation is needed
    if (arity == 0) {
        m_blob = empty_list();
        return;
    }

    m_blob = new blob_t(sizeof(header_t) + arity*sizeof(cons_t), a_alloc);
    header_t* l_header = header();
    l_header->initialized = true;
    l_header->alloc_size  = arity;
    l_header->size        = arity;

    cons_t* hd = l_header->head;
    for (cons_t* end = hd+arity; hd != end; ++hd) {
        eterm<Alloc> et(buf, idx, size, a_alloc);
        new (&hd->node) eterm<Alloc>(et);
        hd->next = hd+1;
    }
    if (arity == 0) {
        l_header->tail = NULL;
    } else {
        l_header->tail = hd-1;
        l_header->tail->next = NULL;
        if (*(buf+idx) != ERL_NIL_EXT)
            throw err_decode_exception("Not a NIL list!", idx);
        idx++;
    }
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void list<Alloc>::encode(char* buf, uintptr_t& idx, size_t size) const
{
    BOOST_ASSERT(initialized());
    char* s = buf + idx;
    if (empty()) {
        put8(s,ERL_NIL_EXT);
    } else {
        put8(s,ERL_LIST_EXT);
        const header_t* l_header = header();
        auto sz = l_header->size;
        if (sz > UINT32_MAX)
            throw err_encode_exception("LIST_EXT length exceeds maximum");
        uint32_t len = (uint32_t)sz;
        put32be(s, len);
        idx += 5;
        for(const cons_t* p = l_header->head; p; p = p->next) {
            visit_eterm_encoder visitor(buf, idx, size);
            visitor.apply_visitor(p->node);
        }
        s = buf + idx;
        put8(s,ERL_NIL_EXT);
    }
    idx++;
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
list<Alloc> list<Alloc>::tail(size_t idx) const
{
    const header_t* l_header = header();
    const cons_t* p = l_header->head;
    size_t len = l_header->size - idx - 1;
    if (idx >= l_header->size)
        throw err_bad_argument("List too short");
    for (size_t i=0; i <= idx; i++)
        p = p->next;
    list<Alloc> l(p, len, this->get_allocator());
    return l;
}

template <class Alloc>
void list<Alloc>::push_back(const eterm<Alloc>& a)
{
    BOOST_ASSERT(a.initialized());
    if (unlikely(!m_blob)) {
        m_blob = new blob_t(sizeof(header_t) + sizeof(cons_t), this->get_allocator());
        header_t* hd = header();
        hd->initialized = false;
        hd->tail = hd->head;
        hd->size = 1;
        hd->alloc_size = 1;
        cons_t* p = hd->tail;
        new (&p->node) eterm<Alloc>(a);
        p->next = NULL;
        return;
    }
    BOOST_ASSERT(!initialized());
    header_t* hd = header();
    bool has_space = hd->size < hd->alloc_size;
    cons_t* p = has_space ? &hd->head[hd->size] : this->get_t_allocator().allocate(1);
    new (&p->node) eterm<Alloc>(a);
    p->next  = NULL;
    if (likely(hd->size > 0))
        hd->tail->next = p;
    hd->tail = p;
    hd->size++;
}

template <class Alloc>
bool list<Alloc>::subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
{
    // We check if any contained term changes.
    bool changed = false;
    const header_t* l_header = header();

    if (l_header->size == 0)
        return false;

    Alloc alloc = this->get_allocator();
    list<Alloc> l_new(l_header->size, alloc);

    for (const cons_t* p=l_header->head; p; p = p->next) {
        eterm<Alloc> l_ele;
        visit_eterm_subst<Alloc> visitor(l_ele, binding);
        if (!visitor.apply_visitor(p->node))
            l_new.push_back(p->node);
        else {
            changed = true;
            l_new.push_back(l_ele);
        }
    }

    if (!changed)
        return false;

    // If changed, return the new list
    l_new.close();
    out = l_new;
    return true;
}

template <class Alloc>
bool list<Alloc>::match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const
{
    switch (pattern.type()) {
        case VAR:  return pattern.match(eterm<Alloc>(*this), binding);
        case LIST: break;
        default:   return false;
    }

    const list<Alloc>& pl = pattern.to_list();
    if (unlikely(!initialized() || !pl.initialized()))
        throw err_invalid_term("List not initialized!");

    const header_t* l_header = header();
    // Do a quick check on the size.
    if (l_header->size != pl.length())
        return false;

    const_iterator it1  = begin(), it2  = pl.begin(),
                   end1 = end(),   end2 = pl.end();
    for (; it1 != end1 && it2 != end2; ++it1, ++it2)
        if (!it1->match(*it2, binding))
            return false;

    BOOST_ASSERT(it1 == end1 && it2 == end2);
    return true;
}

template <class Alloc>
std::ostream& list<Alloc>::dump(std::ostream& out, const varbind<Alloc>* vars) const
{
    out << '[';
    const cons_t* hd = empty() ? NULL : header()->head;
    for(const cons_t* p = hd; p; p = p->next) {
        out << (p != hd ? "," : "");
        const visit_eterm_stringify<Alloc> visitor(out, vars);
        visitor.apply_visitor(p->node);
    }
    return out << ']';
}

} // namespace marshal
} // namespace eixx
