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
port<Alloc>::port(const char *buf, uintptr_t& idx, [[maybe_unused]] size_t size, const Alloc& a_alloc)
{
    const char* s   = buf + idx;
    const char* s0  = s;
    uint8_t     tag = get8(s);
    if (tag != ERL_PORT_EXT
#ifdef ERL_NEW_PORT_EXT
        && tag != ERL_NEW_PORT_EXT
#endif
#ifdef ERL_V4_PORT_EXT
        && tag != ERL_V4_PORT_EXT
#endif
        )
        throw err_decode_exception("Error decoding port's type", idx, tag);

    const uint8_t atom_tag = get8(s);
    long l = atom::get_len(s, atom_tag);
    if (l < 0)
        throw err_decode_exception("Error decoding port's node", idx, l);
    size_t len = static_cast<size_t>(l);
    detail::check_node_length(len);
    atom l_node(s, len);
    s += len;

    uint64_t id;
    uint32_t cre;

    switch (tag) {
#ifdef ERL_V4_PORT_EXT
        case ERL_V4_PORT_EXT:
            id  = get64be(s);
            cre = get32be(s);
            break;
#endif
#ifdef ERL_NEW_PORT_EXT
        case ERL_NEW_PORT_EXT:
            id  = static_cast<uint64_t>(get32be(s));
            cre = get32be(s);
            break;
#endif
        case ERL_PORT_EXT:
            id  = static_cast<uint64_t>(get32be(s) & 0x0fffffff);  /* 28 bits */
            cre = static_cast<uint32_t>(get8(s) & 0x03);           /*  2 bits */
            break;
        
        default:
            throw err_decode_exception("Error decoding port's type", idx, tag);
    }
    init(l_node, id, cre, a_alloc);

    idx += static_cast<uintptr_t>(s - s0);
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void port<Alloc>::encode(char* buf, uintptr_t& idx, [[maybe_unused]] size_t size) const
{
    char* s  = buf + idx;
    char* s0 = s;
    s++; // Skip ERL_PORT_EXT

    /* the nodename */
#ifdef ERL_ATOM_UTF8_EXT
    put8(s, ERL_ATOM_UTF8_EXT);
#else
    put8(s, ERL_ATOM_EXT);
#endif
    atom nd = node();
    uint16_t n = nd.size();
    put16be(s, n);
    memmove(s, nd.c_str(), n);
    s += n;

    /* the integers */
    const uint32_t l_cre = creation();
#if defined(ERL_V4_PORT_EXT)
    if (id() > 0x0fffffff /* 28 bits */) {
        *s0 = ERL_V4_PORT_EXT;
        put64be(s, id());
        put32be(s, l_cre);
    } else {
#elif defined(ERL_NEW_PORT_EXT)
        *s0 = ERL_NEW_PORT_EXT;
        put32be(s, id() & 0x0fffffff /* 28 bits */);  
        put32be(s, l_cre);
#else
        *s0 = ERL_PORT_EXT;
        put32be(s, id() & 0x0fffffff /* 28 bits */);
        put8(s, l_cre & 0x03 /* 2 bits */);
#endif
#if defined(ERL_V4_PORT_EXT)
    }
#endif

    idx += static_cast<uintptr_t>(s - s0);
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace eixx
