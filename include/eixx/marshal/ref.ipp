//----------------------------------------------------------------------------
/// \file  ref.ipp
//----------------------------------------------------------------------------
/// \brief Implementation of ref member functions.
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

#include <iostream>
#include <sstream>
#include <memory>
#include <eixx/marshal/endian.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

template <class Alloc>
ref<Alloc>::ref(const char* buf, int& idx, size_t size, const Alloc& a_alloc)
    throw(err_decode_exception)
{
    const char *s = buf + idx;
    const char *s0 = s;
    int type = get8(s);

    switch (type) {
        case ERL_NEW_REFERENCE_EXT: {
            int count = get16be(s);
            if (count != COUNT)
                throw err_decode_exception("Error decoding ref's count", idx+1);
            if (get8(s) != ERL_ATOM_EXT)
                throw err_decode_exception("Error decoding ref's atom", idx+3);

            int len = get16be(s);
            detail::check_node_length(len);
            atom l_node(s, len);
            s += len;

            uint8_t  l_creation = get8(s) & 0x03;
            uint32_t l_ids[COUNT];

            for (size_t i = 0; i < COUNT; i++)
                l_ids[i] = get32be(s);

            init(l_node, l_ids, l_creation, a_alloc);

            idx += s-s0;
            break;
        }
        default:
            throw err_decode_exception("Error decoding ref's type", type);
    }
}

template <class Alloc>
void ref<Alloc>::encode(char* buf, int& idx, size_t size) const
{
    char* s  = buf + idx;
    char* s0 = s;
    put8(s,ERL_NEW_REFERENCE_EXT);
    /* first, number of integers */
    put16be(s, COUNT);
    /* then the nodename */
    put8(s,ERL_ATOM_EXT);
    const atom::string_t& str = node().to_string();
    unsigned short n = str.size();
    put16be(s, n);
    memmove(s, str.c_str(), n);
    s += n;

    /* now the integers */
    put8(s, (creation() & 0x03)); /* 2 bits */
    for (size_t i=0; i < COUNT; ++i)
        put32be(s, ids()[i]);

    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace EIXX_NAMESPACE
