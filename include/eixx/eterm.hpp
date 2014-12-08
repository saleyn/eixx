//----------------------------------------------------------------------------
/// \file  eterm.hpp
//----------------------------------------------------------------------------
/// \brief Header file for including marshaling part of eixx.
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
#ifndef _EIXX_ETERM_HPP_
#define _EIXX_ETERM_HPP_

#include <eixx/namespace.hpp>  // definition of EIXX_NAMESPACE
#include <eixx/marshal/defaults.hpp>
#include <eixx/marshal/eterm.hpp>

namespace EIXX_NAMESPACE {

typedef marshal::eterm<allocator_t>                  eterm;
typedef marshal::atom                                atom;
typedef marshal::string<allocator_t>                 string;
typedef marshal::binary<allocator_t>                 binary;
typedef marshal::epid<allocator_t>                   epid;
typedef marshal::port<allocator_t>                   port;
typedef marshal::ref<allocator_t>                    ref;
typedef marshal::tuple<allocator_t>                  tuple;
typedef marshal::list<allocator_t>                   list;
typedef marshal::trace<allocator_t>                  trace;
typedef marshal::var                                 var;
typedef marshal::varbind<allocator_t>                varbind;
typedef marshal::eterm_pattern_matcher<allocator_t>  eterm_pattern_matcher;
typedef marshal::eterm_pattern_action<allocator_t>   eterm_pattern_action;

namespace detail {
    BOOST_STATIC_ASSERT(sizeof(eterm)     == 2*sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(atom)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(string)    <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(binary)    <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(epid)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(port)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(ref)       <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(tuple)     <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(list)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(trace)     <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(var)       == sizeof(uint64_t));
} // namespace detail

} // namespace EIXX_NAMESPACE

#endif
