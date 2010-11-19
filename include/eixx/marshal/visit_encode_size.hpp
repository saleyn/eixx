//----------------------------------------------------------------------------
/// \file  visit_encode_size.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing size encoder visitor.
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
#ifndef _IMPL_VISIT_ENCODE_SIZE_HPP_
#define _IMPL_VISIT_ENCODE_SIZE_HPP_

#include <eixx/marshal/visit.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <typename Alloc>
struct visit_eterm_encode_size_calc
    : public static_visitor<visit_eterm_encode_size_calc<Alloc>, size_t> {

    size_t operator()(bool   a) const { return 3 + (a ? 4 : 5); }
    size_t operator()(double a) const { return 9; }
    size_t operator()(long   a) const { int n = 0; ei_encode_longlong(NULL, &n, a); return n; }

    template <typename T>
    size_t operator()(const T& a) const { return a.encode_size(); }
};

} // namespace EIXX_NAMESPACE
} // namespace EIXX_NAMESPACE

#endif // _IMPL_VISIT_ENCODE_SIZE_HPP_
