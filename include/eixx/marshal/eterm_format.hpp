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

#ifndef _EI_ETERM_FORMAT_HPP_
#define _EI_ETERM_FORMAT_HPP_

#include <eixx/marshal/defaults.hpp>

namespace EIXX_NAMESPACE {
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
} // namespace EIXX_NAMESPACE

#include <eixx/marshal/eterm_format.ipp>

#endif // _EI_ETERM_FORMAT_HPP_

