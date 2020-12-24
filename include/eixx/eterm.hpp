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
#ifndef _EIXX_ETERM_HPP_
#define _EIXX_ETERM_HPP_

#include <eixx/config.h>
#include <eixx/marshal/defaults.hpp>
#include <eixx/marshal/eterm.hpp>

#define EIXX_DECL_ATOM(Atom)           static const eixx::atom am_##Atom(#Atom)
#define EIXX_DECL_ATOM_VAR(Name, Atom) static const eixx::atom Name(Atom)

namespace eixx {

typedef marshal::eterm<allocator_t>                  eterm;
typedef marshal::atom                                atom;
typedef marshal::string<allocator_t>                 string;
typedef marshal::binary<allocator_t>                 binary;
typedef marshal::epid<allocator_t>                   epid;
typedef marshal::port<allocator_t>                   port;
typedef marshal::ref<allocator_t>                    ref;
typedef marshal::tuple<allocator_t>                  tuple;
typedef marshal::list<allocator_t>                   list;
typedef marshal::map<allocator_t>                    map;
typedef marshal::trace<allocator_t>                  trace;
typedef marshal::var                                 var;
typedef marshal::varbind<allocator_t>                varbind;
typedef marshal::eterm_pattern_matcher<allocator_t>  eterm_pattern_matcher;
typedef marshal::eterm_pattern_action<allocator_t>   eterm_pattern_action;

namespace detail {
    BOOST_STATIC_ASSERT(sizeof(eterm)     == (ALIGNOF_UINT64_T > sizeof(int) ? ALIGNOF_UINT64_T : sizeof(int)) + sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(atom)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(string)    <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(binary)    <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(epid)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(port)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(ref)       <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(tuple)     <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(list)      <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(map)       <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(trace)     <= sizeof(uint64_t));
    BOOST_STATIC_ASSERT(sizeof(var)       == sizeof(uint64_t));
} // namespace detail

} // namespace eixx

#endif
