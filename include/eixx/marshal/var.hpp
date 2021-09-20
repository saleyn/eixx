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
#ifndef _IMPL_VAR_HPP_
#define _IMPL_VAR_HPP_

#include <string>
#include <eixx/eterm_exception.hpp>
#include <eixx/marshal/am.hpp>
#include <eixx/marshal/varbind.hpp>

namespace eixx {
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
    atom       m_name;
    eterm_type m_type;

    template <class Alloc>
    bool check_type(const eterm<Alloc>& t) const {
        return is_any() || m_type == UNDEFINED || t.type() == m_type
            || (m_type == STRING && t.is_list() && t.to_list().empty());
    }

    eterm_type set(eterm_type t) { return m_name == am_ANY_ ? UNDEFINED : t; }

public:
    var(eterm_type t = UNDEFINED)
        : var(am_ANY_, t)
    {
        static_assert(sizeof(var) == sizeof(uint64_t), "Invalid class size!");
    }

    var(const atom& s, eterm_type t = UNDEFINED)    : m_name(s) { m_type = set(t); }

    var(const char* s, eterm_type t = UNDEFINED)            : var(atom(s), t) {}
    var(const std::string& s, eterm_type t = UNDEFINED)     : var(atom(s), t) {}
    template <typename Alloc>
    var(const string<Alloc>& s, eterm_type t = UNDEFINED)   : var(atom(s), t) {}
    var(const char* s, size_t n, eterm_type t = UNDEFINED)  : var(atom(s, n), t) {}
    var(const var& v)                                       : var(v.name(), v.type()) {}

    const char*             c_str()         const { return m_name.c_str(); }
    const std::string&      str()           const { return m_name.to_string(); }
    atom                    name()          const { return m_name; }
    size_t                  length()        const { return m_name.length(); }

    eterm_type              type()          const { return m_type; }
    bool                    is_any()        const { return name() == am_ANY_; }

    std::string to_string() const {
        std::stringstream s;
        s << name().to_string() << type_to_type_string(type(), true);
        return s.str();
    }

    template <typename T>
    bool operator==(const T&) const { return false; }

    template <typename T>
    bool operator<(const T&)  const { return false; }

    size_t encode_size() const { throw err_encode_exception("Cannot encode vars!"); }

    void encode(char*, uintptr_t&, size_t) const {
        throw err_encode_exception("Cannot encode vars!");
    }

    template <typename Alloc>
    const eterm<Alloc>*
    find_unbound(const varbind<Alloc>* binding = NULL) const {
        return binding ? binding->find(name()) : NULL;
    }

    template <typename Alloc>
    bool subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
    {
        const eterm<Alloc>* term = binding ? binding->find(name()) : NULL;
        if (!term || !check_type(*term))
            throw err_unbound_variable(c_str());
        out = *term;
        return true;
    }

    template <typename Alloc>
    bool match(const eterm<Alloc>& pattern, varbind<Alloc>* binding) const
    {
        if (is_any()) return true;
        if (!binding) return false;
        const eterm<Alloc>* value = binding->find(name());
        if (value)
            return check_type(*value) ? value->match(pattern, binding) : false;
        if (!check_type(pattern))
            return false;
        // Bind the variable
        eterm<Alloc> et;
        binding->bind(name(), pattern.subst(et, binding) ? et : pattern);
        return true; 
    }

    template <typename Alloc>
    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding = NULL) const {
        const eterm<Alloc>* term = binding ? binding->find(name()) : NULL;
        return out << (term && check_type(*term)
                        ? term->to_string(std::string::npos, binding) : to_string());
    }
};

BOOST_STATIC_ASSERT(sizeof(var) == 8);

} // namespace marshal
} // namespace eixx

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, eixx::marshal::var s) {
        return s.dump(out, (const eixx::marshal::varbind<Alloc>*)NULL);
    }
} // namespace std

#endif
