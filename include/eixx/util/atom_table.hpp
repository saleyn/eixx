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
#ifndef _EIXX_ATOM_TABLE_HPP_
#define _EIXX_ATOM_TABLE_HPP_

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
namespace util {

    namespace eid = EIXX_NAMESPACE::detail;
    using eid::lock_guard;

    /// Non-garbage collected hash table for atoms. It stores strings
    /// as atoms so that atoms can be quickly compared to with O(1)
    /// complexity.  The instance of this table is statically maintained
    /// and its content is never cleared.  The table contains a unique
    /// list of strings represented as atoms added throughout the lifetime
    /// of the application.
    template
    <
        typename String  = std::string,
        typename Vector  = std::vector<String>,
        typename HashMap = char_int_hash_map,
        typename Mutex   = eid::mutex
    >
    class basic_atom_table : private detail::hsieh_hash_fun {
        static const int s_default_max_atoms = 1024*1024;

        int find_value(size_t bucket, const char* a_atom) {
            typename HashMap::const_local_iterator
                lit = m_index.begin(bucket), lend = m_index.end(bucket);
            while(lit != lend && strcmp(a_atom, lit->first) != 0) ++lit;
            return lit == lend ? -1 : lit->second;
        }
    public:
        /// Returns the default atom table maximum size. The value can be
        /// changed by setting the EI_ATOM_TABLE_SIZE environment variable. 
        static size_t default_size() {
            const char* p = getenv("EI_ATOM_TABLE_SIZE");
            int n = p ? atoi(p) : s_default_max_atoms;
            return n > 0 && n < 1024*1024*100 ? n : s_default_max_atoms;
        }

        /// Returns the maximum number of atoms that can be stored in the atom table.
        size_t capacity()  const { return m_atoms.capacity(); }

        /// Returns the current number of atoms stored in the atom table.
        size_t allocated() const { return m_atoms.size();     }

        explicit basic_atom_table(int a_max_atoms = default_size())
            : m_index(a_max_atoms) {
            m_atoms.reserve(a_max_atoms);
            m_atoms.push_back(""); // The 0-th element is an empty atom ("").
            m_index[""] = 0;
        }

        ~basic_atom_table() {
            lock_guard<Mutex> guard(m_lock);
            m_atoms.clear();
            m_index.clear();
        }

        /// Lookup an atom in the atom table by index.
        const String& get(int n) const { return (*this)[n]; }

        /// Lookup an atom in the atom table by index.
        const String& operator[] (int n) const {
            BOOST_ASSERT((size_t)n < m_atoms.size());
            return m_atoms[n];
        }

        /// Lookup an atom in the atom table by name. If the atom is not
        /// present in the atom table - add it.  Return the index of the 
        /// atom in the atom table.
        /// @throws std::runtime_error if atom table is full.
        /// @throws err_bad_argument if atom size is longer than MAXATOMLEN
        int lookup(const char* a_name, size_t n) { return lookup(String(a_name, n)); }
        int lookup(const char* a_name)           { return lookup(String(a_name)); }
        int lookup(const String& a_name)
            throw(std::runtime_error, err_bad_argument)
        {
            if (a_name.size() == 0)
                return 0;
            if (a_name.size() > MAXATOMLEN)
                throw err_bad_argument("Atom size is too long!");
            size_t bucket = m_index.bucket(a_name.c_str());
            int n = find_value(bucket, a_name.c_str());
            if (n >= 0)
                return n;

            lock_guard<Mutex> guard(m_lock);
            n = find_value(bucket, a_name.c_str());
            if (n >= 0)
                return n;

            n = m_atoms.size();
            if ((size_t)(n+1) == m_atoms.capacity())
                throw std::runtime_error("Atom hash table is full!");
            m_atoms.push_back(a_name);
            m_index[a_name.c_str()] = n;
            return n;
        }
    private:
        Vector  m_atoms;
        HashMap m_index;
        Mutex   m_lock;
    };

    typedef basic_atom_table<> atom_table;

} // namespace util
} // namespace EIXX_NAMESPACE

#endif
