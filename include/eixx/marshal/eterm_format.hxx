//----------------------------------------------------------------------------
/// \file  eterm_format.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing encoding of eterm from a string.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Copyright (c) 2005 Hector Rivas Gandara <keymon@gmail.com>
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

/*
 * Implementation format functionality
 * This is *a copy* of source from erl_interface
 * Implementation is largely unfinished.
 *
 * The Initial Developer of the Original Code is Ericsson Utvecklings AB.
 * Portions created by Ericsson are Copyright 1999, Ericsson Utvecklings
 * AB. All Rights Reserved.''
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <vector>
#include <iostream>
#include <eixx/marshal/defaults.hpp>
#include <eixx/util/string_util.hpp>
#include <eixx/eterm_exception.hpp>
#include <boost/concept_check.hpp>
//#include <boost/scope_exit.hpp>

namespace eixx {
namespace marshal {

    namespace {

        template <typename Alloc>
        struct vector : public
            std::vector<
                eterm<Alloc>,
                typename std::allocator_traits<Alloc>::template rebind_alloc<eterm<Alloc>>
            >
        {
            using VecAlloc = typename std::allocator_traits<Alloc>::
                             template rebind_alloc<eterm<Alloc>>;
            using base     = std::vector<eterm<Alloc>, VecAlloc>;

            explicit vector(const Alloc& a_alloc = Alloc()) : base(a_alloc)
            {}

            eterm<Alloc> to_tuple(Alloc& a_alloc) {
                eterm<Alloc>& term = (eterm<Alloc>&)*this->begin();
                auto t = tuple<Alloc>(&term, this->size(), a_alloc);
                return eterm<Alloc>(t);
            }

            eterm<Alloc> to_list(Alloc& a_alloc) {
                eterm<Alloc>& term = (eterm<Alloc>&)*this->begin();
                auto t = list<Alloc>(&term, this->size(), a_alloc);
                return eterm<Alloc>(t);
            }

            const eterm<Alloc>& operator[] (size_t idx) const {
                return (const eterm<Alloc>&)*(base::begin()+idx);
            }

            const eterm<Alloc>& back() const {
                return (const eterm<Alloc>&)*(base::end()-1);
            }
        };

    } // namespace

    static void skip_ws_and_comments(const char** fmt) {
        bool inside_comment = false;
        for(char c = **fmt; c; c = *(++(*fmt))) {
            if (inside_comment) {
                if (c == '\n') inside_comment = false;
                continue;
            } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                continue;
            } else if (c == '%') {
                inside_comment = true;
                continue;
            }
            break;
        }
    }

    static inline var pvariable(const char **fmt)
    {
        const char* start = *fmt, *p = start;
        char c;
        uintptr_t len;

        skip_ws_and_comments(fmt);

        for (c = *p; c && (isalnum((int)c) || (c == '_')); c = *(++p));

        const char* end = p;

        eterm_type type;

        // TODO: Add recursive type checking
        // (i.e. A :: [{atom(), [integer() | string() | double() | tuple()]}])
        if (c == ':' && *(p+1) == ':') {
            p += 2;

            const char* tps = p;

            for (c = *p; c && isalnum((int)c); c = *(++p));

            if (c == '(' && *(p+1) == ')') {
                type = type_string_to_type(tps, static_cast<uintptr_t>(p - tps));
                if (type == UNDEFINED)
                    throw err_format_exception("Error parsing variable type", start);
                p += 2;
            } else
                throw err_format_exception("Invalid variable type", tps);
        } else
            type = UNDEFINED;

        *fmt = p;
        len = static_cast<uintptr_t>(end - start);

        return var(start, len, type);

    } /* pvariable */

    static inline atom patom(const char **fmt)
    {
        skip_ws_and_comments(fmt);

        const char* start = *fmt, *p = start;

        for(char c = *p; c && (isalnum(c) || c == '_' || c == '@'); c = *(++p));

        *fmt = p;

        return atom(start, p - start);
    } /* patom */

    static inline atom pquotedatom(const char **fmt)
    {
        ++(*fmt); /* skip first quote */
        //skip_ws_and_comments(fmt);

        const char* start = *fmt, *p = start;

        for (char c = *p; c && (c != '\'' || *(p-1) == '\\') ; c = *(++p));

        if (*p != '\'')
            throw err_format_exception("Error parsing quotted atom", start);

        *fmt = p+1; /* skip last quote */
        uintptr_t len = static_cast<uintptr_t>(p - start);

        return atom(start, len);

    } /* pquotedatom */

    /* Check if integer or float
     */
    template <class Alloc>
    static eterm<Alloc> pdigit(const char **fmt)
    {
        const char* start = *fmt, *p = start;
        bool dotp = false;
        int  base = 10;

        skip_ws_and_comments(fmt);

        if (*p == '-') p++;

        for (char c = *p; c; c = *(++p)) {
            if (isdigit(c))
                continue;
            else if (c == '.' && !dotp)
                dotp = true;
            else if (c == '#') {
                long b = strtol(start, NULL, 10);
                BOOST_ASSERT(b <= INT_MAX);
                base  = (int)b;
                start = p+1;
            } else
                break;
        }

        *fmt = p;
        if (dotp) {
            auto d = strtod(start, NULL);
            return d;
        }
        auto n = strtol(start, NULL, base);
        return n;
    } /* pdigit */

    template <class Alloc>
    static eterm<Alloc> pstring(const char **fmt, Alloc& alloc)
    {
        const char* start = ++(*fmt); /* skip first quote */
        const char* p = start;

        // skip_ws_and_comments(fmt);

        for (char c = *p; c && (c != '"' || *(p-1) == '\\'); c = *(++p));

        if (*p != '"')
            throw err_format_exception("Error parsing string", start);

        *fmt = p+1; /* skip last quote */
        uintptr_t len = static_cast<uintptr_t>(p - start);

        return eterm<Alloc>(string<Alloc>(start, len, alloc));
    } /* pstring */


    /**
     * Convert a string with variables into an Erlang term.
     *
     * The format letters are:
     *   @li a  -  An atom
     *   @li s  -  A string
     *   @li i  -  An integer
     *   @li l  -  A long integer
     *   @li u  -  An unsigned long integer
     *   @li f  -  A double float
     *   @li w  -  A pointer to some arbitrary term
     *   @li v  -  A pointer to an Erlang variable
     *
     * @todo Implement support for parsing tail list expressions in the form
     *       "[H | T]".  Currently the pipe notation doesn't work.
     */
    template <class Alloc>
    static eterm<Alloc> pformat(const char** fmt, va_list* pap, Alloc& a_alloc)
    {
        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {
            case 'v': return *va_arg(*pap, var*);
            case 'w': return *va_arg(*pap, eterm<Alloc>*);
            case 'a': return atom(va_arg(*pap, char*));
            case 's': return string<Alloc>(va_arg(*pap, char*), a_alloc);
            case 'i': return va_arg(*pap, int);
            case 'l': return va_arg(*pap, long);
            case 'u': return (long)va_arg(*pap, unsigned long);
            case 'f': return va_arg(*pap, double);
            default:  throw err_format_exception("Error parsing string", *fmt-1);
        }
    } /* pformat */

    template <class Alloc>
    static bool ptuple(const char** fmt, va_list* pap,
                      vector<Alloc>& v, Alloc& a_alloc)
    {
        bool res = false;

        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {

            case '}':
                res = true;
                break;

            case ',':
                res = ptuple(fmt, pap, v, a_alloc);
                break;

            default: {
                (*fmt)--;
                v.push_back(eformat(fmt, pap, a_alloc));
                if (v.back().type() != UNDEFINED)
                    res = ptuple(fmt, pap, v, a_alloc);
                break;
            }

        } /* switch */

        return res;

    } /* ptuple */

    template <class Alloc>
    static bool plist(const char** fmt, va_list* pap,
                     vector<Alloc>& v, Alloc& a_alloc)
    {
        bool res = false;

        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {

            case ']':
                res = true;
                break;

            case ',':
                res = plist(fmt, pap, v, a_alloc);
                break;

            case '|':
                skip_ws_and_comments(fmt);
                if (isupper((int)**fmt) || (**fmt == '_')) {
                    var a = pvariable(fmt);
                    v.push_back(eterm<Alloc>(a));
                    skip_ws_and_comments(fmt);
                    if (**fmt == ']')
                        res = true;
                    break;
                }
                break;

            default: {
                (*fmt)--;
                auto et = eformat(fmt, pap, a_alloc);
                if (et.type() != UNDEFINED) {
                    v.push_back(et);
                    res = plist(fmt, pap, v, a_alloc);
                }
                break;
            }

        } /* switch */

        return res;

    } /* plist */

    template <class Alloc>
    static eterm<Alloc> eformat(const char** fmt, va_list* pap, const Alloc& a_alloc)
    {
        vector<Alloc> v(a_alloc);
        Alloc alloc(a_alloc);
        eterm<Alloc> ret;

        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {
            case '{': {
                if (!ptuple(fmt, pap, v, alloc))
                    throw err_format_exception("Error parsing tuple", *fmt);
                ret = v.to_tuple(alloc);
                break;
            }
            case '[':
                if (**fmt == ']') {
                    (*fmt)++;
                    ret = v.to_list(alloc);
                } else if (!plist(fmt, pap, v, alloc))
                    throw err_format_exception("Error parsing list", *fmt);
                ret = v.to_list(alloc);
                break;

            case '<': {
                if (**fmt != '<')
                    throw err_format_exception("Error parsing binary", *fmt);
                bool str   = *(*fmt + 1) == '"';
                if (str) {
                    *fmt += 2;
                    const char* end = strstr(*fmt, "\">>");
                    if (!end)
                        throw err_format_exception("Cannot find end of binary", *fmt);
                    ret = eterm<Alloc>(binary<Alloc>(*fmt, static_cast<uintptr_t>(end - *fmt), alloc));
                    *fmt = end + 3;
                } else {
                    const char* end = strstr(++(*fmt), ">>");
                    if (!end)
                        throw err_format_exception("Cannot find end of binary", *fmt);
                    std::vector<char> v2;
                    auto p = *fmt;

                    while (p < end) {
                        while (*p == ' ' || *p == '\t') ++p;
                        int byte;
                        auto q = fast_atoi<int, false>(p, end, byte);
                        if (!q)
                            throw err_format_exception("Error parsing binary", p);
                        p = q;
                        if (byte < 0 || byte > 255)
                            throw err_format_exception("Invalid byte value in binary", p);
                        v2.push_back((char)byte);
                        while (*p == ' ' || *p == '\t') ++p;
                        if (*p == ',')
                            ++p;
                        else if (p < end)
                            throw err_format_exception("Invalid byte delimiter in binary", p);
                    }
                    auto begin = &v2[0];
                    ret = eterm<Alloc>(binary<Alloc>(begin, v2.size(), alloc));
                    *fmt = end + 2;
                }
                break;
            }

            case '$': /* char-value? */
                ret = eterm<Alloc>((int)(*(*fmt)++));
                break;

            case '~':
                ret = pformat(fmt, pap, alloc);
                break;

            default: {
                (*fmt)--;
                if (islower(**fmt))         /* atom ? */
                    ret = patom(fmt);
                else if (isupper(**fmt) || (**fmt == '_'))
                    ret = pvariable(fmt);
                else if (isdigit(**fmt) || **fmt == '-') /* int|float ? */
                    ret = pdigit<Alloc>(fmt);
                else if (**fmt == '"')      /* string ? */
                    ret = pstring(fmt, alloc);
                else if (**fmt == '\'')     /* quoted atom ? */
                    ret = pquotedatom(fmt);
                break;
            }
        }

        if (ret.empty())
            throw err_format_exception("invalid term", *fmt);

        return ret;

    } /* eformat */

    template <class Alloc>
    static void eformat(atom& mod, atom& fun, eterm<Alloc>& args,
                        const char** fmt, va_list* pap, const Alloc& a_alloc)
    {
        Alloc alloc(a_alloc);
        vector<Alloc> v(alloc);

        skip_ws_and_comments(fmt);

        const char* start = *fmt, *p = start;
        const char* q = strchr(p, ':');

        if (!q)
            throw err_format_exception("Module name not found", p);

        mod = atom(p, q - p);

        p = q+1;

        q = strchr(p, '(');

        if (!q)
            throw err_format_exception("Function name not found", p);

        fun = atom(p, q - p);

        p = q+1;

        skip_ws_and_comments(&p);

        if (*p == '\0')
            throw err_format_exception("Invalid argument syntax", p);
        else if (*p == ')')
            ++p;
        else while(true) {
            v.push_back(eformat(&p, pap, a_alloc));
            skip_ws_and_comments(&p);

            char c = *p++;

            if (c == '\0')
                throw err_format_exception("Arguments list not closed", &c);
            else if (c == ')')
                break;
            if (c != ',')
                throw err_format_exception("Arguments must be comma-delimited", &c);
        }

        skip_ws_and_comments(&p);

        if (*p != '\0') {
            if (*p == '.') {
                ++p; skip_ws_and_comments(&p);
            }
        }

        if (*p != '\0')
            throw err_format_exception("Invalid MFA format", p);

        args = v.to_list(alloc);
    }

} // namespace marshal
} // namespace eixx

