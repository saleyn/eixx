//----------------------------------------------------------------------------
/// \file  eterm.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing polymorphic Erlang term capable of storing
///        various data types.
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
#ifndef _EIXX_MARSHAL_ETERM_HPP_
#define _EIXX_MARSHAL_ETERM_HPP_

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/noncopyable.hpp>

#include <initializer_list>

#include <eixx/marshal/defaults.hpp> // Must be included before any <eixx/impl/*>

#include <eixx/marshal/atom.hpp>
#include <eixx/marshal/string.hpp>
#include <eixx/marshal/binary.hpp>
#include <eixx/marshal/pid.hpp>
#include <eixx/marshal/port.hpp>
#include <eixx/marshal/ref.hpp>
#include <eixx/marshal/tuple.hpp>
#include <eixx/marshal/list.hpp>
#include <eixx/marshal/trace.hpp>
#include <eixx/marshal/var.hpp>
#include <eixx/marshal/varbind.hpp>
#include <eixx/marshal/eterm_match.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

namespace {
    template <typename T, typename Alloc> struct enum_type;
    template <typename Alloc> struct enum_type<long,   Alloc>        { typedef long   type; };
    template <typename Alloc> struct enum_type<double, Alloc>        { typedef double type; };
    template <typename Alloc> struct enum_type<bool,   Alloc>        { typedef bool   type; };
    template <typename Alloc> struct enum_type<atom,   Alloc>        { typedef atom   type; };
    template <typename Alloc> struct enum_type<string<Alloc>, Alloc> { typedef string<Alloc> type; };
    template <typename Alloc> struct enum_type<binary<Alloc>, Alloc> { typedef binary<Alloc> type; };
    template <typename Alloc> struct enum_type<epid<Alloc>,   Alloc> { typedef epid<Alloc>   type; };
    template <typename Alloc> struct enum_type<port<Alloc>,   Alloc> { typedef port<Alloc>   type; };
    template <typename Alloc> struct enum_type<ref<Alloc>,    Alloc> { typedef ref<Alloc>    type; };
    template <typename Alloc> struct enum_type<var,    Alloc>        { typedef var type; };
    template <typename Alloc> struct enum_type<tuple<Alloc>,  Alloc> { typedef tuple<Alloc>  type; };
    template <typename Alloc> struct enum_type<list<Alloc>,   Alloc> { typedef list<Alloc>   type; };
    template <typename Alloc> struct enum_type<trace<Alloc>,  Alloc> { typedef trace<Alloc>  type; };
}

/**
 * @brief Polymorphic type representing an arbitrary Erlang term.
 *
 * A eterm is always readonly, but when it represents a list or
 * tuple it can be partially initialized.
 * eterm initialization is not thread safe!
 *
 * eterm is a very lightweight structure of size equal to two
 * longs.  The first one represents term type and the second one
 * either contains the value or a pointer to a compound reference
 * counted value.  All eterms are copy constructed on stack, therefore
 * there's no need to create pointers to eterms as copying them is
 * a very fast operation due to their small size.  The underlying
 * storage for compound terms is allocated on heap using user-provided
 * allocator.  The reference counting assures proper resource
 * reclamations when more than one eterm points to the same underlying
 * value.  This reference counting is thread safe using atomic
 * operations.
 *
 * Generally eterm objects are immutable once they are initialized.
 * An exception is a tuple and a list which you can update when
 * you are using the eterm in some local scope and need to
 * reuse its shape by plugging in different values. An example is
 * to use a static variable to hold a compiled eterm produced by the
 * format function, and encode it into binary on each code pass:
 *
 * <code>
 *      static eterm_t tuple =
 *          eterm_t::format("{test, ~i, ~d}", 1, 2.0);
 *      for (int i=1; i <= 10; i++) {
 *          tuple[1] = eterm_t(i);
 *          tuple[2] = eterm_t(2.0*i);
 *          string_t buf = tuple.encode();
 *          send_to_client(buf);
 *          ...
 *      }
 * </code>
 *
 * While this is a convenience shortcut representing performance
 * optimization, we discourage using this method in general cases
 * as mutability is evil. Consequently, favor this implementation
 * that treats tuple as an immutable object and recreates it
 * on every iteration.  This approach is thread safe when
 * passing eterm objects between threads:
 *
 * <code>
 *      for (int i=1; i <= 10; i++) {
 *          eterm_t tuple =
 *              eterm_t::format("{test, ~i, ~d}", i, 2.0*i);
 *          ...
 *      }
 * </code>
 */
