//----------------------------------------------------------------------------
/// \file  visit_to_string.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing eterm to string conversion visitor.
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
#ifndef _IMPL_VISIT_TO_STRING_HPP_
#define _IMPL_VISIT_TO_STRING_HPP_

#include <eixx/marshal/visit.hpp>
#include <eixx/marshal/varbind.hpp>
#include <ostream>
#include <string.h>

namespace EIXX_NAMESPACE {
namespace marshal {

template <typename Alloc>
class visit_eterm_stringify
    : public static_visitor<visit_eterm_stringify<Alloc>, void> {
    std::ostream& out;  // FIXME:
    const varbind<Alloc>* vars;
public: 
    visit_eterm_stringify(std::ostream& a, const varbind<Alloc>* binding=NULL)
        : out(a), vars(binding)
    {}

    template <typename T>
    void operator() (const T& a) const { a.dump(out, vars); }
    void operator() (bool     a) const { out << (a ? "true" : "false"); }
    void operator() (long     a) const { out << a; }
    void operator() (double   a) const {
        char buf[128];
        snprintf(buf, sizeof(buf)-1, "%f", a);
        // Remove trailing zeros.
        for (char* p = buf+strlen(buf); p > buf && *(p-1) != '.'; p--)
            if (*p == '0') *p = '\0';
        out << buf;
    }
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

#endif // _IMPL_VISIT_TO_STRING_HPP_
