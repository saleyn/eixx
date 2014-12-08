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

#include <eixx/marshal/endian.hpp>
#include <eixx/marshal/visit_to_string.hpp>
#include <eixx/marshal/visit_encode_size.hpp>
#include <eixx/marshal/visit_encoder.hpp>
#include <eixx/marshal/visit_subst.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
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
list<Alloc>::list(const cons_t* a_head, int a_len, const Alloc& alloc)
    throw (err_bad_argument) : base_t(alloc)
{
    unsigned int alloc_size;

    if (a_len >= 0) {
        alloc_size = a_len;
    } else if (a_len == -1) {
        a_len = 0;
        for (const cons_t* p = a_head; p; p = p->next)
            a_len++;
        alloc_size = a_len;
    } else
        throw err_bad_argument("List of negative length!");

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
list<Alloc>::list(const char *buf, int& idx, size_t size, const Alloc& a_alloc)
    throw(err_decode_exception) : base_t(a_alloc) 
{
    int arity;
    if (ei_decode_list_header(buf, &idx, &arity) < 0)
        err_decode_exception("Error decoding list header", idx);
// TODO: optimize for an empty list!!!
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
void list<Alloc>::encode(char* buf, int& idx, size_t size) const
{
    BOOST_ASSERT(initialized());
    char* s = buf + idx;
    if (empty()) {
        put8(s,ERL_NIL_EXT);
    } else {
        put8(s,ERL_LIST_EXT);
        const header_t* l_header = header();
        put32be(s,l_header->size);
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
list<Alloc> list<Alloc>::tail(size_t idx) const throw(err_bad_argument)
{
    const header_t* l_header = header();
    const cons_t* p = l_header->head;
    int len = (int)l_header->size - (int)idx - 1;
    if (len < 0)
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
        p->node.set(a);
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
    throw (err_unbound_variable)
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
    throw (err_invalid_term, err_unbound_variable)
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
} // namespace EIXX_NAMESPACE
