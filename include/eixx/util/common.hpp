//----------------------------------------------------------------------------
/// \file common.hpp
//----------------------------------------------------------------------------
/// \namespace dmf Distributed Monitoring Framework.
///
/// This file contains a set of commonly used functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------

#ifndef _EIXX_COMMON_HPP_
#define _EIXX_COMMON_HPP_

#include <string>
#include <sstream>
#include <stdexcept>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/version.hpp>
#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <eixx/util/compiler_hints.hpp>

#ifdef HAVE_CONFIG_H
#include <eixx/config.h>
#endif

#define ERL_MONITOR_P       19
#define ERL_DEMONITOR_P     20
#define ERL_MONITOR_P_EXIT  21

namespace EIXX_NAMESPACE {

#if BOOST_VERSION > 104800
namespace bid = boost::interprocess::ipcdetail;
#else
namespace bid = boost::interprocess::detail;
#endif

/// \def THROW_RUNTIME_ERROR(S)
/// Throw an <tt>std::runtime_error</tt> by allowing to use a stream 
/// operator to create \a S.
#ifndef THROW_RUNTIME_ERROR
#define THROW_RUNTIME_ERROR(S) \
    do { \
        std::stringstream _s; _s << S; throw std::runtime_error(_s.str().c_str()); \
    } while(0)
#endif

/// \def ON_ERROR_CALLBACK(Client, S) 
/// Invokes an error callback from within perc_client's implementation.
///
/// @param Client is of type 'dmf::client'
/// @param S      is the stream of elements to include in the message.
///               The elements can be concatinated with left shift
///               notation.
#define ON_ERROR_CALLBACK(Connection, S) \
    do { \
        std::stringstream __s; __s << S; \
        (Connection)->on_error(__s.str()); \
    } while(0)

/// Calculate <tt>a</tt> raised to the power of <tt>b</tt>.
template <typename T>
T inline power(T a, size_t b) {
    if (a==0) return 0;
    if (b==0) return 1;
    if (b==1) return a;
    return (b & (b-1)) == 0 ? power(a*a, b >> 1) : a*power(a*a, b >> 1);
}

int __inline__ log2(unsigned long n, uint8_t base = 2) {
    BOOST_ASSERT(n > 0);
    return n == 1 ? 0 : 1+log2(n/base, base); 
}

static __inline__ unsigned long bit_scan_forward(unsigned long v)
{   
    unsigned long r;
    __asm__ __volatile__(
        #if (__SIZEOF_LONG__ == 8)
            "bsfq %1, %0": "=r"(r): "rm"(v) );
        #else
            "bsfl %1, %0": "=r"(r): "rm"(v) );
        #endif
    return r;
}

/// Wrapper for basic atomic operations over an integer.
template <typename T>
struct atomic {
    atomic(T a = 0): m_value(a) {
        // For now we just support 32bit signed and unsigned ints
        BOOST_STATIC_ASSERT(sizeof(T) == 4);
    }
    T operator++ ()                 { return bid::atomic_inc32(&m_value)+1; }
    T operator-- ()                 { return bid::atomic_dec32(&m_value)-1; }
    T operator++ (int)              { return bid::atomic_inc32(&m_value); }
    operator T () const             { return m_value; }
    void operator= (const T& rhs)   { xchg(rhs); }
    void operator+= (const T& a)    { bid::atomic_add32(&m_value, a); }

    void xchg(const T& a)           { bid::atomic_write32(&m_value, a); }

    T cas(const T& a_old, const T& a_new) { 
        return bid::atomic_cas32(&m_value, a_new, a_old); 
    }
private:
    uint32_t m_value;
};

/// Return the index of a_string in the a_list using a_default index if 
/// a_string is not found in the list.
template <int N>
int find_index(const char* (&a_list)[N], const char* a_string, int a_default=-1) {
    for (int i=0; i < N; i++)
        if (strcmp(a_string, a_list[i]) == 0)
            return i;
    return a_default;
}

} // namespace EIXX_NAMESPACE

#endif // _EIXX_COMMON_HPP_

