//----------------------------------------------------------------------------
/// \file  port.hxx
//----------------------------------------------------------------------------
/// \brief Implemention of port member functions.
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
} // namespace eixx
