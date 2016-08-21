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
#ifndef _EIXX_ARRAY_VARIADIC_INIT_HPP_
#define _EIXX_ARRAY_VARIADIC_INIT_HPP_

#include <eixx/marshal/eterm.hpp>

namespace eixx {
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
} // namespace eixx

#endif // _EIXX_ARRAY_VARIADIC_INIT_HPP_
