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

This file is part of the eixx (Erlang C++ Interface) Library.

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
#ifndef _EIXX_STRING_HPP_
#define _EIXX_STRING_HPP_

#include <string.h>
#include <eixx/marshal/defaults.hpp>
#include <eixx/util/string_util.hpp>
#include <eixx/marshal/alloc_base.hpp>
#include <eixx/marshal/endian.hpp>
#include <eixx/eterm_exception.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <class Alloc> class varbind;

template <class Alloc>
class string
{
protected:
    blob<char, Alloc>* m_blob;

    void release() {
        if (m_blob)
            m_blob->release();
    }

public:
    typedef const char* const_iterator;

    string() : m_blob(nullptr) {}

    string(const char* s, const Alloc& a = Alloc()) {
        BOOST_ASSERT(s);
        if (!s[0]) {
            m_blob = nullptr;
            return;
        }
        m_blob = new blob<char, Alloc>(strlen(s)+1, a);
        memcpy(m_blob->data(), s, m_blob->size()-1);
        m_blob->data()[m_blob->size()-1] = '\0';
    }
    string(const std::string& s, const Alloc& a = Alloc()) {
        if (s.empty()) {
            m_blob = nullptr;
            return;
        }
        m_blob = new blob<char, Alloc>(s.size()+1, a);
        memcpy(m_blob->data(), s.c_str(), m_blob->size());
        m_blob->data()[m_blob->size()-1] = '\0';
    }
    string(const char* s, size_t n, const Alloc& a = Alloc()) {
        if (n == 0) {
            m_blob = nullptr;
            return;
        }
        m_blob = new blob<char, Alloc>(n+1, a);
        if (s != NULL) {
            memcpy(m_blob->data(), s, m_blob->size());
            m_blob->data()[m_blob->size()-1] = '\0';
        } else
            m_blob->data()[0] = '\0';
    }
    string(const string<Alloc>& s) : m_blob(s.m_blob) {
        if (m_blob) m_blob->inc_rc();
    }

    string(string<Alloc>&& s) : m_blob(s.m_blob) {
        s.m_blob = nullptr;
    }

    string(const char* buf, int& idx, size_t size, const Alloc& a_alloc = Alloc())
        throw(err_decode_exception);

    ~string() {
        release();
    }

    string<Alloc>& operator= (const string<Alloc>& s) {
        if (this != &s) {
            release();
            m_blob = s.m_blob;
            if (m_blob) m_blob->inc_rc();
        }
        return *this;
    }

    string<Alloc>& operator= (string<Alloc>&& s) {
        if (this != &s) {
            release();
            m_blob = s.m_blob;
            s.m_blob = nullptr;
        }
        return *this;
    }

    void operator= (const std::string& s) {
        if (!m_blob)
            new (this) string<Alloc>(s);
        else {
            string<Alloc> str(s, m_blob->get_allocator());
            *this = str;
        }
    }

    const_iterator begin() const { return m_blob ? c_str() : NULL; }
    const_iterator end()   const { return m_blob ? c_str()+size() : NULL; }

    const char* c_str()  const { return m_blob ? m_blob->data() : ""; }
    size_t      size()   const { return m_blob ? m_blob->size()-1 : 0; }
    std::string to_str() const { return m_blob ? std::string(m_blob->data(), m_blob->size()-1) : ""; }
    size_t      length() const { return size(); }
    bool        empty()  const { return c_str()[0] == '\0'; }

    void        clear()        { release(); m_blob = NULL; }

    // Use only for debugging
    int         use_count() const { return m_blob ? m_blob->use_count() : -1000000; }

    bool operator== (const char* rhs) const {
        return strcmp(c_str(), rhs) == 0;
    }
    bool operator== (const string<Alloc>& rhs) const {
        return size() == rhs.size() && strcmp(c_str(), rhs.c_str()) == 0;
    }
    bool operator<  (const string<Alloc>& rhs) const {
        return strcmp(c_str(), rhs.c_str()) < 0;
    }

    /// Tests if this string is equal to the content of the binary buffer \a rhs.
    template <int N>
    bool equal(const char (&rhs)[N]) const {
        int i = N > 0 && rhs[N-1] == '\0' ? -1 : 0;
        return strncmp(c_str(), rhs, size()+i) == 0;
    }

    /// Tests if this string is equal to the content of the binary buffer \a rhs.
    template <int N>
    bool equal(const uint8_t (&rhs)[N]) const {
        int i = N > 0 && rhs[N-1] == '\0' ? -1 : 0;
        return strncmp(c_str(), (const char*)rhs, size()+i) == 0;
    }

    /** Size of binary buffer needed to hold the encoded string. */
    size_t encode_size() const {
        size_t n = this->length();
        return n == 0 ? 1 : n + (n <= 0xffff ? 3 : 5+n+1);
    }

    void encode(char* buf, int& idx, size_t size) const {
        ei_encode_string_len(buf, &idx, c_str(), length());
    }

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding=NULL) const {
        return out << *this;
    }
};

template <typename Alloc>
static std::string to_binary_string(const string<Alloc>& a) {
    return to_binary_string(a.c_str(), a.size());
}

template <class Alloc>
string<Alloc>::string(const char* buf, int& idx, size_t size, const Alloc& a_alloc)
    throw(err_decode_exception)
{
    const char *s = buf + idx;
    const char *s0 = s;
    int etype = get8(s);

    switch (etype) {
        case ERL_STRING_EXT: {
            int len = get16be(s);
            if (len == 0)
                m_blob = NULL;
            else {
                m_blob = new blob<char, Alloc>(len+1, a_alloc);
                memcpy(m_blob->data(), s, len);
                m_blob->data()[len] = '\0';
                s += len;
            }
            break;
        }
        case ERL_LIST_EXT: {
            /* Really long strings are represented as lists of small integers.
             * We don't know in advance if the whole list is small integers,
             * but we decode as much as we can, exiting early if we run into a
             * non-character in the list.
             */
            int len = get32be(s);
            if (len == 0)
                m_blob = NULL;
            else {
                m_blob = new blob<char, Alloc>(len+1, a_alloc);
                for (int i=0; i<len; i++) {
                    if ((etype = get8(s)) != ERL_SMALL_INTEGER_EXT)
                        throw err_decode_exception("Error decoding string", s+i-s0);
                    m_blob->data()[i] = get8(s);
                }
                m_blob->data()[len] = '\0';
            }
            break;
        }
        case ERL_NIL_EXT:
            m_blob = NULL;
            break;

        default:
            throw err_decode_exception("Error decoding string type", etype);
    }
    idx += s-s0;
}

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {

    template <typename Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::string<Alloc>& s) {
        return out << '"' << s.c_str() << '"';
    }

    template <typename Alloc>
    bool operator== (const std::string& lhs, const EIXX_NAMESPACE::marshal::string<Alloc>& rhs) {
        return lhs == rhs.c_str();
    }

    template <typename Alloc>
    bool operator== (const EIXX_NAMESPACE::marshal::string<Alloc>& lhs, const std::string& rhs) {
        return rhs == rhs.c_str();
    }

    template <typename Alloc>
    bool operator== (const EIXX_NAMESPACE::marshal::string<Alloc>& lhs, const char* rhs) {
        return strcmp(rhs, lhs.c_str(), lhs.size()) == 0;
    }

}

#endif // _EIXX_STRING_HPP_
