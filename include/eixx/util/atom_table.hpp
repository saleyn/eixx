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

namespace eixx {
namespace util {

    namespace eid = eixx::detail;
    using eid::lock_guard;

    inline size_t utf8_length(const std::string& str) {
        size_t count = 0;
        const char* s = str.c_str();
        while (*s) count += (*s++ & 0xc0) != 0x80;
        return count;
    }

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

        explicit basic_atom_table(size_t a_max_atoms = default_size())
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

        /// Try to lookup an atom in the atom table
        /// @return -1 if the atom by name is not found, -2 if the atom is invalid,
        ///         or a value >= 0, if an existing atom is found.
        int try_lookup(const char* a_name, size_t n) { return try_lookup(String(a_name, n)); }
        int try_lookup(const char* a_name)           { return try_lookup(String(a_name)); }
        int try_lookup(const String& a_name)
        {
            if (a_name.size() == 0)
                return 0;
            if (a_name.size() > MAXATOMLEN_UTF8 || utf8_length(a_name) > MAXATOMLEN)
                return -2;
            size_t bucket = m_index.bucket(a_name.c_str());
            int    n      = find_value(bucket, a_name.c_str());
            return n > 0 ? n : -1;
        }

        /// Lookup an atom in the atom table by name. If the atom is not
        /// present in the atom table - add it.  Return the index of the 
        /// atom in the atom table.
        /// @throw std::runtime_error if atom table is full.
        /// @throw err_bad_argument if atom size is longer than MAXATOMLEN
        int lookup(const char* a_name, size_t n) { return lookup(String(a_name, n)); }
        int lookup(const char* a_name)           { return lookup(String(a_name)); }
        int lookup(const String& a_name)
        {
            auto n = try_lookup(a_name);
            if  (n >=  0) return n;
            if  (n == -2) throw  err_bad_argument("Atom size is too long!");

            lock_guard<Mutex> guard(m_lock);
            if (!std::is_same<Mutex, eid::null_mutex>::value) {
                size_t bucket = m_index.bucket(a_name.c_str());
                n = find_value(bucket, a_name.c_str());
                if (n >= 0)
                    return n;
            }

            n = m_atoms.size();
            if ((size_t)(n+1) == m_atoms.capacity())
                throw std::runtime_error("Atom hash table is full!");
            m_atoms.push_back(a_name);
            m_index[m_atoms.back().c_str()] = n;
            return n;
        }
    private:
        Vector  m_atoms;
        HashMap m_index;
        Mutex   m_lock;
    };

    typedef basic_atom_table<> atom_table;

} // namespace util
} // namespace eixx

#endif
