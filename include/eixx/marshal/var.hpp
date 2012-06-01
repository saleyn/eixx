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
template <class Alloc>
class var : protected string<Alloc>
{
    typedef string<Alloc> base_t;
public:
    var(const Alloc& a = Alloc())                       : base_t("_", a) {}
    var(const char* s, const Alloc& a = Alloc())        : base_t(s, a) {}
    var(const std::string& s, const Alloc& a = Alloc()) : base_t(s.c_str(), s.size(), a) {}
    var(const char* s, size_t n, const Alloc& a = Alloc()) : base_t(s, n, a) {}
    var(const var<Alloc>& s) : base_t(s) {}

    const char*             c_str()         const { return base_t::c_str(); }
    const string<Alloc>&    name()          const { return *this; }
    size_t                  size()          const { return base_t::size();  }
    size_t                  length()        const { return base_t::length(); }

    bool                    is_any()        const { return base_t::size() == 1 && c_str()[0] == '_'; }

    template <typename T>
    bool        operator==(const T&)        const { return false; }

    size_t encode_size() const { throw err_encode_exception("Cannot encode vars!"); }

    void encode(char* buf, int& idx, size_t size) const {
        throw err_encode_exception("Cannot encode vars!");
    }    

    const eterm<Alloc>*
    find_unbound(const varbind<Alloc>* binding = NULL) const {
        if (is_any()) return NULL;
        return binding ? binding->find(c_str()) : NULL;
    }

    bool subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
        throw (err_unbound_variable) {
        if (is_any()) throw err_unbound_variable(c_str());
        const eterm<Alloc>* term = binding ? binding->find(c_str()) : NULL;
        if (!term) throw err_unbound_variable(c_str());
        out = *term;
        return true;
    }

    bool match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const
        throw (err_unbound_variable) {
        if (is_any()) return true;
        const eterm<Alloc>* value = binding ? binding->find(c_str()) : NULL;
        if (value)
            return value->match(pattern, binding);
        if (binding != NULL) {
            // Bind the variable
            eterm<Alloc> et;
            binding->bind(name(), pattern.subst(et, binding) ? et : pattern);
        }
        return true; 
    }

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding = NULL) const {
        if (is_any()) { return out << c_str(); }
        const eterm<Alloc>* term = binding ? binding->find(name()) : NULL;
        return out << (term ? term->to_string(std::string::npos, binding) : *this);
    }
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::var<Alloc>& s) {
        return s.dump(out);
    }
} // namespace std

#endif
