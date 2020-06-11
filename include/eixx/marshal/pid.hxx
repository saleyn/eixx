//----------------------------------------------------------------------------
/// \file  pid.hxx
//----------------------------------------------------------------------------
/// \brief Implementation of the epid member functions.
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

#include <eixx/marshal/endian.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {

template <class Alloc>
void epid<Alloc>::decode(const char *buf, int& idx, size_t size, const Alloc& alloc)
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
} // namespace eixx