template <typename Alloc>
class eterm {
    eterm_type m_type;

    union vartype {
        double          d;
        bool            b;
        long            i;
        atom            a;
        var             v;
        string<Alloc>   s;
        binary<Alloc> bin;
        epid<Alloc>   pid;
        port<Alloc>   prt;
        ref<Alloc>      r;
        tuple<Alloc>    t;
        list<Alloc>     l;
        trace<Alloc>  trc;

        uint64_t value; // this is for ease of copying

        // We ensure that the size of each compound type
        // is sizeof(uint64_t).  Therefore it's safe to store the actual
        // value of a compound type in this union, so the uint64 integer
        // acts as the value placeholder.
        // This allows us to have the minimum overhead related to
        // copying terms as for simple types it merely involves copying
        // 16 bytes and for compound types it means copying
        // the same 16 bytes and in some cases
        // incrementing compound type's reference count.
        // This approach was tested against boost::variant<> and was found
        // to be several times more efficient.

        vartype(int     x)  : i(x) {}
        vartype(long    x)  : i(x) {}
        vartype(double  x)  : d(x) {}
        vartype(bool    x)  : b(x) {}
        vartype(atom    x)  : a(x) {}
        vartype(var     x)  : v(x) {}
        vartype(const string<Alloc>& x) : s(x)   {}
        vartype(const binary<Alloc>& x) : bin(x) {}
        vartype(const epid<Alloc>&   x) : pid(x) {}
        vartype(const port<Alloc>&   x) : prt(x) {}
        vartype(const ref<Alloc>&    x) :   r(x) {}
        vartype(const tuple<Alloc>&  x) :   t(x) {}
        vartype(const list<Alloc>&   x) :   l(x) {}
        vartype(const trace<Alloc>&  x) : trc(x) {}

        vartype() : i(0) {}
        ~vartype() {}

        void reset() { i = 0; }
    } vt;

    BOOST_STATIC_ASSERT(sizeof(vartype) == sizeof(uint64_t));

    void check(eterm_type tp) const { if (unlikely(m_type != tp)) throw err_wrong_type(tp, m_type); }

    /**
     * Decode a term from the Erlang external binary format.
     */
    void decode(const char* a_buf, int& idx, size_t a_size, const Alloc& a_alloc)
        throw (err_decode_exception);

    long&           get(long*)                  { check(LONG);   return vt.i; }
    double&         get(double*)                { check(DOUBLE); return vt.d; }
    bool&           get(bool*)                  { check(BOOL);   return vt.b; }
    atom&           get(const atom*)            { check(ATOM);   return vt.a; }
    var&            get(const var*)             { check(VAR);    return vt.v; }
    string<Alloc>&  get(const string<Alloc>*)   { check(STRING); return vt.s; }
    binary<Alloc>&  get(const binary<Alloc>*)   { check(BINARY); return vt.bin; }
    epid<Alloc>&    get(const epid<Alloc>*)     { check(PID);    return vt.pid; }
    port<Alloc>&    get(const port<Alloc>*)     { check(PORT);   return vt.prt; }
    ref<Alloc>&     get(const ref<Alloc>*)      { check(REF);    return vt.r; }
    tuple<Alloc>&   get(const tuple<Alloc>*)    { check(TUPLE);  return vt.t; }
    list<Alloc>&    get(const list<Alloc>*)     { check(LIST);   return vt.l; }
    trace<Alloc>&   get(const trace<Alloc>*)    { check(TRACE);  return vt.trc; }

    template <typename T, typename A> friend T& get(eterm<A>& t);

