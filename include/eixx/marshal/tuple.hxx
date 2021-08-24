//----------------------------------------------------------------------------
/// \file  tuple.hxx
//----------------------------------------------------------------------------
/// \brief Implementation of tuple's member functions.
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
#include <eixx/marshal/visit_encode_size.hpp>
#include <eixx/marshal/visit_encoder.hpp>
#include <eixx/marshal/visit_to_string.hpp>
#include <eixx/marshal/visit_subst.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {

template <class Alloc>
tuple<Alloc>::tuple(const char* buf, uintptr_t& idx, size_t size, const Alloc& a_alloc)
{
    BOOST_ASSERT(idx <= INT_MAX);
    int arity;
    if (ei_decode_tuple_header(buf, (int*)&idx, &arity) < 0)
        err_decode_exception("Error decoding tuple header", idx);
    m_blob = new blob<eterm<Alloc>, Alloc>(arity+1, a_alloc);
    for (int i=0; i < arity; i++) {
        new (&m_blob->data()[i]) eterm<Alloc>(buf, idx, size, a_alloc);
    }
    set_init_size(arity);
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void tuple<Alloc>::encode(char* buf, uintptr_t& idx, size_t size) const
{
    BOOST_ASSERT(initialized());
    BOOST_ASSERT(idx <= INT_MAX);
    size_t arity = this->size();
    if (arity > INT_MAX)
        throw err_encode_exception("LARGE_TUPLE_EXT arity exceeds maximum supported");
    ei_encode_tuple_header(buf, (int*)&idx, (int)arity);
    for(const_iterator it = begin(), iend=end(); it != iend; ++it) {
        visit_eterm_encoder visitor(buf, idx, size);
        visitor.apply_visitor(*it);
    }
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
bool tuple<Alloc>::subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
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
} // namespace eixx
