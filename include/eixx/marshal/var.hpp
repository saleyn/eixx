//----------------------------------------------------------------------------
/// \file  var.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing a variable used in pattern matching Erlang
///        terms.
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
#ifndef _IMPL_VAR_HPP_
#define _IMPL_VAR_HPP_

#include <string>
#include <eixx/eterm_exception.hpp>
#include <eixx/marshal/varbind.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

/**
 * Provides a representation of an Variable.
 * A variable has a name (string). The name is case sensitive.
 * Always equals to false.
 * The essencial difference is in match method.
 * If you use '_' as variable, it will allways succeeds in matching, but
 * it will not be bound.
 **/
class var
{
    union u {
        struct s {
            uint32_t type;
            atom     name;
        } s;
        uint64_t i;

    } m_data;

    BOOST_STATIC_ASSERT(sizeof(var) == 8);

    bool check_type(eterm_type t) const { return is_any() || t == type(); }

public:
    var(eterm_type t = eterm_type::UNDEFINED)
        { m_data.s.type = t; m_data.s.name = am_ANY_;  }
    var(const char* s, eterm_type t = eterm_type::UNDEFINED)
        { m_data.s.type = t; m_data.s.name = atom(s); }
    var(const std::string& s, eterm_type t = eterm_type::UNDEFINED)
        { m_data.s.type = t; m_data.s.name = atom(s); }
    template <typename Alloc>
    var(const std::string<Alloc>& s, eterm_type t = eterm_type::UNDEFINED)
        { m_data.s.type = t; m_data.s.name = atom(s); }
    var(const char* s, size_t n, eterm_type t = eterm_type::UNDEFINED)
        { m_data.s.type = t; m_data.s.name = atom(s, n); }
    var(const atom& s, eterm_type t = eterm_type::UNDEFINED)
        { m_data.s.type = t; m_data.s.name = s; }
    var(const var& v) { m_data.i = v.m_data.i; }

    const char*             c_str()         const { return m_data.s.name.c_str(); }
    const string_t&         str()           const { return m_data.s.name.to_string(); }
    atom                    name()          const { return m_data.s.name; }
    size_t                  length()        const { return m_data.s.name.length(); }

    eterm_type              type()          const { return m_data.s.type; }
    bool                    is_any()        const { return name() == am_ANY_; }

    const string_t& to_string() const {
        std::stringstream s;
        s << name().to_string() << type_to_type_string(type(), true);
        return s.str();
    }

    template <typename T>
    bool operator==(const T&) const { return false; }

    size_t encode_size() const { throw err_encode_exception("Cannot encode vars!"); }

    void encode(char* buf, int& idx, size_t size) const {
        throw err_encode_exception("Cannot encode vars!");
    }    

    template <Alloc>
    const eterm<Alloc>*
    find_unbound(const varbind<Alloc>* binding = NULL) const {
        return binding ? binding->find(name()) : NULL;
    }

    template <Alloc>
    bool subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
        throw (err_unbound_variable)
    {
        const eterm<Alloc>* term = binding ? binding->find(name()) : NULL;
        if (!term || !check_type(term->type()))
            throw err_unbound_variable(c_str());
        out = *term;
        return true;
    }

    template <Alloc>
    bool match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const
        throw (err_unbound_variable)
    {
        if (!binding) return false;
        const eterm<Alloc>* value = binding ? binding->find(name()) : NULL;
        if (value)
            return check_type(value->type()) ? value->match(pattern, binding) : false;
        if (!check_type(pattern.type()))
            return false;
        // Bind the variable
        eterm<Alloc> et;
        binding->bind(name(), pattern.subst(et, binding) ? et : pattern);
        return true; 
    }

    template <Alloc>
    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding = NULL) const {
        const eterm<Alloc>* term = binding ? binding->find(name()) : NULL;
        return out << (term && check_type(term->type())
                        ? term->to_string(std::string::npos, binding) : *this);
    }
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, EIXX_NAMESPACE::marshal::var s) {
        return s.dump(out);
    }
} // namespace std

#endif
