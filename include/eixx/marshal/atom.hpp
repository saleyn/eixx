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

namespace EIXX_NAMESPACE {
namespace marshal {
namespace detail {

    namespace eid = EIXX_NAMESPACE::detail;
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
 * created from strings whose length is not more than
 * MAXATOMLEN characters.
 */
class atom
{
    int m_index;

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
        BOOST_STATIC_ASSERT(sizeof(atom) == 4);
    }

    /// Create an atom from the given string.
    /// @param atom the string to create the atom from.
    /// @throws std::runtime_error if atom table is full.
    /// @throws err_bad_argument if atom size is longer than MAXATOMLEN
    atom(const char* s) throw(std::runtime_error, err_bad_argument)
        : m_index(atom_table().lookup(std::string(s))) {}

    /// @copydoc atom::atom
    template <int N>
    atom(const char (&s)[N]) throw(std::runtime_error, err_bad_argument)
        : m_index(atom_table().lookup(std::string(s, N))) {}

    /// @copydoc atom::atom
    explicit atom(const std::string& s) throw(std::runtime_error)
        : m_index(atom_table().lookup(s))
    {}

    /// @copydoc atom::atom
    template<typename Alloc>
    explicit atom(const string<Alloc>& s) throw(std::runtime_error)
        : m_index(atom_table().lookup(std::string(s.c_str(), s.size())))
    {}

    /// @copydoc atom::atom
    atom(const char* s, size_t n) throw(std::runtime_error)
        : m_index(atom_table().lookup(std::string(s, n)))
    {}

    /// Copy atom from another atom.  This is a constant time 
    /// SMP safe operation.
    atom(const atom& s) throw() : m_index(s.m_index) {}

    /// Decode an atom from a binary buffer encoded in 
    /// Erlang external binary format.
    atom(const char* a_buf, int& idx, size_t a_size)
        throw (err_decode_exception, std::runtime_error)
    {
        const char *s = a_buf + idx;
        const char *s0 = s;
        int len;
        switch (get8(s)) {
            case ERL_ATOM_EXT:       len = get16be(s); break;
            case ERL_SMALL_ATOM_EXT: len = get8(s); break;
            default: throw err_decode_exception("Error decoding atom", idx);
        }
        m_index = atom_table().lookup(std::string(s, len));
        idx += s + len - s0;
        BOOST_ASSERT((size_t)idx <= a_size);
    }

    const char*         c_str()     const { return atom_table()[m_index].c_str();  }
    const std::string&  to_string() const { return atom_table()[m_index];          }
    size_t              size()      const { return atom_table()[m_index].size();   }
    size_t              length()    const { return size();                         }
    bool                empty()     const { return m_index == 0;                   }

    /// Get atom's index in the atom table.
    int             index()     const { return m_index; }

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
    size_t encode_size() const { return 3 + length(); }

    /// Encode the atom in external binary format.
    /// @param buf is the buffer space to encode the atom to.
    /// @param idx is the offset in the \a buf where to begin writing.
    /// @param size is the size of \a buf.
    void encode(char* buf, int& idx, size_t size) const {
        char* s  = buf + idx;
        char* s0 = s;
        const int len = std::min((size_t)MAXATOMLEN, length());
        /* This function is documented to truncate at MAXATOMLEN (256) */
        put8(s,ERL_ATOM_EXT);
        put16be(s,len);
        memmove(s,c_str(),len); /* unterminated string */
        s   += len;
        idx += s-s0;
        BOOST_ASSERT((size_t)idx <= size);
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
/// @throws std::runtime_error if atom table is full.
/// @throws err_bad_argument if atom size is longer than MAXNODELEN
inline atom make_node_name(const std::string& s)
    throw(std::runtime_error, err_bad_argument)
{
    if (!s.find('@')) throw err_bad_argument("Invalid node name", s);
    detail::check_node_length(s.size());
    return atom(s);
}


} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {

    inline ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::atom& s) {
        return s.dump(out);
    }

}

#endif
