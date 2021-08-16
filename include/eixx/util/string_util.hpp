//----------------------------------------------------------------------------
/// \file  string.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an string object of Erlang external 
///        term format.
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
#ifndef _EIXX_STRING_UTIL_HPP_
#define _EIXX_STRING_UTIL_HPP_

#include <ostream>
#include <sstream>
#include <string>
#include <eixx/marshal/defaults.hpp>

namespace eixx {

/// Print the content of a buffer to \a out stream in the form:
/// \verbatim <<I1, I2, ..., In>> \endverbatim where <tt>Ik</tt> is
/// unsigned integer less than 256.
template <typename Stream>
inline Stream& to_binary_string(Stream& out, const char* buf, size_t sz) {
    out << "<<";
    const char* begin = buf, *end = buf + sz;
    for(const char* p = begin; p != end; ++p) {
        out << (p == begin ? "" : ",") << (int)*(unsigned char*)p;
    }
    out << ">>";
    return out;
}

/// Convert the content of a buffer to a binary string in the form:
/// \verbatim <<I1, I2, ..., In>> \endverbatim where <tt>Ik</tt> is
/// unsigned integer less than 256.
inline std::string to_binary_string(const char* a, size_t sz) {
    std::stringstream oss;
    to_binary_string(oss, a, sz);
    return oss.str();
}

/// Convert string to integer
///
/// @tparam TillEOL instructs that the integer must be validated till a_end.
///                 If false, "123ABC" is considered a valid 123 number. Otherwise
///                 the function will return NULL.
/// @return input string ptr beyond the the value read if successful, NULL otherwise
//
template <typename T, bool TillEOL = true>
inline const char* fast_atoi(const char* a_str, const char* a_end, T& res) {
    if (a_str >= a_end) return nullptr;

    bool l_neg;

    if (*a_str == '-') { l_neg = true; ++a_str; }
    else               { l_neg = false; }

    T x = 0;

    do {
        const int c = *a_str - '0';
        if (c < 0 || c > 9) {
            if (TillEOL)
               return nullptr;
            break;
        }
        x = (x << 3) + (x << 1) + c;
    } while (++a_str != a_end);

    res = l_neg ? -x : x;
    return a_str;
}

} // namespace eixx

namespace std {

    template <int N>
    std::string to_string(const uint8_t (&s)[N]) { return std::string((const char*)s, N); }

}

#endif // _EIXX_STRING_UTIL_HPP_
