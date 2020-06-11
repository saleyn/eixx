//----------------------------------------------------------------------------
/// \file  ref.hxx
//----------------------------------------------------------------------------
/// \brief Implementation of ref member functions.
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

#include <iostream>
#include <sstream>
#include <memory>
#include <eixx/marshal/endian.hpp>

namespace eixx {
namespace marshal {

template <class Alloc>
ref<Alloc>::ref(const char* buf, int& idx, size_t size, const Alloc& a_alloc)
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
            uint32_t l_id0 = get32be(s);
            uint64_t l_id1 = get32be(s) | ((uint64_t)get32be(s) << 32);

            init(l_node, l_id0, l_id1, l_creation, a_alloc);

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
    const std::string& str = node().to_string();
    unsigned short n = str.size();
    put16be(s, n);
    memmove(s, str.c_str(), n);
    s += n;

    /* now the integers */
    if (m_blob) {
        put8(s,    m_blob->data()->u.s.creation); /* 2 bits */
        put32be(s, id0());
        put32be(s, id1() & 0xFFFF);
        put32be(s, (id1() >> 32) & 0xFFFF);
    } else {
        put8(s, 0); /* 2 bits */
        put32be(s, 0u);
        put32be(s, 0u);
        put32be(s, 0u);
    }

    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace eixx
