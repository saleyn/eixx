//----------------------------------------------------------------------------
/// \file  array_variadic_init.hpp
//----------------------------------------------------------------------------
/// \brief Copies variadic parameters to an array
//----------------------------------------------------------------------------
// Copyright (c) 2013 Serge Aleynikov <saleyn@gmail.com>
// Created: 2013-10-05
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) Library.

Copyright (C) 2013 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _EIXX_ARRAY_VARIADIC_INIT_HPP_
#define _EIXX_ARRAY_VARIADIC_INIT_HPP_

#include <eixx/marshal/eterm.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {
namespace detail {

    namespace {
        template <std::size_t I, typename Alloc, typename Head, typename... Tail>
        struct init_array {
            static void set(eterm<Alloc>* a_array, const Head& v, const Tail&... tail) {
                a_array[I] = eterm<Alloc>(v);
                init_array<I+1, Tail...>::set(a_array, tail...);
            }
        };

        template <std::size_t I, typename Alloc, typename Head>
        struct init_array<I, Alloc, Head> {
            static void set(eterm<Alloc>* a_array, const Head& v) {
                a_array[I] = eterm<Alloc>(v);
            }
        };
    }

    template <typename Alloc, typename... Args>
    void initialize(eterm<Alloc>* a_target, Args... args)
    {
        init_array<0, Args...>::set(a_target, args...);
    }

} // namespace detail
} // namespace marshal
} // namespace EIXX_NAMESPACE

#endif // _EIXX_ARRAY_VARIADIC_INIT_HPP_
