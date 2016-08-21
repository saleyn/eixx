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
#ifndef _EIXX_SYNC_HPP_
#define _EIXX_SYNC_HPP_

#include <eixx/marshal/defaults.hpp>

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#include <mutex>
#else
#include <boost/thread.hpp>
#endif

namespace eixx {
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
} // namespace eixx

#endif // _EIXX_SYNC_HPP_

