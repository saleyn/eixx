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
#ifndef _IMPL_VISIT_SUBST_HPP_
#define _IMPL_VISIT_SUBST_HPP_

#include <eixx/marshal/visit.hpp>
//#include <eixx/marshal/tuple.hpp>
//#include <eixx/marshal/list.hpp>
#include <eixx/marshal/var.hpp>
#include <ei.h>

namespace eixx {
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

} // namespace eixx
} // namespace eixx

#endif // _IMPL_VISIT_SUBST_HPP_
