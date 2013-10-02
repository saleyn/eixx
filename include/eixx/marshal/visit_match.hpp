//----------------------------------------------------------------------------
/// \file  visit_match.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing pattern matching visitor.
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
#ifndef _IMPL_VISIT_MATCH_HPP_
#define _IMPL_VISIT_MATCH_HPP_

#include <eixx/marshal/visit.hpp>
#include <eixx/marshal/tuple.hpp>
#include <eixx/marshal/list.hpp>
#include <eixx/marshal/var.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <typename Alloc>
class visit_eterm_match
    : public static_visitor<visit_eterm_match<Alloc>, bool> {
    const eterm<Alloc>& m_pattern;
    varbind<Alloc>*     m_binding;
public:
    visit_eterm_match(const eterm<Alloc>& a_pattern, varbind<Alloc>* a_binding)
        : m_pattern(a_pattern), m_binding(a_binding)
    {}

    bool operator()(const tuple<Alloc>& a) const { return a.match(m_pattern, m_binding); }
    bool operator()(const list<Alloc>&  a) const { return a.match(m_pattern, m_binding); }
    bool operator()(const var&          a) const { return a.match(m_pattern, m_binding); }

    template <typename T>
    bool operator()(const T& a) const {
        // default behaviour.
        eterm<Alloc> et(a);
        if (m_pattern.type() == VAR)
            return m_pattern.match(et, m_binding);
        else
            return et == m_pattern;
    }
};

} // namespace EIXX_NAMESPACE
} // namespace EIXX_NAMESPACE

#endif // _IMPL_VISIT_MATCH_HPP_
