//----------------------------------------------------------------------------
/// \file  endian.hpp
//----------------------------------------------------------------------------
/// \brief A wrapper around boost endian-handling functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

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

#ifndef _EIXX_ENDIAN_HPP_
#define _EIXX_ENDIAN_HPP_

#include <boost/version.hpp>
#include <boost/spirit/home/support/detail/endian.hpp>

namespace EIXX_NAMESPACE {

template <typename T>
inline void put_be(char*& s, T n) {
    #if BOOST_VERSION >= 104900
    boost::spirit::detail::store_big_endian<T, sizeof(T)>(s, n);
    #else
    boost::detail::store_big_endian<T, sizeof(T)>(s, n);
    #endif
    s += sizeof(T);
}

template <typename T>
inline T cast_be(const char* s) {
    #if BOOST_VERSION >= 104900
    return boost::spirit::detail::load_big_endian<T, sizeof(T)>(s);
    #else
    return boost::detail::load_big_endian<T, sizeof(T)>(s);
    #endif
}

template <typename T>
inline void get_be(const char*& s, T& n) {
    n = cast_be<T>(s);
    s += sizeof(T);
}

inline void put8   (char*& s, uint8_t n ) { put_be(s, n); }
inline void put16be(char*& s, uint16_t n) { put_be(s, n); }
inline void put32be(char*& s, uint32_t n) { put_be(s, n); }
inline void put64be(char*& s, uint64_t n) { put_be(s, n); }

inline uint8_t  get8   (const char*& s) { uint8_t  n; get_be(s, n); return n; }
inline uint16_t get16be(const char*& s) { uint16_t n; get_be(s, n); return n; }
inline uint32_t get32be(const char*& s) { uint32_t n; get_be(s, n); return n; }
inline uint64_t get64be(const char*& s) { uint64_t n; get_be(s, n); return n; }

} // namespace EIXX_NAMESPACE

#endif // _EIXX_ENDIAN_HPP_