    void replace(eterm* a) {
        m_type    = a->m_type;
        vt.value  = a->vt.value;
        a->m_type = UNDEFINED;
        a->vt.reset();
    }

    static eterm<Alloc> format(const Alloc& a_alloc, const char** fmt, va_list* args)
        throw (err_format_exception);

    static void format(const Alloc& a_alloc, atom& m, atom& f, eterm<Alloc>& args,
        const char** fmt, va_list* pa) throw (err_format_exception);

public:
    eterm_type  type()        const { return m_type; }
    const char* type_string() const;

    eterm() : m_type(UNDEFINED) {}

    eterm(unsigned int  a)          : m_type(LONG),  vt((int)a)  {}
    eterm(unsigned long a)          : m_type(LONG),  vt((long)a) {}
    eterm(int    a)                 : m_type(LONG),  vt(a) {}

    eterm(long   a)                 : m_type(LONG),  vt(a) {}
    eterm(double a)                 : m_type(DOUBLE),vt(a) {}
    eterm(bool   a)                 : m_type(BOOL),  vt(a) {}
    eterm(atom   a)                 : m_type(ATOM),  vt(a) {}
    eterm(var    a)                 : m_type(VAR),   vt(a) {}
    eterm(const char* a, const Alloc& alloc = Alloc())
        : m_type(STRING), vt(string<Alloc>(a, alloc)) {}
    eterm(const std::string& a, const Alloc& alloc = Alloc())
        : m_type(STRING), vt(string<Alloc>(a.c_str(), a.size(), alloc)) {}
    eterm(const string<Alloc>& a)  : m_type(STRING), vt(a) {}
    eterm(const binary<Alloc>& a)  : m_type(BINARY), vt(a) {}
    eterm(const epid<Alloc>& a)    : m_type(PID),    vt(a) {}
    eterm(const port<Alloc>& a)    : m_type(PORT),   vt(a) {}
    eterm(const ref<Alloc>& a)     : m_type(REF),    vt(a) {}
    eterm(const tuple<Alloc>& a)   : m_type(TUPLE),  vt(a) {}
    eterm(const list<Alloc>&  a)   : m_type(LIST),   vt(a) {}
    eterm(const trace<Alloc>& a)   : m_type(TRACE),  vt(a) {}

    /**
     * Tuple initialization
     */
    eterm(std::initializer_list<eterm<Alloc>> items, const Alloc& alloc = Alloc())
        : eterm(tuple<Alloc>(items, alloc)) {}

    /**
     * Construct a term by decoding it from the begining of
     * a binary buffer encoded using Erlang external format.
     * http://www.erlang.org/doc/apps/erts/erl_ext_dist.html
     * @param a_buf is the buffer to decode the term from.
     * @param a_size is the total size of the term stored in \a a_buf buffer.
     * @param a_alloc is the custom allocator.
     */
    eterm(const char* a_buf, size_t a_size, const Alloc& a_alloc = Alloc())
        throw(err_decode_exception);

    /**
     * Construct a term by decoding it from an \a idx offset of the
     * binary buffer \a a_buf encoded using Erlang external format.
     * http://www.erlang.org/doc/apps/erts/erl_ext_dist.html
     * @param a_buf is the buffer to decode the term from.
     * @param idx is the current offset from the start of the buffer.
     * @param a_size is the total size of the term stored in \a a_buf buffer.
     * @param a_alloc is the custom allocator.
     */
    eterm(const char* a_buf, int& idx, size_t a_size, const Alloc& a_alloc = Alloc())
        throw(err_decode_exception) {
        decode(a_buf, idx, a_size, a_alloc);
    }

