//----------------------------------------------------------------------------
/// \file  eterm.ipp
//----------------------------------------------------------------------------
/// \brief Implementation of eterm member functions.
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
#include <stdarg.h>
#include <eixx/marshal/visit.hpp>
#include <eixx/marshal/visit_encode_size.hpp>
#include <eixx/marshal/visit_encoder.hpp>
#include <eixx/marshal/visit_to_string.hpp>
#include <eixx/marshal/visit_subst.hpp>
#include <eixx/marshal/visit_match.hpp>
#include <eixx/marshal/eterm_format.hpp>

namespace EIXX_NAMESPACE {

inline eterm_type type_string_to_type(const char* s, size_t n) {
     eterm_type r = UNDEFINED;

     if (n < 3) return r;

     int m = n - 1;
     const char* p = s+1;

     switch (s[0]) {
         case 'i':
             if (strncmp(p,"nt",m) == 0)         r = LONG;
             if (strncmp(p,"nteger",m) == 0)     r = LONG;
             break;
         case 'd':
             if (strncmp(p,"ouble",m) == 0)      r = DOUBLE;
             break;
         case 'f':
             if (strncmp(p,"loat",m) == 0)       r = DOUBLE;
             break;
         case 'b':
             if      (strncmp(p,"ool",m) == 0)   r = BOOL;
             else if (strncmp(p,"inary",m) == 0) r = BINARY;
             else if (strncmp(p,"oolean",m)== 0) r = BOOL;
             else if (strncmp(p,"yte",m) == 0)   r = LONG;
             break;
         case 'c':
             if (strncmp(p,"har",m) == 0)        r = LONG;
             break;
         case 'a':
             if (strncmp(p,"tom",m) == 0)        r = ATOM;
             break;
         case 's':
             if (strncmp(p,"tring",m) == 0)      r = STRING;
             break;
         case 'p':
             if      (strncmp(p,"id",m) == 0)    r = PID;
             else if (strncmp(p,"ort",m) == 0)   r = PORT;
             break;
         case 'r':
             if      (strncmp(p,"ef",m) == 0)       r = REF;
             else if (strncmp(p,"eference",m) == 0) r = REF;
             break;
         case 'v':
             if (strncmp(p,"ar",m) == 0)         r = VAR;
             break;
         case 't':
             if      (strncmp(p,"uple",m) == 0)  r = TUPLE;
             else if (strncmp(p,"race",m) == 0)  r = TRACE;
             break;
         case 'l':
             if (strncmp(p,"ist",m) == 0)        r = LIST;
             break;
         default:
             break;
     }
     return r;
 }

namespace marshal {

template <typename Alloc>
const char* eterm<Alloc>::type_string() const {
    switch (m_type) {
        case UNDEFINED: return "undefined";
        case LONG:      return "long";
        case DOUBLE:    return "double";
        case BOOL:      return "bool";
        case ATOM:      return "atom";
        case STRING:    return "string";
        case BINARY:    return "binary";
        case PID:       return "pid";
        case PORT:      return "port";
        case REF:       return "ref";
        case VAR:       return "var";
        case TUPLE:     return "tuple";
        case LIST:      return "list";
        case TRACE:     return "trace";
    }
    BOOST_STATIC_ASSERT(MAX_ETERM_TYPE == 13);
}

template <typename Alloc>
inline bool eterm<Alloc>::operator== (const eterm<Alloc>& rhs) const {
    if (m_type != rhs.type())
        return false;
    switch (m_type) {
        case LONG:   return vt.i    == rhs.vt.i;
        case DOUBLE: return vt.d    == rhs.vt.d;
        case BOOL:   return vt.b    == rhs.vt.b;
        case ATOM:   return vt.a    == rhs.vt.a;
        case VAR:    return vt.v    == rhs.vt.v;
        case STRING: return vt.s    == rhs.vt.s;
        case BINARY: return vt.bin  == rhs.vt.bin;
        case PID:    return vt.pid  == rhs.vt.pid;
        case PORT:   return vt.prt  == rhs.vt.prt;
        case REF:    return vt.r    == rhs.vt.r;
        case TUPLE:  return vt.t    == rhs.vt.t;
        case LIST:   return vt.l    == rhs.vt.l;
        case TRACE:  return vt.trc  == rhs.vt.trc;
        default: {
            std::stringstream s; s << "Undefined term_type (" << m_type << ')';
            throw err_invalid_term(s.str());
        }
    }
    BOOST_STATIC_ASSERT(MAX_ETERM_TYPE == 13);
}

template <typename Alloc>
std::string eterm<Alloc>::to_string(size_t a_size_limit, const varbind<Alloc>* binding) const {
    if (m_type == UNDEFINED)
        return std::string();
    std::ostringstream out;
    visit_eterm_stringify<Alloc> visitor(out, binding);
    visitor.apply_visitor(*this);
    std::string s = out.str();
    return a_size_limit == std::string::npos
        ? s : s.substr(0, std::min(a_size_limit, s.size()));
}

template <class Alloc>
eterm<Alloc>::eterm(const char* a_buf, size_t a_size, const Alloc& a_alloc)
    throw(err_decode_exception)
{
    int idx = 0;
    int vsn;
    if (ei_decode_version(a_buf, &idx, &vsn) < 0)
        throw err_decode_exception("Wrong eterm version byte!", vsn);
    decode(a_buf, idx, a_size, a_alloc);
}

template <class Alloc>
void eterm<Alloc>::decode(const char* a_buf, int& idx, size_t a_size, const Alloc& a_alloc)
    throw(err_decode_exception)
{
    if ((size_t)idx == a_size)
        throw err_decode_exception("Empty term", idx);

    // check the type of next term:
    int type, sz;

    if (ei_get_type(a_buf, &idx, &type, &sz) < 0)
        throw err_decode_exception("Cannot determine term type", idx);

    switch (type) {
    case ERL_ATOM_EXT: {
        int b;
        int i = idx; // TODO: Eliminate this variable when there's is a fix for the bug in ei_decode_boolean
        if (ei_decode_boolean(a_buf, &i, &b) < 0)
            new (this) eterm<Alloc>(atom(a_buf, idx, a_size));
        else {
            idx = i;
            new (this) eterm<Alloc>((bool)b);
        }
        break;
    }
    case ERL_LARGE_TUPLE_EXT:
    case ERL_SMALL_TUPLE_EXT: {
        new (this) eterm<Alloc>(tuple<Alloc>(a_buf, idx, a_size, a_alloc));
        break;
    }
    case ERL_STRING_EXT:
        new (this) eterm<Alloc>(string<Alloc>(a_buf, idx, a_size, a_alloc));
        break;

    case ERL_LIST_EXT:
    case ERL_NIL_EXT: {
        new (this) eterm<Alloc>(list<Alloc>(a_buf, idx, a_size, a_alloc));
        break;
    }
    case ERL_SMALL_INTEGER_EXT:
    case ERL_SMALL_BIG_EXT:
    case ERL_LARGE_BIG_EXT:
    case ERL_INTEGER_EXT: {
        long long l;
        if (ei_decode_longlong(a_buf, &idx, &l) < 0)
            throw err_decode_exception("Failed decoding long value", idx);
        new (this) eterm<Alloc>((long)l);
        break;
    }
    case NEW_FLOAT_EXT:
    case ERL_FLOAT_EXT: {
        double d;
        if (ei_decode_double(a_buf, &idx, &d) < 0)
            throw err_decode_exception("Failed decoding double value", idx);
        new (this) eterm<Alloc>(d);
        break;
    }
    case ERL_BINARY_EXT:
        new (this) eterm<Alloc>(binary<Alloc>(a_buf, idx, a_size, a_alloc));
        break;

    case ERL_PID_EXT:
        new (this) eterm<Alloc>(epid<Alloc>(a_buf, idx, a_size, a_alloc));
        break;

    case ERL_REFERENCE_EXT:
    case ERL_NEW_REFERENCE_EXT:
        new (this) eterm<Alloc>(ref<Alloc>(a_buf, idx, a_size, a_alloc));
        break;

    case ERL_PORT_EXT:
        new (this) eterm<Alloc>(port<Alloc>(a_buf, idx, a_size, a_alloc));
        break;

    default:
        std::ostringstream oss;
        oss << "Unknown message content type " << type;
        throw err_decode_exception(oss.str());
        break;
    }
}

template <typename Alloc>
size_t eterm<Alloc>::encode_size(size_t a_header_size, bool a_with_version) const 
{
    BOOST_ASSERT(m_type != UNDEFINED);
    size_t n = visit_eterm_encode_size_calc<Alloc>().apply_visitor(*this);
    return a_header_size + n + (a_with_version ? 1 : 0);
}

template <typename Alloc>
string<Alloc> eterm<Alloc>::encode(size_t a_header_size, bool a_with_version) const 
{
    size_t size = encode_size(a_header_size, a_with_version);
    string<Alloc> out(NULL, size);
    char* p = const_cast<char*>(out.c_str());
    encode(p, size, a_header_size, a_with_version);
    return out;
}

template <typename Alloc>
void eterm<Alloc>::encode(char* a_buf, size_t size, 
    size_t a_header_size, bool a_with_version) const throw (err_encode_exception)
{
    #if BOOST_VERSION >= 104900
    namespace bd = boost::spirit::detail;
    #else
    namespace bd = boost::detail;
    #endif

    BOOST_ASSERT(size > 0);
    size_t msg_sz = size - a_header_size;
    switch (a_header_size) {
        case 0:
            break;
        case 1:
            bd::store_big_endian<uint8_t, 1>(a_buf, msg_sz);
            break;
        case 2:
            bd::store_big_endian<uint16_t, 2>(a_buf, msg_sz);
            break;
        case 4:
            bd::store_big_endian<uint32_t, 4>(a_buf, msg_sz);
            break;
        default: {
            std::stringstream s;
            s << "Bad header size: " << a_header_size;
            throw err_encode_exception(s.str());
        }
    }
    int offset = a_header_size;
    if (a_with_version)
        ei_encode_version(a_buf, &offset);
    visit_eterm_encoder visitor(a_buf, offset, size);
    visitor.apply_visitor(*this);
    BOOST_ASSERT((size_t)offset == size);
}

template <class Alloc>
bool eterm<Alloc>::match(
    const eterm<Alloc>& pattern,
    varbind<Alloc>* binding,
    const Alloc& a_alloc) const
    throw (err_unbound_variable)
{
    // Protect the given binding. Change it only if the match succeeds.
    varbind<Alloc> dirty(a_alloc);
    if (!binding) {
        visit_eterm_match<Alloc> visitor(pattern, &dirty);
        if (!visitor.apply_visitor(*this))
            return false;
    } else {
        dirty.copy(*binding);
        visit_eterm_match<Alloc> visitor(pattern, &dirty);
        if (!visitor.apply_visitor(*this))
            return false;
        binding->merge(dirty);
    }
    return true;
}

template <typename Alloc>
bool eterm<Alloc>::subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
    throw (err_invalid_term, err_unbound_variable)
{
    visit_eterm_subst<Alloc> visitor(out, binding);
    return visitor.apply_visitor(*this);
}

template <typename Alloc>
eterm<Alloc> eterm<Alloc>::apply(const varbind<Alloc>& binding) const
    throw (err_invalid_term, err_unbound_variable)
{
    eterm<Alloc> out;
    visit_eterm_subst<Alloc> visitor(out, &binding);
    static const eterm<Alloc> s_null;
    return visitor.apply_visitor(*this) ? out : s_null;
}

template <class Alloc>
eterm<Alloc> eterm<Alloc>::format(const Alloc& a_alloc, const char** fmt, va_list* pap)
    throw (err_format_exception)
{
    try {
        return eformat<Alloc>(fmt, pap, a_alloc);
    } catch (err_format_exception& e) {
        e.start(*fmt);
        throw;
    } catch (...) {
        throw err_format_exception("Error parsing expression", *fmt, *fmt);
    }
}

template <class Alloc>
void eterm<Alloc>::format(const Alloc& a_alloc, atom& m, atom& f, eterm<Alloc>& args,
    const char** fmt, va_list* pap) throw (err_format_exception)
{
    try {
        eformat<Alloc>(m, f, args, fmt, pap, a_alloc);
    } catch (err_format_exception& e) {
        e.start(*fmt);
        throw;
    } catch (...) {
        throw err_format_exception("Error parsing expression", *fmt, *fmt);
    }
}

template <class Alloc>
eterm<Alloc> eterm<Alloc>::format(const Alloc& a_alloc, const char* fmt, ...)
    throw (err_format_exception)
{
    va_list ap;
    va_start(ap, fmt);
    try { return format(a_alloc, &fmt, &ap); } catch (...) { va_end(ap); throw; }
    va_end(ap);
}

template <class Alloc>
eterm<Alloc> eterm<Alloc>::format(const char* fmt, ...)
    throw (err_format_exception)
{
    va_list ap;
    va_start(ap, fmt);
    try { return format(Alloc(), &fmt, &ap); } catch (...) { va_end(ap); throw; }
    va_end(ap);
}

template <class Alloc>
void eterm<Alloc>::format(const Alloc& a_alloc, atom& m, atom& f, eterm<Alloc>& args,
    const char* fmt, ...) throw (err_format_exception)
{
    va_list ap;
    va_start(ap, fmt);
    try { format(a_alloc, m, f, args, &fmt, &ap); } catch (...) { va_end(ap); throw; }
    va_end(ap);
}
template <class Alloc>
void eterm<Alloc>::format(atom& m, atom& f, eterm<Alloc>& args, const char* fmt, ...)
    throw (err_format_exception)
{
    va_list ap;
    va_start(ap, fmt);
    try { format(Alloc(), m, f, args, &fmt, &ap); } catch (...) { va_end(ap); throw; }
    va_end(ap);
}

} // namespace marshal
} // namespace EIXX_NAMESPACE
