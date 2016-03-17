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

#pragma once

// Branch prediction optimization (see http://lwn.net/Articles/255364/)

#ifndef NO_HINT_BRANCH_PREDICTION
#  ifndef  LIKELY
#   define LIKELY(expr)    __builtin_expect(!!(expr),1)
#  endif
#  ifndef  UNLIKELY
#   define UNLIKELY(expr)  __builtin_expect(!!(expr),0)
#  endif
#else
#  ifndef  LIKELY
#   define LIKELY(expr)    (expr)
#  endif
#  ifndef  UNLIKELY
#   define UNLIKELY(expr)  (expr)
#  endif
#endif

namespace eixx {

// Though the compiler should optimize this inlined code in the same way as
// when using LIKELY/UNLIKELY macros directly the preference is to use the later
#ifndef NO_HINT_BRANCH_PREDICTION
    inline bool likely(bool expr)   { return __builtin_expect((expr),1); }
    inline bool unlikely(bool expr) { return __builtin_expect((expr),0); }
#else
    inline bool likely(bool expr)   { return expr; }
    inline bool unlikely(bool expr) { return expr; }
#endif

} // namespace eixx
