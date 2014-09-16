//----------------------------------------------------------------------------
/// \file  varbind.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing a container of bound variables.
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

#ifndef _IMPL_VARBIND_HPP_
#define _IMPL_VARBIND_HPP_

#include <string>
#include <ostream>
#include <map>
#include <eixx/marshal/eterm.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

/// Name-value pair associating an eterm with an atom name
template <class Alloc>
struct epair : std::pair<atom, eterm<Alloc> > {
    typedef std::pair<atom, eterm<Alloc> > base;

    epair(atom a_name, const eterm<Alloc>& a_value) : base(a_name, a_value) {}

#if __cplusplus >= 201103L
    epair(atom a_name, eterm<Alloc>&& a_value) : base(a_name, std::move(a_value)) {}
    epair(const epair& a_rhs)                  : base(a_rhs) {}
    epair(epair&& a_rhs)                       : base(std::forward<epair>(a_rhs)) {}

    epair& operator=(const epair& a_rhs) {
        return base::operator=(static_cast<const base&>(a_rhs));
    }

#endif

    atom                name()  const { return base::first;  }
    const eterm<Alloc>& value() const { return base::second; }
    eterm<Alloc>&       value()       { return base::second; }
};

/**
 * This class maintains bindings of variables to values.
 */
template <class Alloc>
class varbind {

    template<class AllocT>
    friend std::ostream& std::operator<< (
        std::ostream& out, const varbind<AllocT>& binding);

protected:
    typedef std::map<atom, eterm<Alloc>, std::less<atom>, Alloc> eterm_map_t;

public:
    explicit varbind(const Alloc& a_alloc = Alloc())
        : m_term_map(std::less<atom>(), a_alloc)
    {}

    varbind(const varbind<Alloc>& rhs) : m_term_map(rhs.m_term_map)
    {}

#if __cplusplus >= 201103L
    varbind(std::initializer_list<epair<Alloc>> a_list) {
        m_term_map.insert(a_list.begin(), a_list.end());
    }
//     varbind(std::initializer_list<std::pair<atom, eterm<Alloc>> a_list) {
//         m_term_map.insert(a_list.begin(), a_list.end());
//     }
#endif

    void copy(const varbind<Alloc>& rhs) { m_term_map = rhs.m_term_map; }

    /**
     * Bind a value to a variable name. The binding will be updated
     * if a_var_name is unbound. If its bound, it will ignore the new value.
     * @param a_var_name
     * @param a_term eterm to bind (must be non-zero).
     */
    void bind(const char* a_var_name, const eterm<Alloc>& a_term) {
        bind(atom(a_var_name), a_term);
    }

    void bind(const string<Alloc>& a_var_name, const eterm<Alloc>& a_term) {
        bind(atom(a_var_name), a_term);
    }

    void bind(atom a_var_name, const eterm<Alloc>& a_term) {
        // bind only if is unbound
        m_term_map.insert(std::make_pair(a_var_name, a_term));
    }

    /**
     * Search for a variable by name
     * @param a_var_name variable to find
     * @return bound eterm pointer if variable is bound, 0 otherwise
     */
    const eterm<Alloc>*
    find(const char* a_var_name) const { return find(atom(a_var_name)); }

    const eterm<Alloc>*
    find(const string<Alloc>& a_var_name) const { return find(atom(a_var_name)); }

    const eterm<Alloc>*
    find(atom a_var_name) const {
        typename eterm_map_t::const_iterator p = m_term_map.find(a_var_name);
        return (p != m_term_map.end()) ? &p->second : NULL;
    }

    const eterm<Alloc>*
    operator[] (const char* a_var_name) const throw(err_unbound_variable) {
        return (*this)[atom(a_var_name)];
    }

    const eterm<Alloc>*
    operator[] (const string<Alloc>& a_var_name) const throw(err_unbound_variable) {
        return (*this)[atom(a_var_name)];
    }

    const eterm<Alloc>*
    operator[] (atom a_var_name) const throw(err_unbound_variable) {
        const eterm<Alloc>* p = find(a_var_name);
        if (!p) throw err_unbound_variable(a_var_name.c_str());
        return p;
    }

    /**
     * Merge all data in a variable binding with data in this
     * variable binding.
     * @param binding pointer to binding to use.
     */
    void merge(const varbind<Alloc>& binding) {
        for (typename eterm_map_t::const_iterator it = binding.m_term_map.begin(),
             end=binding.m_term_map.end(); it != end; ++it)
            bind(it->first, it->second);
    }

    /**
     * Reset this binding
     */
    void clear() { m_term_map.clear(); }

    /**
     * Convert varbind to string
     */
    void dump(std::ostream& out) const { out << *this; }

    /**
     * Return the number of bound variables held in internal dictionary.
     */
    size_t count() const { return m_term_map.size(); }

protected:
    eterm_map_t m_term_map;
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {

    template <class Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::varbind<Alloc>& binding) {
        using namespace EIXX_NAMESPACE::marshal;

        for (typename varbind<Alloc>::eterm_map_t::const_iterator
                it = binding.m_term_map.begin(); it != binding.m_term_map.end(); ++it)
            out << "    " << it->first.to_string() << " = " << it->second << std::endl;
        return out;
    }

} // namespace std

#endif
