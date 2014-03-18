/*
***** BEGIN LICENSE BLOCK *****

This file is part of the EPI (Erlang Plus Interface) Library.

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
#ifndef _EIXX_SYNC_HPP_
#define _EIXX_SYNC_HPP_

#include <eixx/marshal/defaults.hpp>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#include <mutex>
#else
#include <boost/thread.hpp>
#endif

namespace EIXX_NAMESPACE {
namespace detail {

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
typedef std::mutex                  mutex;
typedef std::recursive_mutex        recursive_mutex;
template <typename Mutex> struct lock_guard: public std::lock_guard<Mutex> {
    lock_guard(Mutex& a_m) : std::lock_guard<Mutex>(a_m) {}
};
#else
typedef boost::mutex                mutex;
typedef boost::recursive_mutex      recursive_mutex;
// Note that definition of boost::lock_guard is missing mutex_type compared
// to the definition of std::lock_guard.
template <typename Mutex> struct lock_guard: public boost::lock_guard<Mutex> {
    typedef boost::mutex mutex_type;
    lock_guard(Mutex& a_m) : boost::lock_guard<Mutex>(a_m) {}
};
#endif

} // namespace detail
} // namespace EIXX_NAMESPACE

#endif // _EIXX_SYNC_HPP_