    /**
     * Copy construct a term from another one. The term is copied by value
     * and for compound terms the storage is reference counted.
     */
    eterm(const eterm& a) : m_type(a.m_type) {
        switch (m_type) {
            case STRING:    { new (&vt.s)   string<Alloc>(a.vt.s);    break; }
            case BINARY:    { new (&vt.bin) binary<Alloc>(a.vt.bin);  break; }
            case PID:       { new (&vt.pid) epid<Alloc>(a.vt.pid);    break; }
            case PORT:      { new (&vt.prt) port<Alloc>(a.vt.prt);    break; }
            case REF:       { new (&vt.r)   ref<Alloc>(a.vt.r);       break; }
            case TUPLE:     { new (&vt.t)   tuple<Alloc>(a.vt.t);     break; }
            case LIST:      { new (&vt.l)   list<Alloc>(a.vt.l);      break; }
            case TRACE:     { new (&vt.trc) trace<Alloc>(a.vt.trc);   break; }
            default:
                vt.value = a.vt.value;
        }
    }

    /**
     * Move constructor
     */
#if __cplusplus >= 201103L
    eterm(eterm&& a) {
        replace(&a);
    }
#endif

    /**
     * Destruct this term. For compound terms it decreases the
     * reference count of their storage. This does nothing to
     * simple terms (e.g. long, double, bool).
     */
    ~eterm() {
        switch (m_type) {
            //No need to destruct atoms - they are stored in global atom table.
            case STRING: { vt.s.~string();   return; }
            case BINARY: { vt.bin.~binary(); return; }
            case PID:    { vt.pid.~epid();   return; }
            case PORT:   { vt.prt.~port();   return; }
            case REF:    { vt.r.~ref();      return; }
            case TUPLE:  { vt.t.~tuple();    return; }
            case LIST:   { vt.l.~list();     return; }
            case TRACE:  { vt.trc.~trace();  return; }
            default: return;
        }
    }

    /**
     * Assign the value to this term.  If current term has been initialized,
     * its old value is destructed.
     */
    template <typename T>
    void operator= (const T& a) { set(a); }

    // For some reason the template version above doesn't work for eterm<Alloc> parameter
    // so we overload it explicitely.
    eterm& operator= (const eterm<Alloc>& a) { if (this != &a) set(a); return *this; }

    /**
     * Assign the value to this term.  If current term has been initialized,
     * its old value is destructed.
     */
#if __cplusplus >= 201103L
    eterm& operator= (eterm&& a) {
        if (this != &a) {
            if (m_type >= STRING)
                this->~eterm();
            replace(&a);
        }
        return *this;
    }
#endif

    template <typename T>
    void set(const T& a) {
        if (m_type >= STRING)
            this->~eterm();
        new (this) eterm(a);
    }

    bool operator== (const eterm<Alloc>& rhs) const;
    bool operator!= (const eterm<Alloc>& rhs) const { return !this->operator==(rhs); }

    /**
     * Return true if the term was default constructed and not initialized.
     */
    bool empty() const { return m_type == UNDEFINED; }

    /**
     * Reset this object to UNDEFINED type.
     */
    void clear()       { this->~eterm(); m_type = UNDEFINED; }

    /**
     * Return true for all terms except tuples and lists for which return
     * the corrsponding value of their initialized() method.
     */
    bool initialized() const {
        switch (type()) {
            case TUPLE: { return vt.t.initialized(); }
            case LIST:  { return vt.l.initialized(); }
            case TRACE: { return vt.trc.initialized(); }
            default:    return true;
        }
    }

    /**
     * Quick way to check that two eterms are equal. The function returns true
     * if the terms are of the same type and their simple type values match or
     * they point to the same storage
     */
    bool equals(const eterm<Alloc>& rhs) const {
        return m_type == rhs.m_type && vt.value == rhs.vt.value;
    }

    /**
     * Get the string representation of this eterm using a variable binding
     * @param binding Variable binding to use. It can be null.
     */
    std::string to_string(size_t a_size_limit = std::string::npos,
        const varbind<Alloc>* binding = NULL) const;

    // Convert a term to its underlying type.  Will throw an exception
    // when the underlying type doesn't correspond to the requested operation.

