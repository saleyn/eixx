//----------------------------------------------------------------------------
/// \file  eterm_match.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing pattern matching on eterm terms.
///        Implementation inspired by the
///        <a href="http://code.google.com/p/epi">epi</a> project.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Copyright (c) 2005 Hector Rivas Gandara <keymon@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) Library.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>
Copyright (C) 2005 Hector Rivas Gandara <keymon@gmail.com>

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

#ifndef _EI_MATCH_HPP_
#define _EI_MATCH_HPP_

#include <eixx/marshal/eterm.hpp>
#include <boost/function.hpp>
#include <list>
#include <stdarg.h>

namespace EIXX_NAMESPACE {
namespace marshal {

// Forward declaration
template <class Alloc>
class eterm_pattern_action;

/**
 * Performs pattern match of a term against a list of registered
 * patterns.  Invokes a callback of a pattern on successful match.
 * If a match succeeded on any pattern the other patters are not 
 * checked
 */
template <class Alloc>
class eterm_pattern_matcher {
    std::list<eterm_pattern_action<Alloc>, Alloc> m_pattern_list;
    std::list< eterm_pattern_action< Alloc >, Alloc > it;
public:
    typedef std::list<eterm_pattern_action<Alloc>, Alloc> list_t;
    typedef typename list_t::const_iterator const_iterator;
    typedef typename list_t::iterator       iterator;

    struct init_struct {
        eterm<Alloc> p;
        long opaque;
    };

    /**
     * Function to be called on pattern match.
     * Arguments are:
     *   a_pattern is the pattern that matched the message.
     *   a_binding is the bound variable bindings.
     *   a_opaque  is some opaque integer associated with pattern.
     * If the match is considered successful and the following
     * patterns in the match list shouldn't be checked, the
     * functor must return true.
     */
    typedef boost::function<
        bool (const eterm<Alloc>& a_pattern,
              const varbind<Alloc>& a_binding,
              long  a_opaque) > pattern_functor_t;

    /**
     * Pattern matching constructor.
     */
    explicit eterm_pattern_matcher(const Alloc& a_alloc = Alloc())
        : m_pattern_list(a_alloc) {}

    /**
     * Construct pattern matcher from a list of patterns. This
     * is the same as calling eterm_pattern_matcher() and iteratively
     * adding patterns using push_back() calls.
     */
    template <int N>
    eterm_pattern_matcher(
        const struct init_struct (&a_patterns)[N], pattern_functor_t a_fun,
        const Alloc& a_alloc = Alloc())
        : m_pattern_list(a_alloc)
    {
        init(a_patterns, N, a_fun);
    }

    eterm_pattern_matcher(
        std::initializer_list<eterm_pattern_action<Alloc>> a_list,
        const Alloc& a_alloc = Alloc())
        : m_pattern_list(a_alloc)
    {
        std::copy(a_list.begin(), a_list.end(), m_pattern_list.begin());
    }

    /**
     * Initialize the pattern list from a given array of patterns.
     */
    void init(const struct init_struct* a_patterns, size_t sz, pattern_functor_t a_fun) {
        m_pattern_list.clear();
        for(size_t i=0; i < sz; i++)
            push_back(a_patterns[i].p, a_fun, a_patterns[i].opaque);
    }

    /**
     * Add a pattern to the end of the list of patterns to be matched
     * against an eterm in call to eterm_matcher::match().
     * @param a_pattern is the pattern to add.
     * @param a_fun is the functor to call on successful match. This
     *        functor takes to arguments - the <tt>eterm</tt> pattern 
     *        object and the <tt>varbind</tt> object.
     * @param a_opaque is an opaque long value passed to \a a_fun in callback.
     * The pattern is assign to a smart pointer.
     */
    const eterm_pattern_action<Alloc>& 
    push_back(const eterm<Alloc>& a_pattern, pattern_functor_t a_fun, long a_opaque=0) {
        m_pattern_list.push_back(eterm_pattern_action<Alloc>(a_pattern, a_fun, a_opaque));
        return m_pattern_list.back();
    }

    /**
     * Add a pattern to the beginning of the list.
     * The pattern is assign to a smart pointer.
     */
    const eterm_pattern_action<Alloc>& 
    push_front(const eterm<Alloc>& a_pattern, pattern_functor_t a_fun, long a_opaque=0) {
        m_pattern_list.push_back(eterm_pattern_action<Alloc>(a_pattern, a_fun, a_opaque));
        return m_pattern_list.front();
    }

