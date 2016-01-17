//----------------------------------------------------------------------------
/// \file  binary.ipp
//----------------------------------------------------------------------------
/// \brief Implementation of binary class member functions.
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

#include <memory>
#include <eixx/marshal/endian.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {

template <class Alloc>
binary<Alloc>::binary(const char* buf, int& idx, size_t size, const Alloc& a_alloc)
    throw (err_decode_exception)
{
    const char* s  = buf + idx;
    const char* s0 = s;

    if (get8(s) != ERL_BINARY_EXT)
        throw err_decode_exception("Error decoding binary", idx);

    size_t sz = get32be(s);
    m_blob = new blob<char, Alloc>(sz, a_alloc);
    ::memcpy(m_blob->data(),s,sz);

    idx += s + sz - s0;
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void binary<Alloc>::encode(char* buf, int& idx, size_t size) const
{
    char* s  = buf + idx;
    char* s0 = s;
    put8(s,ERL_BINARY_EXT);
    size_t n = this->size();
    put32be(s, n);
    memmove(s, this->data(), n);
    s += n;
    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace eixx
