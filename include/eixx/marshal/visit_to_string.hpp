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
#ifndef _IMPL_VISIT_TO_STRING_HPP_
#define _IMPL_VISIT_TO_STRING_HPP_

#include <eixx/marshal/visit.hpp>
#include <eixx/marshal/varbind.hpp>
#include <ostream>
#include <string.h>

namespace eixx {
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
} // namespace eixx

#endif // _IMPL_VISIT_TO_STRING_HPP_