    long                 to_long()   const { check(LONG);   return vt.i; }
    double               to_double() const { check(DOUBLE); return vt.d; }
    bool                 to_bool()   const { check(BOOL);   return vt.b; }
    const atom&          to_atom()   const { check(ATOM);   return vt.a; }
    const var&           to_var()    const { check(VAR);    return vt.v; }
    const string<Alloc>& to_str()    const { check(STRING); return vt.s; }
    const std::string    as_str()    const { check(STRING); return vt.s.to_str(); }
    const binary<Alloc>& to_binary() const { check(BINARY); return vt.bin; }
    const epid<Alloc>&   to_pid()    const { check(PID);    return vt.pid; }
    const port<Alloc>&   to_port()   const { check(PORT);   return vt.prt; }
    const ref<Alloc>&    to_ref()    const { check(REF);    return vt.r; }
    const tuple<Alloc>&  to_tuple()  const { check(TUPLE);  return vt.t; }
    tuple<Alloc>&        to_tuple()        { check(TUPLE);  return vt.t; }
    const list<Alloc>&   to_list()   const { check(LIST);   return vt.l; }
    list<Alloc>&         to_list()         { check(LIST);   return vt.l; }
    const trace<Alloc>&  to_trace()  const { check(TRACE);  return vt.trc; }
    trace<Alloc>&        to_trace()        { check(TRACE);  return vt.trc; }

    // Checks if database of the term is of given type

    bool is_double() const { return m_type == DOUBLE; }
    bool is_long()   const { return m_type == LONG  ; }
    bool is_bool()   const { return m_type == BOOL  ; }
    bool is_atom()   const { return m_type == ATOM  ; }
    bool is_str()    const { return m_type == STRING; }
    bool is_binary() const { return m_type == BINARY; }
    bool is_pid()    const { return m_type == PID   ; }
    bool is_port()   const { return m_type == PORT  ; }
    bool is_ref()    const { return m_type == REF   ; }
    bool is_var()    const { return m_type == VAR   ; }
    bool is_tuple()  const { return m_type == TUPLE ; }
    bool is_list()   const { return m_type == LIST  ; }
    bool is_trace()  const { return m_type == TRACE ; }

    /**
     * Perform pattern matching.
     * @param pattern Pattern (eterm) to match
     * @param binding varbind to use in pattern matching.
     *  This binding will be updated with new bound variables if
     *  match succeeds.
     * @return true if matching succeeded or false if failed.
     */
    bool match(const eterm<Alloc>& pattern, varbind<Alloc>* binding = NULL,
               const Alloc& a_alloc = Alloc()) const
        throw (err_unbound_variable);

    /**
     * Returns the equivalent without inner variables, using the
     * given binding to substitute them.
     * The normal behavior is:
     *  - Simple types return themselves
     *  - Compound types check if any of the contained terms changed,
     *      returning themselves if there are no changes
     *  - Variables get substituted.
     * @param binding eterm_varbind to use to resolve variables.
     *  It can be NULL and no substitutions will be performed, throwing
     *  err_unbound_variable if there is a variable.
     * @returns a smart pointer to the new term with all eterm_var
     *    variables replaced by bound values.
     * @throws err_invalid_term if the term is invalid
     * @throws err_unbound_variable if a variable is unbound
     */
    bool subst(eterm<Alloc>& out, const varbind<Alloc>* binding) const
        throw (err_invalid_term, err_unbound_variable);

    /** Substitutes all variables in the term \a a. */
    eterm<Alloc> apply(const varbind<Alloc>& binding) const
        throw (err_invalid_term, err_unbound_variable);

    /**
     * This method finds the first unbound variable in a term for
     * the given variable binding. It can be used to check if the term has
     * unbound variables.
     * @param a_binding is the list of bound variables.
     * @returns a pointer to eterm_var if there is an unbound variable,
     *          null otherwise.
     */
    const eterm<Alloc>*
    find_unbound(const varbind<Alloc>* a_binding) const;

    /**
     * @return the size of a buffer needed to hold encoded
     * representation of this eterm
     */
    size_t encode_size(size_t a_header_size = DEF_HEADER_SIZE,
                       bool a_with_version = true) const;

    /**
     * Encode a term into a binary representation stored in a
     * string buffer. This function is a wrapper around packet::encode().
     * @param a_header_size is the size of packet header (valid values: 0, 1, 2, 4).
     * @param a_with_version indicates if a magic version byte
     *        needs to be encoded in the beginning of the buffer.
     * @return binary encoded string.
     */
    string<Alloc> encode(size_t a_header_size = DEF_HEADER_SIZE,
                         bool a_with_version = true) const;

