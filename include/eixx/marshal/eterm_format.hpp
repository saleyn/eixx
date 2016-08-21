//----------------------------------------------------------------------------
/// \file  eterm_format.hpp
//----------------------------------------------------------------------------
/// \brief A wrapper class implementing encoding of an eterm from string.
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

#ifndef _EI_ETERM_FORMAT_HPP_
#define _EI_ETERM_FORMAT_HPP_

#include <eixx/marshal/defaults.hpp>

namespace eixx {
namespace marshal {

/**
 * The format letters are:
 * <code>
 *   a  -  An atom
 *   s  -  A string
 *   i  -  An integer
 *   l  -  A long integer
 *   u  -  An unsigned long integer
 *   f  -  A double float
 *   w  -  A pointer to some arbitrary term
 * </code>
 * @return zero ref counted eterm.
 * @throws err_encode_exception
 */

// Forward declaration
template <class Alloc>
static eterm<Alloc> eformat(const char** fmt, va_list* args, const Alloc& a_alloc = Alloc())
    throw(err_format_exception);

/**
 * Parse a format string in the form "Module:Function(Args...)" into corresponding
 * \a mod, \a fun, \a args
 */
template <class Alloc>
static void eformat(atom& mod, atom& fun, eterm<Alloc>& args,
                    const char** fmt, va_list* pa, const Alloc& a_alloc = Alloc());

} // namespace marshal
} // namespace eixx

#include <eixx/marshal/eterm_format.hxx>

#endif // _EI_ETERM_FORMAT_HPP_

