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
        case ERL_NEW_REFERENCE_EXT:
        case ERL_NEWER_REFERENCE_EXT: {
            int count = get16be(s);  // First goes the count
            if (count < 0 || count > COUNT)
                throw err_decode_exception("Error decoding ref's count", idx+1, count);

            int len = atom::get_len(s);
            if (len < 0)
                throw err_decode_exception("Error decoding ref's atom", idx+3, len);
            detail::check_node_length(len);
            atom nd(s, len);
            s += len;

            uint32_t  cre = type == ERL_NEW_REFERENCE_EXT ? (get8(s) & 0x03)
                                                          : get32be(s);

            uint32_t vals[COUNT];
            for (auto p=vals, e=p+count; p != e; ++p)
                *p = uint32_t(get32be(s));

            init(nd, vals, count, cre, a_alloc);

            idx += s-s0;
            break;
        }
        case ERL_REFERENCE_EXT: {
            int len = atom::get_len(s);
            if (len < 0)
                throw err_decode_exception("Error decoding ref's atom", idx+3, len);
            detail::check_node_length(len);
            atom nd(s, len);
            s += len;

            uint32_t id  = get32be(s);
            uint32_t cre = get8(s) & 0x03;

            init(nd, &id, 1u, cre, a_alloc);

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
    put8(s,ERL_NEWER_REFERENCE_EXT);
    /* first, number of integers */
    put16be(s, len());
    /* then the nodename */
    put8(s,ERL_ATOM_UTF8_EXT);
    const std::string& str = node().to_string();
    unsigned short n = str.size();
    put16be(s, n);
    memmove(s, str.c_str(), n);
    s += n;

    /* now the integers */
    if (m_blob) {
        put32be(s, m_blob->data()->creation);
        for (auto* p = ids(), *e = p + len(); p != e; ++p)
            put32be(s, *p);
    }
    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace eixx
