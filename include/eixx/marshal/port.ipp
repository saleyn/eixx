//----------------------------------------------------------------------------
/// \file  port.ipp
//----------------------------------------------------------------------------
/// \brief Implemention of port member functions.
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
port<Alloc>::port(const char *buf, int& idx, size_t size, const Alloc& a_alloc)
    throw(err_decode_exception)
{
    const char* s  = buf + idx;
    const char* s0 = s;
    if (get8(s) != ERL_PORT_EXT || get8(s) != ERL_ATOM_EXT)
        throw err_decode_exception("Error decoding port", -1);
    int len = get16be(s);
    detail::check_node_length(len);
    atom l_node(s, len);
    s += len;

    int     l_id  = get32be(s)  & 0x0fffffff;   /* 28 bits */
    uint8_t l_cre = get8(s)     & 0x03;         /* 2 bits */
    init(l_node, l_id, l_cre, a_alloc);

    idx += s - s0;
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void port<Alloc>::encode(char* buf, int& idx, size_t size) const
{
    char* s  = buf + idx;
    char* s0 = s;
    put8(s,ERL_PORT_EXT);
    put8(s,ERL_ATOM_EXT);
    const std::string& str = node().to_string();
    unsigned short n = str.size();
    put16be(s, n);
    memmove(s, str.c_str(), n);
    s += n;

    /* now the integers */
    put32be(s, id() & 0x0fffffff);  /* 28 bits */
    put8(s,   (creation() & 0x03)); /* 2 bits */

    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace EIXX_NAMESPACE
