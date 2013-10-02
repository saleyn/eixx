//----------------------------------------------------------------------------
/// \file  visit_subst.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing variable substitution visitor.
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
#ifndef _IMPL_VISIT_SUBST_HPP_
#define _IMPL_VISIT_SUBST_HPP_

#include <eixx/marshal/visit.hpp>
//#include <eixx/marshal/tuple.hpp>
//#include <eixx/marshal/list.hpp>
#include <eixx/marshal/var.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <typename Alloc>
class visit_eterm_subst
    : public static_visitor<visit_eterm_subst<Alloc>, bool> {
    eterm<Alloc>& m_out;
    const varbind<Alloc>* m_binding;
public:
    visit_eterm_subst(eterm<Alloc>& a_out, const varbind<Alloc>* a_binding)
        : m_out(a_out), m_binding(a_binding)
    {}

    bool operator()(const tuple<Alloc>& a) const { return a.subst(m_out, m_binding); }
    bool operator()(const list<Alloc>&  a) const { return a.subst(m_out, m_binding); }
    bool operator()(const var&          a) const { return a.subst(m_out, m_binding); }

    template <typename T>
    bool operator()(const T& a) const { return false; }
};

} // namespace EIXX_NAMESPACE
} // namespace EIXX_NAMESPACE

#endif // _IMPL_VISIT_SUBST_HPP_
