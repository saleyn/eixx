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

