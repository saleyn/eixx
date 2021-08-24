//----------------------------------------------------------------------------
/// \file  binary.hxx
//----------------------------------------------------------------------------
/// \brief Implementation of binary class member functions.
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

#include <memory>
#include <eixx/marshal/endian.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {

template <class Alloc>
binary<Alloc>::binary(const char* buf, uintptr_t& idx, size_t size, const Alloc& a_alloc)
{
    const char* s  = buf + idx;
    const char* s0 = s;

    if (get8(s) != ERL_BINARY_EXT)
        throw err_decode_exception("Error decoding binary", idx);

    uint32_t sz = get32be(s);
    m_blob = new blob<char, Alloc>(sz, a_alloc);
    ::memcpy(m_blob->data(),s,sz);

    idx += s + sz - s0;
    BOOST_ASSERT((size_t)idx <= size);
}

template <class Alloc>
void binary<Alloc>::encode(char* buf, uintptr_t& idx, size_t size) const
{
    char* s  = buf + idx;
    char* s0 = s;
    put8(s,ERL_BINARY_EXT);
    size_t sz = this->size();
    if (sz > UINT32_MAX)
        throw err_encode_exception("BINARY_EXT length exceeds maximum");
    uint32_t len = (uint32_t)sz;
    put32be(s, len);
    memmove(s, this->data(), len);
    s += len;
    idx += s-s0;
    BOOST_ASSERT((size_t)idx <= size);
}

} // namespace marshal
} // namespace eixx
