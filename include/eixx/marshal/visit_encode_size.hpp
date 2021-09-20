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
#ifndef _IMPL_VISIT_ENCODE_SIZE_HPP_
#define _IMPL_VISIT_ENCODE_SIZE_HPP_

#include <eixx/marshal/visit.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {

template <typename Alloc>
struct visit_eterm_encode_size_calc
    : public static_visitor<visit_eterm_encode_size_calc<Alloc>, size_t> {

    size_t operator()(bool   a) const { return 2 + (a ? 4 : 5); }
    size_t operator()(double  ) const { return 9; }
    size_t operator()(long   a) const { int n = 0; ei_encode_longlong(NULL, &n, a); return static_cast<size_t>(n); }

    template <typename T>
    size_t operator()(const T& a) const { return a.encode_size(); }
};

} // namespace eixx
} // namespace eixx

#endif // _IMPL_VISIT_ENCODE_SIZE_HPP_
