/*
***** BEGIN LICENSE BLOCK *****

This file is part of the EPI (Erlang Plus Interface) Library.

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
#ifndef _EIXX_HASHTABLE_HPP_
#define _EIXX_HASHTABLE_HPP_

#include <eixx/util/sync.hpp>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#include <unordered_map>
#else
#include <boost/unordered_map.hpp>
#endif

namespace EIXX_NAMESPACE {
namespace detail {

namespace src = 
    #if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
    std;
    #else
    boost;
    #endif

/// Hash map class
template <typename K, typename V, typename Hash>
struct hash_map_base : public src::unordered_map<K, V, Hash>
{
    hash_map_base() {}
    hash_map_base(size_t n): src::unordered_map<K,V,Hash>(n)
    {}
    hash_map_base(size_t n, const Hash& h): src::unordered_map<K,V,Hash>(n, h)
    {}
};

/// Hash functor implemention hsieh hashing algorithm
// See http://www.azillionmonkeys.com/qed/hash.html
// Copyright 2004-2008 (c) by Paul Hsieh 
struct hsieh_hash_fun {
    static uint16_t get16bits(const char* d) { return *(const uint16_t *)d; }

    size_t operator()(const char* data) const {
        int len = strlen(data);
        uint32_t hash = len, tmp;
        int rem;

        if (len <= 0 || data == NULL) return 0;

        rem = len & 3;
        len >>= 2;

        /* Main loop */
        for (;len > 0; len--) {
            hash  += get16bits (data);
            tmp    = (get16bits (data+2) << 11) ^ hash;
            hash   = (hash << 16) ^ tmp;
            data  += 2*sizeof (uint16_t);
            hash  += hash >> 11;
        }

        /* Handle end cases */
        switch (rem) {
            case 3: hash += get16bits (data);
                    hash ^= hash << 16;
                    hash ^= data[sizeof (uint16_t)] << 18;
                    hash += hash >> 11;
                    break;
            case 2: hash += get16bits (data);
                    hash ^= hash << 11;
                    hash += hash >> 17;
                    break;
            case 1: hash += *data;
                    hash ^= hash << 10;
                    hash += hash >> 1;
        }

        /* Force "avalanching" of final 127 bits */
        hash ^= hash << 3;
        hash += hash >> 5;
        hash ^= hash << 4;
        hash += hash >> 17;
        hash ^= hash << 25;
        hash += hash >> 6;

        return hash;
    }
};

} // namespace detail

typedef detail::hash_map_base<const char*, size_t, detail::hsieh_hash_fun> char_int_hash_map;

} // namespace EIXX_NAMESPACE

#endif // _EIXX_HASHTABLE_HPP_

