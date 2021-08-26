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
ref<Alloc>::ref(const char* buf, uintptr_t& idx, [[maybe_unused]] size_t size, const Alloc& a_alloc)
{
    const char *s  = buf + idx;
    const char *s0 = s;
    uint8_t    tag = get8(s);

    switch (tag) {
#ifdef ERL_NEWER_REFERENCE_EXT
        case ERL_NEWER_REFERENCE_EXT:
#endif
#ifdef ERL_NEW_REFERENCE_EXT
        case ERL_NEW_REFERENCE_EXT: {
            uint16_t count = get16be(s);  // First goes the count
            if (count > COUNT)
                throw err_decode_exception("Error decoding ref's count", idx+1, count);

            int len = atom::get_len(s);
            if (len < 0)
                throw err_decode_exception("Error decoding ref's atom", idx+3, len);
            detail::check_node_length(len);
            atom nd(s, len);
            s += len;

            uint32_t cre, mask;
            std::tie(cre, mask) = tag == ERL_NEW_REFERENCE_EXT
                                ? std::make_pair((get8(s) & 0x03u),    0x0003ffffu)  /* 18 bits */
                                : std::make_pair(uint32_t(get32be(s)), 0xFFFFffff);

            uint32_t vals[COUNT];
            for (auto p=vals, e=p+count; p != e; ++p)
                *p = get32be(s) & mask;

            init(nd, vals, count, cre, a_alloc);

            idx += s-s0;
            break;
        }
#endif
        case ERL_REFERENCE_EXT: {
            int len = atom::get_len(s);
            if (len < 0)
                throw err_decode_exception("Error decoding ref's atom", idx+3, len);
            detail::check_node_length(len);
            atom nd(s, len);
            s += len;

            uint32_t id  = get32be(s) & 0x0003ffff;  /* 18 bits */
            uint32_t cre = get8(s)    & 0x03;        /*  2 bits */

            init(nd, &id, 1u, cre, a_alloc);

            idx += s-s0;
            break;
        }
        default:
            throw err_decode_exception("Error decoding ref's type", idx, tag);
    }
}

template <class Alloc>
void ref<Alloc>::encode(char* buf, uintptr_t& idx, [[maybe_unused]] size_t size) const
{
    char* s  = buf + idx;
    char* s0 = s;
    s++; // Skip ERL_NEW_REFERENCE_EXT

    /* first, count of ids */
    put16be(s, len());

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
#ifdef ERL_NEWER_REFERENCE_EXT
    *s0 = ERL_NEWER_REFERENCE_EXT;
    put32be(s, l_cre);
    for (auto* p = ids(), *e = p + len(); p != e; ++p)
        put32be(s, *p);
#else
    *s0 = ERL_NEW_REFERENCE_EXT;
    put8(s, l_cre & 0x03 /* 2 bits */);
    for (auto* p = ids(), *e = p + len(); p != e; ++p)
        put32be(s, *p & 0x0003ffff /* 18 bits */);
#endif

    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace eixx
