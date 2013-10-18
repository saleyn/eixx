//----------------------------------------------------------------------------
/// \file compiler_hints.hpp
//----------------------------------------------------------------------------
/// \namespace eixx
///
/// This file contains various compiler optimization hints.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------

#ifndef _EIXX_COMPILER_HINTS_HPP_
#define _EIXX_COMPILER_HINTS_HPP_

#include <boost/lockfree/detail/branch_hints.hpp>

// Branch prediction optimization (see http://lwn.net/Articles/255364/)
namespace EIXX_NAMESPACE {

#ifndef NO_HINT_BRANCH_PREDICTION
    inline bool likely(bool expr)   { return boost::lockfree::detail::likely  (expr); }
    inline bool unlikely(bool expr) { return boost::lockfree::detail::unlikely(expr); }
#else
    inline bool likely(bool expr)   { return expr; }
    inline bool unlikely(bool expr) { return expr; }
#endif


} // namespace EIXX_NAMESPACE

#endif // _EIXX_COMPILER_HINTS_HPP_

