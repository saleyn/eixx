//----------------------------------------------------------------------------
/// \file  pid.ipp
//----------------------------------------------------------------------------
/// \brief Implementation of the epid member functions.
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

#include <eixx/marshal/endian.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <class Alloc>
void epid<Alloc>::decode(const char *buf, int& idx, size_t size, const Alloc& alloc)
    throw(err_decode_exception, err_bad_argument)
{
    const char* s  = buf + idx;
    const char* s0 = s;
    if (get8(s) != ERL_PID_EXT || get8(s) != ERL_ATOM_EXT)
        throw err_decode_exception("Error decoding pid", -1);

    int len = get16be(s);
    detail::check_node_length(len);

    atom l_node(s, len);
    s += len;

    uint64_t i1 = get32be(s);
    uint64_t i2 = get32be(s);
    uint64_t l_id  = i1 | i2 << 15;
    int l_creation = get8(s);

    init(l_node, l_id, l_creation, alloc);

    idx += s - s0;
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void epid<Alloc>::encode(char* buf, int& idx, size_t size) const
{
    char* s  = buf + idx;
    char* s0 = s;
    put8(s,ERL_PID_EXT);
    put8(s,ERL_ATOM_EXT);
    const std::string& nd = node().to_string();
    unsigned short n = nd.size();
    put16be(s, n);
    memmove(s, nd.c_str(), n);
    s += n;

    /* now the integers */
    put32be(s, id() & 0x7fff); /* 15 bits */
    put32be(s, serial() & 0x1fff); /* 13 bits */
    put8(s,   (creation() & 0x03)); /* 2 bits */

    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace EIXX_NAMESPACE
