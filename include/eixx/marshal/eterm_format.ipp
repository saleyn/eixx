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
//#include <boost/scope_exit.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

    template <typename Alloc>
    struct vector : public std::vector<eterm<Alloc>, Alloc> {
        explicit vector(const Alloc& a_alloc = Alloc())
            : std::vector<eterm<Alloc>, Alloc> (a_alloc)
        {}
    };

    enum {
          ERL_OK                = 0
        , ERL_FMT_ERR           = -1
        , ERL_MAX_ENTRIES       = 255  /* Max entries in a tuple/list term */
        , ERL_MAX_NAME_LENGTH   = 255  /* Max length of variable names */
    };

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

    static var pvariable(const char **fmt)
    {
        const char* start = *fmt, *p = start;
        char c;
        int len;

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
                type = type_string_to_type(tps, p - tps);
                if (type == UNDEFINED)
                    throw err_format_exception("Error parsing variable type", start);
                p += 2;
            } else
                throw err_format_exception("Invalid variable type", tps);
        } else
            type = UNDEFINED;

        *fmt = p;
        len = end - start;

        return var(start, len, type);

    } /* pvariable */

    static char *patom(const char **fmt, char *buf)
    {
        const char* start = *fmt;
        char c;
        int len;

        skip_ws_and_comments(fmt);

        while (1) {
            c = *(*fmt)++;
            if (isalnum((int) c) || (c == '_') || (c == '@'))
                continue;
            else
                break;
        }
        (*fmt)--;
        len = *fmt - start;
        memcpy(buf, start, len);
        buf[len] = 0;

        return buf;

    } /* patom */

    /* Check if integer or float
     */
    static char *pdigit(const char **fmt, char *buf)
    {
        const char* start = *fmt;
        char c;
        int len,dotp=0;

        skip_ws_and_comments(fmt);

        while (1) {
            c = *(*fmt)++;
            if (isdigit((int) c) || c == '-')
                continue;
            else if (!dotp && (c == '.')) {
                dotp = 1;
                continue;
            }
            else
                break;
        }
        (*fmt)--;
        len = *fmt - start;
        memcpy(buf, start, len);
        buf[len] = 0;

        return buf;

    } /* pdigit */

    static char *pstring(const char **fmt, char *buf)
    {
        const char* start = ++(*fmt); /* skip first quote */
        char c;
        int len;

        // skip_ws_and_comments(fmt);

        while (1) {
            c = *(*fmt)++;
            if (c == '"') {
                if (*((*fmt)-1) == '\\')
                    continue;
                else
                    break;
            } else
                continue;
        }
        len = *fmt - 1 - start; /* skip last quote */
        memcpy(buf, start, len);
        buf[len] = 0;

        return buf;

    } /* pstring */

    static char *pquotedatom(const char **fmt, char *buf)
    {
        const char* start = ++(*fmt); /* skip first quote */
        char c;
        int len;

        skip_ws_and_comments(fmt);

        while (1) {
            c = *(*fmt)++;
            if (c == '\'') {
                if (*((*fmt)-1) == '\\')
                    continue;
                else
                    break;
            } else
                continue;
        }
        len = *fmt - 1 - start; /* skip last quote */
        memcpy(buf, start, len);
        buf[len] = 0;

        return buf;

    } /* pquotedatom */


    /// @todo Implement support for parsing tail list expressions in the form
    ///       "[H | T]".  Currently the pipe notation doesn't work.
    /*
     * The format letters are:
     *   a  -  An atom
     *   s  -  A string
     *   i  -  An integer
     *   l  -  A long integer
     *   u  -  An unsigned long integer
     *   f  -  A double float
     *   w  -  A pointer to some arbitrary term
     */
    template <class Alloc>
    static int pformat(const char** fmt, va_list* pap, 
                       vector<Alloc>& v, Alloc& a_alloc)
    {
        int rc=ERL_OK;

        /* this next section hacked to remove the va_arg calls */
        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {
            case 'w':
                v.push_back(*va_arg(*pap, eterm<Alloc>*));
                break;

            case 'a':
                v.push_back(eterm<Alloc>(atom(va_arg(*pap, char*))));
                break;

            case 's':
                v.push_back(eterm<Alloc>(string<Alloc>(va_arg(*pap, char*), a_alloc)));
                break;

            case 'i':
                v.push_back(eterm<Alloc>(va_arg(*pap, int)));
                break;

            case 'l':
                v.push_back(eterm<Alloc>(va_arg(*pap, long)));
                break;

            case 'u':
                v.push_back(eterm<Alloc>((long)va_arg(*pap, unsigned long)));
                break;

            case 'f':
                v.push_back(eterm<Alloc>(va_arg(*pap, double)));
                break;

            default:
                rc = ERL_FMT_ERR;
                break;
        }

        return rc;

    } /* pformat */

    template <class Alloc>
    static int ptuple(const char** fmt, va_list* pap, 
                      vector<Alloc>& v, Alloc& a_alloc)
    {
        int res=ERL_FMT_ERR;

        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {

            case '}':
                res = ERL_OK;
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

              /*
                if (isupper(**fmt)) {
                v[size++] = erl_mk_var(pvariable(fmt, wbuf));
                res = ptuple(fmt, pap, v);
                }
                else if ((v[size++] = eformat(fmt, pap)) != (ErlTerm *) NULL)
                res = ptuple(fmt, pap, v);
                break;
              */
            }

        } /* switch */

        return res;

    } /* ptuple */

    template <class Alloc>
    static int plist(const char** fmt, va_list* pap,
                     vector<Alloc>& v, Alloc& a_alloc)
    {
        int res=ERL_FMT_ERR;

        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {

            case ']':
                res = ERL_OK;
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
                        res = ERL_OK;
                    break;
                }
                break;

            default: {
                (*fmt)--;
                eterm<Alloc> et = eformat(fmt, pap, a_alloc);
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
        throw (err_format_exception)
    {
        vector<Alloc> v(a_alloc);
        Alloc alloc(a_alloc);
        eterm<Alloc> ret;

        skip_ws_and_comments(fmt);

        switch (*(*fmt)++) {
            case '{':
                if (ptuple(fmt, pap, v, alloc) != ERL_OK)
                    throw err_format_exception("Error parsing tuple", *fmt);
                ret.set( eterm<Alloc>(tuple<Alloc>(&v[0], v.size(), alloc)) );
                break;

            case '[':
                if (**fmt == ']') {
                    (*fmt)++;
                    ret.set( eterm<Alloc>(list<Alloc>(0, alloc)) );
                } else if (plist(fmt, pap, v, alloc) == ERL_OK) {
                    ret.set( eterm<Alloc>(list<Alloc>(&v[0], v.size(), alloc)) );
                } else {
                    throw err_format_exception("Error parsing list", *fmt);
                }
                break;

            case '$': /* char-value? */
                ret.set( eterm<Alloc>((int)(*(*fmt)++)) );
                break;

            case '~':
                if (pformat(fmt, pap, v, alloc) != ERL_OK)
                    throw err_format_exception("Error parsing term", *fmt);
                ret.set(v[0]);
                break;

            default: {
                char wbuf[BUFSIZ];  /* now local to this function for reentrancy */

                (*fmt)--;
                if (islower((int)**fmt)) {         /* atom  ? */
                    char* a = patom(fmt, wbuf);
                    ret.set( eterm<Alloc>(atom(a)) );
                } else if (isupper((int)**fmt) || (**fmt == '_')) {
                    var v = pvariable(fmt);
                    ret.set( eterm<Alloc>(v) );
                } else if (isdigit((int)**fmt) || **fmt == '-') {    /* integer/float ? */
                    char* digit = pdigit(fmt, wbuf);
                    if (strchr(digit,(int) '.') == NULL)
                        ret.set( eterm<Alloc>(atoi((const char *) digit)) );
                    else
                        ret.set( eterm<Alloc>(atof((const char *) digit)) );
                } else if (**fmt == '"') {      /* string ? */
                    char* str = pstring(fmt, wbuf);
                    ret.set( eterm<Alloc>(string<Alloc>(str, alloc)) );
                } else if (**fmt == '\'') {     /* quoted atom ? */
                    char* qatom = pquotedatom(fmt, wbuf);
                    ret.set( eterm<Alloc>(atom(qatom)) );
                }
            }
            break;
        }

        if (ret.empty())
            throw err_format_exception("invalid term", *fmt);

        return ret;

    } /* eformat */


} // namespace marshal
} // namespace EIXX_NAMESPACE

