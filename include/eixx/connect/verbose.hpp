//----------------------------------------------------------------------------
// verbose.hpp
//----------------------------------------------------------------------------
/// This file contains a set of debugging primitives used by the 
/// performance monitoring library.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------

#ifndef _EIXX_VERBOSE_HPP_
#define _EIXX_VERBOSE_HPP_

namespace EIXX_NAMESPACE {

// otp_node.on_status reporting level
enum report_level {
      REPORT_INFO
    , REPORT_WARNING
    , REPORT_ERROR
};

namespace connect {

// Verboseness level
enum verbose_type {
      VERBOSE_NONE
    , VERBOSE_TEST
    , VERBOSE_DEBUG
    , VERBOSE_INFO
    , VERBOSE_MESSAGE
    , VERBOSE_WIRE
    , VERBOSE_TRACE
};

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_VERBOSE_HPP_

