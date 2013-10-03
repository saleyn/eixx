//----------------------------------------------------------------------------
/// \file  binary.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing a binary object of Erlang external 
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
#ifndef _IMPL_BINARY_HPP_
#define _IMPL_BINARY_HPP_

#include <eixx/marshal/defaults.hpp>
#include <eixx/util/string_util.hpp>
#include <eixx/marshal/alloc_base.hpp>
#include <eixx/marshal/string.hpp>
#include <eixx/marshal/varbind.hpp>
#include <eixx/eterm_exception.hpp>
#include <string.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <class Alloc>
class binary
{
    blob<char, Alloc>* m_blob;

    void decode(const char* buf, int& idx, size_t size) throw(err_decode_exception);

public:
    binary() : m_blob(nullptr) {}

    /**
     * Create a binary from the given data.
     * Data is shared between all cloned binaries by using reference counting.
     * @param data pointer to data.
     * @param size binary size in bytes
     * @param a_alloc is the allocator to use
     **/
    binary(const char* data, size_t size, const Alloc& a_alloc = Alloc()) {
        if (size == 0) {
            m_blob = nullptr;
            return;
        }
        m_blob = new blob<char, Alloc>(size, a_alloc);
        memcpy(m_blob->data(), data, size);
    }

    binary(const binary<Alloc>& rhs) : m_blob(rhs.m_blob) {
        if (m_blob) m_blob->inc_rc();
    }

    binary(binary<Alloc>&& rhs) : m_blob(rhs.m_blob) { rhs.m_blob = nullptr; }

    binary(std::initializer_list<uint8_t> bytes, const Alloc& alloc = Alloc())
        : binary(reinterpret_cast<const char*>(bytes.begin()), bytes.size(), alloc) {}

    /**
     * Construct the object by decoding it from a binary
     * encoded buffer and using custom allocator.
     * @param a_alloc is the allocator to use.
     * @param buf is the buffer containing Erlang external binary format.
     * @param idx is the current offset in the buf buffer.
     * @param size is the size of \a buf buffer.
     */
    binary(const char* buf, int& idx, size_t size, const Alloc& a_alloc = Alloc())
        throw(err_decode_exception);

    /** Get the size of the data (in bytes) */
    size_t size() const { return m_blob ? m_blob->size() : 0; }

    /** Get the data's binary buffer */
    const char* data() const { return m_blob ? m_blob->data() : ""; }

    binary& operator= (const binary& rhs) {
        if (this != &rhs) {
            m_blob = rhs.m_blob;
            if (m_blob) m_blob->inc_rc();
        }
        return *this;
    }

    binary& operator= (binary&& rhs) {
        if (this != &rhs) {
            m_blob = rhs.m_blob;
            rhs.m_blob = nullptr;
        }
        return *this;
    }

    bool operator== (const binary<Alloc>& rhs) const {
        return size() == rhs.size() 
            && (size() == 0 || memcmp(data(), rhs.data(), size()) == 0);
    }
    bool operator< (const binary<Alloc>& rhs) {
        if (size() < rhs.size()) return true;
        if (size() > rhs.size()) return false;
        if (size() == 0)         return false;
        int res = memcmp(data(), rhs.data(), size());
        if (res < 0) return true;
        if (res > 0) return false;
        return false;
    }

    /** Encode binary to a flat buffer. */
    void encode(char* buf, int& idx, size_t size) const;

    /** Size of binary buffer needed to hold encoded binary. */
    size_t encode_size() const { return 5 + size(); }

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding=NULL) const {
        bool printable = size() > 1;
        for (const char* p = data(), *end = data() + size(); printable && p != end; ++p)
            if (*p < ' ' || *p > '~')
                printable = false;
        if (printable)
            return out << "<<" << string<Alloc>(data(), size()) << ">>";
        else
            return EIXX_NAMESPACE::to_binary_string(out, data(), size());
    }
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::binary<Alloc>& a) {
        return a.dump(out);
    }

} // namespace std

#include <eixx/marshal/binary.ipp>

#endif // _IMPL_BINARY_HPP_
