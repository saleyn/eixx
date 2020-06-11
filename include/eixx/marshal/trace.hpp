//----------------------------------------------------------------------------
/// \file  trace.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an trace object of Erlang external 
///        term format. This object is included in distributed messages
///        when tracing is enabled.  The object represents a tuple described
///        <a href="http://www.erlang.org/doc/man/seq_trace.html">here</a>.
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

#ifndef _IMPL_TRACE_HPP_
#define _IMPL_TRACE_HPP_

#include <boost/static_assert.hpp>
#include <eixx/eterm_exception.hpp>
#include <eixx/marshal/tuple.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {

/// Represents trace operations performed on the <tt>trace<Alloc>::tracer</tt>.
enum trace_op { TRACE_OFF = -1, TRACE_GET = 0, TRACE_ON = 1 };

/**
 * Representation of erlang trace token.
 * A trace token is a tuple of 5 elements:
 *   {Flags, Label, Serial, From, Prev}
 */
template <class Alloc>
class trace : protected tuple<Alloc> {
    trace() {}

    /// Increment the serial number. This method is not atomic.
    void inc_serial() { long n = serial(); (*this)[4] = n; (*this)[2] = n+1; }
    
    void check_clock(uint32_t& clock) {
        long n = serial(); if (n > clock) (*this)[4] = clock = n;
    }

public:

    /**
     * Create an Erlang trace tocken from its components using provided allocator.
     **/
    trace(long a_flags, long a_label, long a_serial, const epid<Alloc>& a_from, 
          long a_prev,  const Alloc& a_alloc = Alloc()) 
        : tuple<Alloc>(5, a_alloc)
    {
        this->init_element(0, a_flags);
        this->init_element(1, a_label);
        this->init_element(2, a_serial);
        this->init_element(3, a_from);
        this->init_element(4, a_prev);
        this->set_initialized();
    }

    /// Decode the pid from a binary buffer.
    trace(const char* buf, int& idx, size_t a_size, const Alloc& a_alloc = Alloc())
        : tuple<Alloc>(buf, idx, a_size, a_alloc)
    {
        if (size() != 5 || (*this)[0].type() != LONG
                        || (*this)[1].type() != LONG
                        || (*this)[2].type() != LONG
                        || (*this)[3].type() != PID
                        || (*this)[4].type() != LONG)
            throw err_decode_exception("Invalid trace token type!");
    }

    trace(const trace& rhs) : tuple<Alloc>(rhs) {}

    ~trace() {}

    void operator= (const trace& rhs) {
        tuple<Alloc>::operator= (static_cast<const tuple<Alloc>&>(rhs));
    }

    /// Get the flags from the trace.
    long flags()            const { return (*this)[0].to_long(); }

    /// Get the label from the trace.
    long label()            const { return (*this)[1].to_long(); }

    /// Get the serial number from the trace.
    long serial()           const { return (*this)[2].to_long(); } 

    /// Get the pid of sender from the trace
    const epid<Alloc>& from() const { return (*this)[3].to_pid(); }

    /// Get the prev number from the trace.
    long prev()             const { return (*this)[4].to_long(); }

    bool   initialized()    const { return tuple<Alloc>::initialized(); }
    size_t size()           const { return tuple<Alloc>::size(); }

    bool operator== (const trace<Alloc>& rhs) const {
        return tuple<Alloc>::operator== (static_cast<const tuple<Alloc>&>(rhs));
    }

    bool operator< (const trace<Alloc>& rhs) const {
        return *static_cast<const tuple<Alloc>*>(this) < static_cast<const tuple<Alloc>&>(rhs);
    }

    size_t encode_size() const { return tuple<Alloc>::encode_size(); }

    void encode(char* buf, int& idx, size_t size) const {
        tuple<Alloc>::encode(buf, idx, size);
    }

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding=NULL) const {
        return out << *static_cast<const tuple<Alloc>*>(this);
    }

    /// Provides support for message tracing facility.
    static trace<Alloc>* tracer(trace_op op, const trace<Alloc>* token = NULL) {
        static trace<Alloc> save_token;
        static bool     tracing = false;
        static uint32_t clock   = 0;

        switch (op) {
            case TRACE_OFF: tracing = false; break;
            case TRACE_GET:
                if (tracing) {
                    clock++;
                    save_token.inc_serial();
                    return &save_token;
                }
                break;
            case TRACE_ON:
                BOOST_ASSERT(token != NULL);
                tracing = true;
                save_token = *token;
                save_token.check_clock(clock);
                break;
        }
        return NULL;
    }
};

} //namespace marshal
} //namespace eixx

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, const eixx::marshal::trace<Alloc>& a) {
        return a.dump(out);
    }

} // namespace std

#endif // _IMPL_TRACE_HPP_

