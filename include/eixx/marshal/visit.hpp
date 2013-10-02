//----------------------------------------------------------------------------
/// \file  visit.hpp
//----------------------------------------------------------------------------
/// \brief A number of helper classes implementing visitation pattern for
///        eterm objects.
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
#ifndef _IMPL_VISIT_HPP_
#define _IMPL_VISIT_HPP_

#include <eixx/marshal/eterm.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

    template <typename Alloc> class tuple;
    template <typename Alloc> class list;
    class var;

    template <typename ResultType, typename Visitor>
    struct wrap {
        const ResultType result;
        template <typename Param1>
        wrap(const Visitor& v, const Param1& t1)
            : result( v(t1) )
        {}
    };

    template <typename Visitor>
    struct wrap<void, Visitor> {
        template <typename Param1>
        wrap(const Visitor& v, const Param1& t) { v(t); }
    };

template <typename Derived, typename ResultType>
struct static_visitor {
    typedef ResultType result_type;

    template <typename Alloc>
    ResultType apply_visitor(const eterm<Alloc>& et) const {
        return et.visit(*static_cast<const Derived*>(this)).result;
    }
};

template <typename Derived>
struct static_visitor<Derived, void> {
    typedef void result_type;

    template <typename Alloc>
    void apply_visitor(const eterm<Alloc>& et) const {
        et.visit(*static_cast<const Derived*>(this));
    }
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

#endif // _IMPL_VISIT_HPP_
