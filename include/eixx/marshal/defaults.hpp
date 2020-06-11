//----------------------------------------------------------------------------
/// \file  defaults.hpp
//----------------------------------------------------------------------------
/// \brief Definition of some globally used types.
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
#ifndef _EIXX_DEFAULTS_HPP
#define _EIXX_DEFAULTS_HPP

#include <string.h>
#include <eixx/util/compiler_hints.hpp>

namespace eixx {
    namespace marshal {

        // Forward declarations
        template <typename Alloc> class eterm;
        template <typename Alloc> class tuple;
        template <typename Alloc> class list;

        namespace marshal {
            template <typename Alloc> struct visit_eterm_stringify;
            template <typename Alloc> struct visit_eterm_encode_size_calc;
        }

        /// Maximum and default sizes
        enum {
              DEF_HEADER_SIZE   = 4
        };

        template <typename T, typename Alloc> T& get(eterm<Alloc>& t);
    } // namespace marshal

    // eterm types
    enum eterm_type {
          UNDEFINED         = 0
        , LONG              = 1
        , DOUBLE            = 2
        , BOOL              = 3
        , ATOM              = 4
        , VAR               = 5
        // STRING is the first compound item that requires destruction
        , STRING            = 6
        , BINARY            = 7
        , PID               = 8
        , PORT              = 9
        , REF               = 10
        , TUPLE             = 11
        , LIST              = 12
        , MAP               = 13
        , TRACE             = 14
        , MAX_ETERM_TYPE    = 14
    };

    /// Returns string representation of type \a a_type.
    const char* type_to_string(eterm_type a_type);

    /// Converts \a a_type to string
    /// @param a_type is the type to convert
    /// @param a_prefix if true, the value is prepended with "::"
    ///
    /// Example: printf("%s\n", type_to_type_string(eterm_type::BOOL, true);
    ///             Outputs:  ::bool()
    const char* type_to_type_string(eterm_type a_type, bool a_prefix=false);

    /// Converts a string to eterm type (e.g. "binary" -> eterm_type::BINARY)
    eterm_type type_string_to_type(const char* s, size_t n);

    /// Converts a string \a s to eterm type (e.g. "binary" -> eterm_type::BINARY)
    inline eterm_type type_string_to_type(const char* s) {
        return type_string_to_type(s, strlen(s));
    }

    inline const char* type_to_string(eterm_type a_type) {
        switch (a_type) {
            case LONG  : return "LONG";
            case DOUBLE: return "DOUBLE";
            case BOOL  : return "BOOL";
            case ATOM  : return "ATOM";
            case STRING: return "STRING";
            case BINARY: return "BINARY";
            case PID   : return "PID";
            case PORT  : return "PORT";
            case REF   : return "REF";
            case VAR   : return "VAR";
            case TUPLE : return "TUPLE";
            case LIST  : return "LIST";
            case MAP   : return "MAP";
            case TRACE : return "TRACE";
            default    : return "UNDEFINED";
        }
    }

    inline const char* type_to_type_string(eterm_type a_type, bool a_prefix) {
        switch (a_type) {
            case LONG  : return a_prefix ? "::int()"    : "int()";
            case DOUBLE: return a_prefix ? "::float()"  : "float()";
            case BOOL  : return a_prefix ? "::bool()"   : "bool()";
            case ATOM  : return a_prefix ? "::atom()"   : "atom()";
            case STRING: return a_prefix ? "::string()" : "string()";
            case BINARY: return a_prefix ? "::binary()" : "binary()";
            case PID   : return a_prefix ? "::pid()"    : "pid()";
            case PORT  : return a_prefix ? "::port()"   : "port()";
            case REF   : return a_prefix ? "::ref()"    : "ref()";
            case VAR   : return a_prefix ? "::var()"    : "var()";
            case TUPLE : return a_prefix ? "::tuple()"  : "tuple()";
            case LIST  : return a_prefix ? "::list()"   : "list()";
            case MAP   : return a_prefix ? "::map()"    : "map()";
            case TRACE : return a_prefix ? "::trace()"  : "trace()";
            default    : return "";
        }
    }

} // namespace eixx

#endif // _EIXX_DEFAULTS_HPP

