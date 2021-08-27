//----------------------------------------------------------------------------
/// \file  atom.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an atom - enumerated string stored 
///        stored in non-garbage collected hash table of fixed size.
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
#ifndef _EIXX_ATOM_HPP_
#define _EIXX_ATOM_HPP_

#include <string>
#include <vector>
#include <boost/assert.hpp>
#include <eixx/marshal/defaults.hpp>
#include <eixx/marshal/endian.hpp>
#include <eixx/marshal/string.hpp>
#include <eixx/eterm_exception.hpp>
#include <eixx/util/hashtable.hpp>
#include <eixx/util/atom_table.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {
namespace detail {

    namespace eid = eixx::detail;
    using eid::lock_guard;

    /// Create an atom containing node name.
    inline void check_node_length(size_t len) {
        if (len > MAXNODELEN) {
            throw err_bad_argument("Node name too long!", len);
        } else if (len == 0) {
            throw err_bad_argument("Empty node name");
        }
    }

} // namespace detail

/**
 * Provides a representation of Erlang atoms. Atoms can be
 * created from UTF8 strings whose length is not more than
 * MAXATOMLEN characters / codepoints.
 */
class atom
{
    uint32_t m_index;

    atom(uint32_t idx) : m_index(idx) { assert(idx >= 0); }
public:
    inline static util::atom_table& atom_table() {
       static util::atom_table s_atom_table;
       return s_atom_table;
    }

    /// Returns empty atom
    inline static const atom null() {
       static const atom null = atom();
       return null;
    }

    /// Create an empty atom
    atom() : m_index(0) {
        static_assert(sizeof(atom) == 4, "Invalid atom size!");
    }

    /// Create an atom from the given string.
    /// @param atom the string to create the atom from.
    /// @throw std::runtime_error if atom table is full.
    /// @throw err_bad_argument if atom length is longer than MAXATOMLEN
    atom(const char* s)
        : m_index(atom_table().lookup(std::string(s))) {}

    /// @copydoc atom::atom
    template <size_t N>
    atom(const char (&s)[N])
        : m_index(atom_table().lookup(std::string(s, N))) {}

    /// @copydoc atom::atom
    explicit atom(const std::string& s)
        : m_index(atom_table().lookup(s)) {}

    /// Try to create an atom without throwing exceptions
    /// NOTE: if the atom name is invalid or it doesn't exist and \a existing
    ///       is true, then an empty atom is returned.
    static atom create(const std::string& s, bool existing) {
        auto   idx = atom_table().try_lookup(s);
        return idx <= UINT32_MAX && existing ? atom((uint32_t)idx) : atom();
    }
    /// @copydoc atom::create
    static atom create(const char* s, bool existing) {
        return create(std::string(s), existing);
    }

    /// Get atom length from a binary buffer encoded in 
    /// Erlang external binary format.
    static long get_len(const char*& s, const uint8_t tag) {
        switch (tag) {
#ifdef ERL_SMALL_ATOM_UTF8_EXT
            case ERL_SMALL_ATOM_UTF8_EXT: return get8(s);
#endif
#ifdef ERL_ATOM_UTF8_EXT
            case ERL_ATOM_UTF8_EXT:       return get16be(s);
#endif
#ifdef ERL_SMALL_ATOM_EXT
            case ERL_SMALL_ATOM_EXT:      return get8(s);
#endif
            case ERL_ATOM_EXT:            return get16be(s);
            default:                      return -1;
        }
    }

    /// @copydoc atom::atom
    /// @param existing    if true, check that the atom already exists, otherwise throw
    ///                    err_atom_not_found
    atom(const char* s, bool existing)
        : atom(std::string(s), existing)
    {}
    atom(const std::string& s, bool existing)
    {
        if (!existing) {
            m_index = atom_table().lookup(s);
            return;
        }
        auto idx = atom_table().try_lookup(s);
        if (idx <= UINT32_MAX)
            m_index = (uint32_t)idx;
        else
            throw err_atom_not_found(s);
    }

    /// @copydoc atom::atom
    template<typename Alloc>
    explicit atom(const string<Alloc>& s)
        : m_index(atom_table().lookup(std::string(s.c_str(), s.size())))
    {}

    /// @copydoc atom::atom
    atom(const char* s, int    n) : atom(s, static_cast<size_t>(n)) {}
    /// @copydoc atom::atom
    atom(const char* s, long   n) : atom(s, static_cast<size_t>(n)) {}
    /// @copydoc atom::atom
    atom(const char* s, size_t n)
        : m_index(atom_table().lookup(std::string(s, n)))
    {}

