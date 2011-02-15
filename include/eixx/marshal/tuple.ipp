//----------------------------------------------------------------------------
/// \file  tuple.ipp
//----------------------------------------------------------------------------
/// \brief Implementation of tuple's member functions.
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
#include <eixx/marshal/visit_encode_size.hpp>
#include <eixx/marshal/visit_encoder.hpp>
#include <eixx/marshal/visit_to_string.hpp>
#include <eixx/marshal/visit_subst.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <class Alloc>
tuple<Alloc>::tuple(const char* buf, int& idx, size_t size, const Alloc& a_alloc)
    throw(err_decode_exception)
{
    int arity;
    if (ei_decode_tuple_header(buf, &idx, &arity) < 0)
        err_decode_exception("Error decoding tuple header", idx);
    m_blob = new blob<eterm<Alloc>, Alloc>(arity+1, a_alloc);
    for (int i=0; i < arity; i++) {
        new (&m_blob->data()[i]) eterm<Alloc>(buf, idx, size, a_alloc);
    }
    set_init_size(arity);
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void tuple<Alloc>::encode(char* buf, int& idx, size_t size) const
{
    BOOST_ASSERT(initialized());
    ei_encode_tuple_header(buf, &idx, this->size());
    for(const_iterator it = begin(), iend=end(); it != iend; ++it) {
        visit_eterm_encoder visitor(buf, idx, size);
        visitor.apply_visitor(*it);
    }
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
bool tuple<Alloc>::subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
    throw (err_unbound_variable)
{
    // We check if any contained term changes.
    bool changed = false;
    tuple<Alloc> l_new(size());

    for(const_iterator it = begin(), iend=end(); it != iend; ++it) {
        eterm<Alloc> l_ele;
        visit_eterm_subst<Alloc> visitor(l_ele, binding);
        if (!visitor.apply_visitor(*it))
            l_new.push_back(*it);
        else {
            changed = true;
            l_new.push_back(l_ele);
        }
    }

    if (!changed)
        return false;

    // If changed, return the new tuple
    out = l_new;
    return true;
}

template <class Alloc>
bool tuple<Alloc>::match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const
    throw (err_invalid_term, err_unbound_variable)
{
    switch (pattern.type()) {
        case VAR:   return pattern.match(eterm<Alloc>(*this), binding);
        case TUPLE: break;
        default:    return false;
    }

    const tuple<Alloc>& pt = pattern.to_tuple();
    if (unlikely(!initialized() || !pt.initialized()))
        throw err_invalid_term("Tuple not initialized!");
    if (size() != pt.size())
        return false;
    for (size_t i=0, n=size(); i < n; ++i)
        if (!m_blob->data()[i].match(pt[i], binding))
            return false;
    return true;
}

template <class Alloc>
std::ostream& tuple<Alloc>::dump(std::ostream& out, const varbind<Alloc>* vars) const
{
    out << '{';
    for(const_iterator it = begin(), first=begin(), iend=end(); it != iend; ++it) {
        out << (it != first ? "," : "");
        visit_eterm_stringify<Alloc> visitor(out, vars);
        visitor.apply_visitor(*it);
    }
    return out << '}';
}

} // namespace marshal
} // namespace EIXX_NAMESPACE
