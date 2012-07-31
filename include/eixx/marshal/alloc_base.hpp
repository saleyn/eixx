//----------------------------------------------------------------------------
/// \file  alloc_base.hpp
//----------------------------------------------------------------------------
/// \brief Implementation of basic typed blob.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) library.

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
#ifndef _EIXX_ETERM_BASE_HPP_
#define _EIXX_ETERM_BASE_HPP_

#include <boost/noncopyable.hpp>
#include <eixx/marshal/defaults.hpp>
#include <eixx/util/common.hpp>
#include <iostream>

namespace EIXX_NAMESPACE {
namespace marshal {

    template <typename T, typename Alloc>
    struct alloc_base_impl {
        typedef typename Alloc::template rebind<T>::other T_alloc_type;

        struct alloc_impl: public T_alloc_type {
            alloc_impl() : T_alloc_type() {}
            alloc_impl(T_alloc_type const& a) : T_alloc_type(a) {}
        };
    };

    /// The base allocator that can be used to inherit from
    /// by classes that need custom allocation.  When used as a base
    /// class this class should not consume any memory due to 
    /// empty base class optimization.
    template <typename T, typename Alloc>
    class alloc_base : private alloc_base_impl<T, Alloc>::alloc_impl {
        typedef alloc_base_impl<T, Alloc>   impl_t;
        typedef typename impl_t::alloc_impl base_t;
    public:
        typedef Alloc                           allocator_type;
        typedef typename impl_t::T_alloc_type   T_alloc_type;

        alloc_base() {}
        alloc_base(const Alloc& alloc) : base_t(alloc) {}
    protected:
        T_alloc_type&
        get_t_allocator() { return *static_cast<T_alloc_type*>(this); }

        const T_alloc_type&
        get_t_allocator() const { return *static_cast<const T_alloc_type*>(this); }

        allocator_type
        get_allocator() const { return allocator_type(get_t_allocator()); }
    };

    /// \brief Reference-counted blob of memory to store the object of type T.
    template<typename T, typename Alloc>
    class blob : private boost::noncopyable
               , public Alloc::template rebind<T>::other
    {
        typedef typename Alloc::template rebind<T>::other base_t;
        typedef typename Alloc::template rebind<blob<T,Alloc> >::other blob_alloc_t;

        atomic<int>  m_rc;
        const size_t m_size;
        T*           m_data;

        ~blob() {
            this->deallocate(m_data, m_size);
        }

    public:
        blob(const Alloc& a = Alloc())
            : base_t(a), m_rc(1), m_size(0), m_data(NULL)
        {}

        /// Allocate storage for \a n items if size sizeof(T).
        blob(size_t n, const Alloc& a = Alloc())
            : base_t(a), m_rc(1), m_size(n), m_data(this->allocate(n)) {
            BOOST_ASSERT(m_data != NULL);
        }

        /// Decrement reference count and release internal storage 
        /// when reference count reaches 0. If \a immediate is <tt>false</tt>
        /// and reference count decrements to 0 the object deletion is not done.
        /// @return true is object was deleted or was supposed to be deleted
        ///         and \a immediate argument was <tt>false</tt>.
        bool release(bool immediate = true) {
            bool destroy = --m_rc == 0;
            if (destroy && immediate)
                delete this;
            return destroy;
        }

        /// Destructs the object.  This method is to be invoked by caller
        /// after a preceding call to release(false) returned true.
        void free() {
            BOOST_ASSERT(m_rc == 0);
            delete this;
        }

        /// Pointer to allocated array of items.
        T*     data()       const   { return m_data; }
        /// Number of items that data() points to.
        size_t size()       const   { return m_size; }

        /// Increment internal reference count.
        void   inc_rc()             { ++m_rc; }
        /// Return internal reference count. Use for debugging only.
        int    use_count()  const   { return m_rc; }

        const Alloc& get_allocator() const {
            return *reinterpret_cast<const Alloc*>(this);
        }

        static blob_alloc_t& get_blob_alloc() {
            static blob_alloc_t s_alloc;
            return s_alloc;
        }

        /// This method overrides the new() operator for this class
        /// so that the blob memory is taken from the Alloc allocator.
        static void* operator new(size_t sz) {
            BOOST_ASSERT(sz == sizeof(blob<T,Alloc>));
            return get_blob_alloc().allocate(1);
        }

        /// This method overrides the new() operator for this class
        /// so that the blob memory is released to the Alloc allocator.
        static void operator delete(void* p, size_t sz) {
            BOOST_ASSERT(sz == sizeof(blob<T,Alloc>));
            get_blob_alloc().deallocate(static_cast<blob<T,Alloc>*>(p), 1);
        }
    };

} // namespace marshal
} // namespace EIXX_NAMESPACE

#endif // _EIXX_ETERM_BASE_HPP_