    /// Copy atom from another atom.  This is a constant time 
    /// SMP safe operation.
    atom(const atom& s) throw() : m_index(s.m_index) {}

    /// Decode an atom from a binary buffer encoded in 
    /// Erlang external binary format.
    atom(const char* a_buf, uintptr_t& idx, [[maybe_unused]] size_t a_size)
    {
        const char *s = a_buf + idx;
        const char *s0 = s;
        const uint8_t tag = get8(s);
        long len = get_len(s, tag);
        if (len < 0)
            throw err_decode_exception("Error decoding atom", idx);
        m_index = atom_table().lookup(std::string(s, static_cast<size_t>(len)));
        idx += static_cast<uintptr_t>(s - s0) + static_cast<size_t>(len);
        BOOST_ASSERT((size_t)idx <= a_size);
    }

    const char*         c_str()     const { return atom_table()[m_index].c_str();          }
    const std::string&  to_string() const { return atom_table()[m_index];                  }
    uint16_t            size()      const { return (uint16_t)atom_table()[m_index].size(); }
    uint16_t            length()    const { return size();                                 }
    bool                empty()     const { return m_index == 0;                           }

    /// Get atom's index in the atom table.
    uint32_t            index()     const { return m_index; }

    void operator=  (const atom& s)               { m_index = s.m_index; }
    void operator=  (const std::string& s)        { m_index = atom_table().lookup(s); }
    bool operator== (const char* rhs)       const { return strcmp(c_str(), rhs) == 0; }
    bool operator== (const atom& rhs)       const { return m_index == rhs.m_index; }
    bool operator!= (const char* rhs)       const { return !(*this == rhs); }
    bool operator!= (const atom& rhs)       const { return m_index != rhs.m_index; }
    bool operator<  (const atom& rhs)       const { return strcmp(c_str(), rhs.c_str()) < 0;  }

    /// Compare this atom to \a rhs.
    /// @return 0 if they are equal.
    ///       < 0 if this atom is less than \a rhs.
    ///       > 0 if this atom is greater than \a rhs.
    int  compare (const atom& rhs) const {
        return m_index == rhs.m_index ? 0 : strcmp(c_str(), rhs.c_str());
    }

    /// Get the size of a buffer needed to encode this atom in 
    /// the external binary format.
    size_t encode_size() const {
        const size_t sz = size();
        return (sz > 255 ? 3 : 2) + sz;
    }

    /// Encode the atom in external binary format.
    /// @param buf is the buffer space to encode the atom to.
    /// @param idx is the offset in the \a buf where to begin writing.
    /// @param size is the size of \a buf.
    void encode(char* buf, uintptr_t& idx, [[maybe_unused]] size_t a_size) const {
        char* s  = buf + idx;
        char* s0 = s;
        const uint16_t len = std::min((uint16_t)MAXATOMLEN_UTF8, size());
        /* This function is documented to truncate at MAXATOMLEN_UTF8 (1021) */
        if (len > UINT8_MAX) {
            put8(s, ERL_ATOM_UTF8_EXT);
            put16be(s, len);
        } else {
            put8(s, ERL_SMALL_ATOM_UTF8_EXT);
            put8(s, (uint8_t)len);
        }
        memmove(s, c_str(), len); /* unterminated string */
        s   += len;
        idx += static_cast<uintptr_t>(s - s0);
        BOOST_ASSERT((size_t)idx <= a_size);
    }

    /// Write the atom to the \a out stream.
    std::ostream& dump(std::ostream& out, ...) const {
        const char* s = c_str();
        if (this->empty() || s[0] < 'a' || s[0] > 'z' || strchr(s, ' ') != NULL)
            return out << "'" << s << "'";
        else
            return out << s;
    }
};

/// Create an atom containing node name.
/// @param s is the string representation of the node name that must be
///        in the form: \c Alivename@Hostname.
/// @return atom representing node name
/// @throw  std::runtime_error if atom table is full.
/// @throw  err_bad_argument if atom size is longer than MAXNODELEN
inline atom make_node_name(const std::string& s)
{
    if (!s.find('@')) throw err_bad_argument("Invalid node name", s);
    detail::check_node_length(s.size());
    return atom(s);
}


} // namespace marshal
} // namespace eixx

namespace std {

    inline ostream& operator<< (ostream& out, const eixx::marshal::atom& s) {
        return s.dump(out);
    }

}

#endif