    /**
     * Encode a term into a binary representation stored in a
     * given buffer. The buffer must have the size obtained by the function
     * call encode_size().
     * @param buf is the buffer to hold encoded value.
     * @param size is the size of the \a buf.
     * @param a_header_size is the size of packet header (valid values: 0, 1, 2, 4).
     * @param a_with_version indicates if a magic version byte
     *        needs to be encoded in the beginning of the buffer.
     */
    void encode(char* buf, size_t size,
        size_t a_header_size = DEF_HEADER_SIZE, bool a_with_version = true) const
        throw (err_encode_exception);

    /**
     * Create an eterm from an string representation. Like sprintf()
     * function you can use it to create Erlang terms using a format
     * specifier and a corresponding set
     * of arguments.
     *
     * The set of valid format specifiers is as follows:
     * <ul>
     *   <li>a  -  An atom</li>
     *   <li>s  -  A string</li>
     *   <li>i  -  An integer</li>
     *   <li>l  -  A long integer</li>
     *   <li>u  -  An unsigned long integer</li>
     *   <li>f  -  A double float</li>
     *   <li>w  -  A pointer to some arbitrary term passed as argument</li>
     * </ul>
     *
     * Example:
     * <code>
     *   eterm::format("[{name,~a},{age,~i},{dob,~w}]",
     *          "alex", 40, eterm_t("1955-10-1"));
     * </code>
     * @return compiled eterm
     * @throws err_format_exception
     */
    static eterm<Alloc> format(const Alloc& a_alloc, const char* fmt, ...)
        throw (err_format_exception);
    static eterm<Alloc> format(const char* fmt, ...)
        throw (err_format_exception);

    /**
     * Same as format(a_alloc, fmt, ...), but parses string in format:
     * <code>"Module:Function(Arg1, Arg2, ...)</code>
     */
    static void format(const Alloc& a_alloc, atom& mod, atom& fun, eterm<Alloc>& args,
                       const char* fmt, ...) throw (err_format_exception);
    static void format(atom& mod, atom& fun, eterm<Alloc>& args, const char* fmt, ...)
        throw (err_format_exception);

    /// Cast a value to eterm. If t is of eterm type, it is returned as is.
    template <typename T>
    static eterm<Alloc> cast(T t) { return eterm<Alloc>(t); }

    static const eterm<Alloc>& cast(const eterm<Alloc>& t) { return t; }

    template <typename Visitor>
    wrap<typename Visitor::result_type, Visitor>
    visit(const Visitor& v) const {
        typedef wrap<typename Visitor::result_type, Visitor> wrapper;
        switch (m_type) {
            case LONG:   return wrapper(v, vt.i);
            case DOUBLE: return wrapper(v, vt.d);
            case BOOL:   return wrapper(v, vt.b);
            case ATOM:   return wrapper(v, vt.a);
            case VAR:    return wrapper(v, vt.v);
            case STRING: return wrapper(v, vt.s);
            case BINARY: return wrapper(v, vt.bin);
            case PID:    return wrapper(v, vt.pid);
            case PORT:   return wrapper(v, vt.prt);
            case REF:    return wrapper(v, vt.r);
            case TUPLE:  return wrapper(v, vt.t);
            case LIST:   return wrapper(v, vt.l);
            case TRACE:  return wrapper(v, vt.trc);
            default: {
                std::stringstream s; s << "Undefined term_type (" << m_type << ')';
                throw err_invalid_term(s.str());
            }
            BOOST_STATIC_ASSERT(MAX_ETERM_TYPE == 13);
        }
    }
};

template <typename T, typename Alloc> T& get(eterm<Alloc>& t) {
    return t.get((typename enum_type<T,Alloc>::type*)0);
}

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::eterm<Alloc>& a_term) {
        return out << a_term.to_string();
    }
}

#include <eixx/marshal/eterm.ipp>

#endif