    /**
     * Erase a pattern from list.
     * @param a_item is a pattern action reference returned by push_back()
     *        or push_front() calls.
     */
    void erase(const eterm_pattern_action<Alloc>& a_item) {
        iterator it =
            std::find(m_pattern_list.begin(), m_pattern_list.end(), a_item);
        if (it != m_pattern_list.end())
            m_pattern_list.erase(it);
    }

    /**
     * Clear the list of patterns.
     */
    void clear() { m_pattern_list.clear(); }

    /**
     * Returns the number of patterns in the list.
     */
    size_t size() const { return m_pattern_list.size(); }

    const_iterator begin() const { return m_pattern_list.begin(); }
    const_iterator end()   const { return m_pattern_list.end(); }
    iterator       begin()       { return m_pattern_list.begin(); }
    iterator       end()         { return m_pattern_list.end(); }

    const eterm_pattern_action<Alloc>&
    front() const { return m_pattern_list.front(); }

    const eterm_pattern_action<Alloc>&
    back() const { return m_pattern_list.back(); }

    /**
     * Match a term against registered patterns.
     * @param a_term is the term to match.
     * @param a_binding is an optional object containing
     *        predefined variable bindings that will be passed
     *        to every pattern.
     * @return true if any one pattern matched the term.
     */
    bool match(const eterm<Alloc>& a_term,
               varbind<Alloc>* a_binding = NULL) const
    {
        bool res = false;
        for(auto it = m_pattern_list.begin(), end = m_pattern_list.end(); it != end; ++it) {
            if (it->operator() (a_term, a_binding)) {
                res = true;
                break;
            }
        }
        return res;
    }
};

/**
 * Pattern matching action with an associated functor that is invoked
 * on successful match.
 */
template <class Alloc>
class eterm_pattern_action {
    typedef typename eterm_pattern_matcher<Alloc>::pattern_functor_t
        pattern_functor_t;

    eterm<Alloc>        m_pattern;
    pattern_functor_t   m_fun;
    long                m_opaque;
public:
    /**
     * Create a new pattern match functor.
     * @param a_pattern pattern to match
     * @param a_fun is a functor to execute on successful match.
     * @param a_opaque is an opaque long value passed to a_fun in callback.
     */
    eterm_pattern_action(
        const eterm<Alloc>& a_pattern, 
        pattern_functor_t& a_fun, long a_opaque = 0)
        : m_pattern(a_pattern), m_fun(a_fun), m_opaque(a_opaque)
    {
        BOOST_ASSERT(m_fun != NULL);
    }

    eterm_pattern_action(
        const Alloc& a_alloc, pattern_functor_t& a_fun, long a_opaque,
        const char* a_pat_fmt, ...)
        : m_fun(a_fun), m_opaque(a_opaque)
    {
        BOOST_ASSERT(m_fun != NULL);
        va_list ap;
        va_start(ap, a_pat_fmt);
        try         { m_pattern = eterm<Alloc>::format(a_alloc, &a_pat_fmt, &ap); }
        catch (...) { va_end(ap); throw; }
        va_end(ap);
    }

    eterm_pattern_action(const eterm_pattern_action& a_rhs)
        : m_pattern(a_rhs.m_pattern)
        , m_fun(a_rhs.m_fun)
        , m_opaque(a_rhs.m_opaque)
    {}

    eterm_pattern_action(eterm_pattern_action&& a_rhs)
        : m_pattern(std::move(a_rhs.m_pattern))
        , m_fun(std::move(a_rhs.m_fun))
        , m_opaque(a_rhs.m_opaque)
    {}

    bool operator() (const eterm<Alloc>& a_term,
                     varbind<Alloc>* a_binding) const 
        throw (eterm_exception)
    {
        varbind<Alloc> binding;
        if (a_binding)
            binding.merge(*a_binding);
        if (m_pattern.match(a_term, &binding))
            return m_fun(m_pattern, binding, m_opaque);
        return false;
    }

    const eterm<Alloc>& pattern()   const { return m_pattern; }
    long opaque()                   const { return m_opaque; }
    void opaque(long a_opaque)            { m_opaque = a_opaque; }

    bool operator== (const eterm_pattern_action<Alloc>& rhs) {
        return this->m_pattern.equals(rhs.m_pattern);
    }
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

#endif // _EI_MATCH_HPP_
