//----------------------------------------------------------------------------
/// \file  visit_encoder.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing binary term encoder visitor.
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
#ifndef _IMPL_VISIT_ENCODER_HPP_
#define _IMPL_VISIT_ENCODER_HPP_

#include <eixx/marshal/visit.hpp>

namespace eixx {
namespace marshal {

class visit_eterm_encoder: public static_visitor<visit_eterm_encoder, void> {
    mutable char* buf;
    int&  idx;
    const size_t  size;
public: 
    visit_eterm_encoder(char* a_buf, int& a_idx, size_t a_size)
        : buf(a_buf), idx(a_idx), size(a_size)
    {}

    void operator() (bool   a) const { ei_encode_boolean (buf, &idx, a); }
    void operator() (long   a) const { ei_encode_longlong(buf, &idx, a); }
    void operator() (double a) const { ei_encode_double  (buf, &idx, a); }

    template <typename T>
    void operator()(const T& a) const { a.encode(buf, idx, size); }
};

} // namespace marshal
} // namespace eixx

#endif // _IMPL_VISIT_ENCODER_HPP_
