//----------------------------------------------------------------------------
/// \file  eixx.hpp
//----------------------------------------------------------------------------
/// \brief Main header that includes functionality of eixx. It must be 
///        preceeded by inclusion of one of the allocator headers
///        such as <eixx/alloc_std.hpp>.
/// The library is largely inspired by the
/// <a href="http://code.google.com/p/epi/">epi</a> project, but is a complete
/// rewrite that includes many enhancements and optimizations.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) Library.

Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>

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
/**
 * \namespace ei contains a set of marshaling and connectivity primitives.
 * These primitives are built on top of ei library included in
 * <a href="http://www.erlang.org/doc/apps/erl_interface">erl_interface</a>.
 * This library adds on the following features:
 *   - Use of reference-counted smart pointers.
 *   - Ability to provice custom memory allocators.
 *   - Encoding/decoding of nested terms using a single function call
 *     (eterm::encode() and eterm::eterm() constructor).
 * The library consists of two separate parts:
 *   - Term marshaling (included by marshal.hpp or eixx.hpp)
 *   - Distributed node connectivity (included by connect.hpp or eixx.hpp)
 *   .
 */
#ifndef _EIXX_HPP_
#define _EIXX_HPP_

//-----------------------------------------------------------------------------
// !!! Prior to including this header make sure to define eixx::allocator_t.
// !!! Sample implementations are provided in
// !!! eixx/alloc_std.hpp      - uses std::allocator<char>
// !!! eixx/alloc_pool.hpp     - uses boost::pool_allocator<char>
// !!! eixx/alloc_pool_st.hpp  - same as previous, for single-threaded cases.
//-----------------------------------------------------------------------------
#ifndef EIXX_USE_ALLOCATOR
#    error Allocator not defined - include one of eixx/alloc*.hpp headers!
#endif

#include <eixx/eterm.hpp>
#include <eixx/connect.hpp>

#endif // _EIXX_HPP_

