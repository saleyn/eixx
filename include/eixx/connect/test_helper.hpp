//----------------------------------------------------------------------------
/// \file test_helper.hpp
//----------------------------------------------------------------------------
/// This file contains unit test helpers.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Omnibius, LLC
// Created: 2010-06-21
//----------------------------------------------------------------------------

#ifndef _EIXX_TEST_HELPER_HPP_
#define _EIXX_TEST_HELPER_HPP_

#include <eixx/connect/verbose.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

class verboseness {
    static verbose_type _get_verboseness() {
        const char* p = ::getenv("VERBOSE");
        return parse(p);
    }

public:
    static verbose_type level() {
        static verbose_type verbose = _get_verboseness();
        return verbose;
    }

    static verbose_type parse(const char* p) {
        if (!p || p[0] == '\0') return VERBOSE_NONE;
        int n = atoi(p);
        if      (n == 1 || strncmp("test",    p, 4) == 0) return VERBOSE_TEST;
        else if (n == 2 || strncmp("debug",   p, 3) == 0) return VERBOSE_DEBUG;
        else if (n == 3 || strncmp("info",    p, 4) == 0) return VERBOSE_INFO;
        else if (n == 4 || strncmp("message", p, 3) == 0) return VERBOSE_MESSAGE;
        else if (n == 5 || strncmp("wire",    p, 4) == 0) return VERBOSE_WIRE;
        else if (n >= 6 || strncmp("trace",   p, 5) == 0) return VERBOSE_TRACE; 
        else    return VERBOSE_NONE;
    }
};

} // namespace connect

typedef connect::verboseness verboseness;

} // namespace EIXX_NAMESPACE

#endif // _EIXX_TEST_HELPER_HPP_

