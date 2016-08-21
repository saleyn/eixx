//----------------------------------------------------------------------------
/// \file  alloc_pool_st.hpp
//----------------------------------------------------------------------------
/// \brief Custom memory allocator using a pool of objects for single threaded
///        use case.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-08-10
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
#ifndef _EIXX_ALLOC_POOL_HPP_
#define _EIXX_ALLOC_POOL_HPP_

#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/detail/mutex.hpp>

#define EIXX_USE_ALLOCATOR

namespace eixx {

typedef boost::fast_pool_allocator<
      char
    , boost::default_user_allocator_new_delete
    , boost::details::pool::null_mutex
> allocator_t;

} // namespace eixx

#endif // _EIXX_ALLOC_POOL_HPP_

