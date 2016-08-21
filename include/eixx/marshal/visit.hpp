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
#ifndef _IMPL_VISIT_HPP_
#define _IMPL_VISIT_HPP_

#include <eixx/marshal/eterm.hpp>

namespace eixx {
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
} // namespace eixx

#endif // _IMPL_VISIT_HPP_
