//----------------------------------------------------------------------------
/// \file  eterm_exception.hpp
//----------------------------------------------------------------------------
/// \brief Definition of custom exceptions used by eixx.
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

#ifndef _EIXX_EXCEPTION_HPP_
#define _EIXX_EXCEPTION_HPP_

#include <exception>
#include <sstream>
#include <string>

namespace eixx {

/**
 * Base class for the other eixx erlang exception classes.
 */
class eterm_exception: public std::exception {
public:
    eterm_exception() {}
    eterm_exception(const std::string& msg): m_msg(msg) {}

    template <typename... Arg>
    eterm_exception(const std::string& msg, Arg&&... a) {
        std::ostringstream str;
        str  << msg << ": ";
        (str << ... << a); // Fold expression
        m_msg = str.str();
    }

    virtual ~eterm_exception() throw() {}

    virtual const char*         what()    const throw() { return m_msg.c_str(); }
    virtual const std::string&  message() const { return m_msg; }

protected:
    std::string m_msg;
};

/// Exception for invalid atoms
struct err_atom_not_found: public eterm_exception {
    err_atom_not_found(std::string const& atom)
        : eterm_exception("Atom '" + atom + "' not found")
    {}
    err_atom_not_found() : eterm_exception("Atom not found") {}
};

/// Exception for invalid terms
struct err_invalid_term: public eterm_exception {
    err_invalid_term(const std::string &msg) : eterm_exception(msg) {}
};

/**
 * Exception for bad argument
 */
class err_wrong_type: public eterm_exception {
public:
    err_wrong_type(eterm_type a_got, eterm_type a_expected) {
        std::ostringstream s;
        s << "Wrong type " << type_to_string(a_got)
          << " (expected " << type_to_string(a_expected) << ')';
        m_msg = s.str();
    }
    template <typename Got, typename Expected>
    err_wrong_type(Got a_got, Expected a_expected) {
        std::ostringstream s;
        s << "Wrong type " << a_got
          << " (expected " << a_expected << ')';
        m_msg = s.str();
    }
};

/**
 * Exception for bad argument
 */
class err_bad_argument: public eterm_exception {
public:
    err_bad_argument(const std::string &msg) : eterm_exception(msg) {}

    template <typename T>
    err_bad_argument(const std::string &msg, T arg) : eterm_exception(msg, arg) {}
};

/**
 * Exception for unbound variables
 */
class err_unbound_variable: public eterm_exception {
public:
    err_unbound_variable(const std::string& variable)
        : m_var_name(variable)
    {
        std::ostringstream oss;
        oss << "Variable '" << variable << "' is unbound";
        m_msg = oss.str();
    }

    virtual ~err_unbound_variable() throw() {}
    const std::string& var_name() const { return m_var_name; }

private:
    std::string m_var_name;
};

/**
 * Exception while formatting
 */
class err_format_exception: public eterm_exception {
    const char* m_pos;
    const char* m_start;
    std::string m_what;
public:
    err_format_exception(
        const std::string& msg, const char* pos, const char* start = nullptr)
        : eterm_exception(msg)
        , m_pos(pos)
        , m_start(start)
    {
        std::stringstream s; s << m_msg << " (" << (m_pos - m_start) << ").";
        m_what = s.str();
    }

    const char* what() const throw() { return m_what.c_str(); }
    const char* pos() const          { return m_pos; }
    void start(const char* a_start)  { m_start = a_start; }
};

/**
 * Exception while encoding
 */
class err_encode_exception: public eterm_exception {
    int         m_code;
    std::string m_what;
public:
    err_encode_exception(const std::string &msg, int code=0)
        : eterm_exception(msg)
        , m_code(code)
    {
        std::stringstream s; s << m_msg << " (" << m_code << ").";
        m_what = s.str();
    }

    const char* what() const throw() { return m_what.c_str(); }
    int         code() const         { return m_code; }
};

/**
 * Exception while decoding
 */
class err_decode_exception: public err_encode_exception {
public:
    err_decode_exception(const std::string &msg, int code=0)
        : err_encode_exception(msg, code)
    {}
};

class err_empty_list: public eterm_exception {
public:
    err_empty_list() : eterm_exception("List is empty") {}
};

/**
 * Exception for error in establishing connection
 */
class err_connection: public err_bad_argument {
public:
    err_connection(const std::string &msg) : err_bad_argument(msg) {}

    template <class T>
    err_connection(const std::string &msg, T arg): err_bad_argument(msg, arg) {}
};

/**
 * Exception for process not found.
 */
class err_no_process: public err_bad_argument {
public:
    err_no_process(const std::string &msg) : err_bad_argument(msg) {}

    template <class T>
    err_no_process(const std::string &msg, T arg): err_bad_argument(msg, arg) {}
};

} // namespace eixx

#endif // _EIXX_EXCEPTION_HPP_
