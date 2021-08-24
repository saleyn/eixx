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
#ifndef _EIXX_HASHTABLE_HPP_
#define _EIXX_HASHTABLE_HPP_

#include <eixx/util/sync.hpp>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#include <unordered_map>
#else
#include <boost/unordered_map.hpp>
#endif

namespace eixx {
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
        size_t len = strlen(data);
        BOOST_ASSERT(len <= UINT32_MAX);
        uint32_t hash = (uint32_t)len, tmp;
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

} // namespace eixx

#endif // _EIXX_HASHTABLE_HPP_

