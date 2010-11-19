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

    /// Non-garbage collected hash table for atoms. It stores strings
    /// as atoms so that atoms can be quickly compared to with O(1)
    /// complexity.  The instance of this table is statically maintained
    /// and its content is never cleared.  The table contains a unique
    /// list of strings represented as atoms added throughout the lifetime
    /// of the application.
    template <typename Mutex = eid::mutex>
    class atom_table {
    public:
        //typedef std::basic_string<char, std::char_traits<char>, Alloc> string_t;
        typedef std::string string_t;
    private:
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

        static const size_t s_default_max_atoms = 1024*1024;

        int find_value(size_t bucket, const char* a_atom) {
            typename char_int_hash_map::const_local_iterator
                lit = m_index.begin(bucket), lend = m_index.end(bucket);
            while(lit != lend && strcmp(a_atom, lit->first) != 0) ++lit;
            return lit == lend ? -1 : lit->second;
        }
    public:
        static size_t default_size() {
            const char* p = getenv("EI_ATOM_TABLE_SIZE");
            int n = p ? atoi(p) : s_default_max_atoms;
            return n > 0 && n < 1024*1024*100 ? n : s_default_max_atoms;
        }

        /// Returns the maximum number of atoms that can be stored in the atom table.
        size_t capacity()  const { return m_atoms.capacity(); }

        /// Returns the current number of atoms stored in the atom table.
        size_t allocated() const { return m_atoms.size();     }

        explicit atom_table(size_t a_max_atoms = default_size())
            : m_index(a_max_atoms) {
            m_atoms.reserve(a_max_atoms);
            m_atoms.push_back(""); // The 0-th element is an empty atom ("").
            m_index[""] = 0;
        }

        ~atom_table() {
            lock_guard<Mutex> guard(m_lock);
            m_atoms.clear();
            m_index.clear();
        }

        /// Lookup an atom in the atom table by index.
        const string_t& lookup(size_t n) const { return (*this)[n]; }

        /// Lookup an atom in the atom table by index.
        const string_t& operator[] (size_t n) const {
            BOOST_ASSERT(n < m_atoms.size());
            return m_atoms[n];
        }

        /// Lookup an atom in the atom table by name. If the atom is not
        /// present in the atom table - add it.  Return the index of the 
        /// atom in the atom table.
        /// @throws std::runtime_error if atom table is full.
        /// @throws err_bad_argument if atom size is longer than MAXATOMLEN
        size_t lookup(const char* a_atom, size_t n) { return lookup(std::string(a_atom, n)); }
        size_t lookup(const char* a_atom)           { return lookup(std::string(a_atom)); }
        size_t lookup(const std::string& a_atom)
            throw(std::runtime_error, err_bad_argument)
        {
            if (a_atom.size() == 0)
                return 0;
            if (a_atom.size() > MAXATOMLEN)
                throw std::runtime_error("Atom size is too long!");
            size_t bucket = m_index.bucket(a_atom.c_str());
            int n = find_value(bucket, a_atom.c_str());
            if (n >= 0)
                return n;

            lock_guard<Mutex> guard(m_lock);
            n = find_value(bucket, a_atom.c_str());
            if (n >= 0)
                return n;
            
            n = m_atoms.size();
            if (n+1 == m_atoms.capacity())
                throw std::runtime_error("Atom hash table is full!");
            m_atoms.push_back(a_atom);
            m_index[a_atom.c_str()] = n;
            return n;
        }
    private:
        std::vector<string_t>   m_atoms;
        char_int_hash_map       m_index;
        Mutex                   m_lock;
    };
} // namespace detail

/**
 * Provides a representation of Erlang atoms. Atoms can be
 * created from strings whose length is not more than
 * MAXATOMLEN characters.
 */
class atom
{
    size_t m_index;

    typedef detail::atom_table<>::string_t string_t;

public:
    static detail::atom_table<>& atom_table();

    /// Create an empty atom
    atom() : m_index(0) {
        BOOST_STATIC_ASSERT(sizeof(atom) == sizeof(void*));
    }

    /// Create an atom from the given string.
    /// @param atom the string to create the atom from.
    /// @throws std::runtime_error if atom table is full.
    /// @throws err_bad_argument if atom size is longer than MAXATOMLEN
    atom(const char* s)
        throw(std::runtime_error, err_bad_argument)
        : m_index(atom_table().lookup(string_t(s))) {}

    /// @copydoc atom::atom
    template <int N>
    atom(const char (&s)[N])
        throw(std::runtime_error, err_bad_argument)
        : m_index(atom_table().lookup(string_t(s, N))) {}

    /// @copydoc atom::atom
    explicit atom(const std::string& s)
        throw(std::runtime_error)
        : m_index(atom_table().lookup(s))
    {}

    /// @copydoc atom::atom
    template<typename Alloc>
    explicit atom(const string<Alloc>& s)
        throw(std::runtime_error)
        : m_index(atom_table().lookup(string_t(s.c_str(), s.size())))
    {}

    /// @copydoc atom::atom
    atom(const char* s, size_t n)
        throw(std::runtime_error)
        : m_index(atom_table().lookup(string_t(s, n)))
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
        if (get8(s) != ERL_ATOM_EXT)
            throw err_decode_exception("Error decoding atom", idx);
        int len = get16be(s);
        m_index = atom_table().lookup(string_t(s, len));
        idx += s + len - s0;
        BOOST_ASSERT(idx <= a_size);
    }

    const char*     c_str()     const { return atom_table()[m_index].c_str();  }
    const string_t& to_string() const { return atom_table()[m_index];          }
    size_t          size()      const { return atom_table()[m_index].size();   }
    size_t          length()    const { return size();                         }
    bool            empty()     const { return m_index == 0;                   }

    /// Get atom's index in the atom table.
    size_t          index()     const { return m_index; }

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
            out << "'" << s << "'";
        else
            out << s;
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
