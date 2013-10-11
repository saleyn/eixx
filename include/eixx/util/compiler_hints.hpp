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

// Branch prediction optimization (see http://lwn.net/Articles/255364/)
#ifndef NO_HINT_BRANCH_PREDICTION
#  define unlikely(expr) __builtin_expect(!!(expr), 0)
#  define likely(expr)   __builtin_expect(!!(expr), 1)
#else
#  define unlikely(expr) (expr)
#  define likely(expr)   (expr)
#endif

#endif // _EIXX_COMPILER_HINTS_HPP_

