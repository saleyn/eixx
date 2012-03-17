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
#ifndef _IMPL_VISIT_ENCODER_HPP_
#define _IMPL_VISIT_ENCODER_HPP_

#include <eixx/marshal/visit.hpp>

namespace EIXX_NAMESPACE {
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
} // namespace EIXX_NAMESPACE

#endif // _IMPL_VISIT_ENCODER_HPP_
