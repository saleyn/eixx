//----------------------------------------------------------------------------
/// \file   async_queue.hpp
/// \author Serge Aleynikov
//----------------------------------------------------------------------------
/// \brief Multi-producer / single-consumer waitable queue.
//----------------------------------------------------------------------------
// Created: 2013-10-10
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx open-source project.

Copyright (C) 2013 Serge Aleynikov <saleyn@gmail.com>

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
#ifndef _EIXX_ASYNC_QUEUE_HPP
#define _EIXX_ASYNC_QUEUE_HPP

#include <chrono>
#include <stdexcept>
#include <functional>
#include <boost/lockfree/queue.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio.hpp>
#include <boost/concept_check.hpp>

#include <eixx/namespace.hpp>  // definition of EIXX_NAMESPACE
#include <eixx/util/timeout.hpp>

namespace EIXX_NAMESPACE {
namespace util {

using namespace boost::system::errc;

/**
 * Implements an asyncronous multiple-writer-single-reader queue
 * for use with BOOST ASIO.
 */
template<typename T, typename Alloc = std::allocator<char>>
struct async_queue : std::enable_shared_from_this<async_queue<T, Alloc>>
{
    typedef boost::lockfree::queue<
        T, boost::lockfree::allocator<Alloc>,
        boost::lockfree::capacity<255>
    > queue_type;

    typedef std::function<
        bool (T&, const boost::system::error_code& ec)
    > async_handler;
private:
    boost::asio::io_service&        m_io;
    queue_type                      m_queue;
    int                             m_batch_size;
    boost::asio::system_timer       m_timer;
    bool                            m_is_canceled;

    // Dequeue up to m_batch_size of items and for each one call
    // m_wait_handler
    void process_queue(async_handler h, std::chrono::milliseconds repeat) {
        if (h == nullptr) return;

        // Process up to m_batch_size items waiting on the queue.
        // For each dequeued item call m_wait_handler

        int  i = 0;        // Number of handler invocations

        bool canceled = m_is_canceled;

        if (!canceled)
            for (; i < m_batch_size; i++) {
                T value;
                if (m_queue.pop(value))
                    if (!h(value, boost::system::error_code())) // If handler returns false - break
                        break;
            }

        // If we haven't processed any data and the operation was canceled
        // invoke the callback to see if we need to remove the handler
        if (!i && canceled) {
            T dummy;
            h(dummy, boost::asio::error::eof);
        } else if (!canceled) {
            if (i == m_batch_size && !m_queue.empty()) {
                // There's more data to process, reschedule the handler
                m_io.post([this, h, repeat]() {
                    (*this->shared_from_this())(h, boost::asio::error::operation_aborted, repeat);
                });
            } else if (repeat > std::chrono::milliseconds(0)) {
                m_timer.expires_from_now(repeat);
                m_timer.async_wait(
                    [this, h, repeat]
                    (const boost::system::error_code& ec) {
                        (*this->shared_from_this())(h, ec, repeat);
                    });
            }
        }
    }

    // Called by io_service on timeout of m_timer
    void operator() (async_handler h, const boost::system::error_code& ec,
                     std::chrono::milliseconds repeat) {
        process_queue(h, repeat);
    }

public:
    async_queue(boost::asio::io_service& a_io,
        int a_batch_size = 16, const Alloc& a_alloc = Alloc())
        : m_io(a_io)
        , m_queue(a_alloc)
        , m_batch_size(a_batch_size)
        , m_timer(a_io)
        , m_is_canceled(false)
    {}

    ~async_queue() {
        reset();
    }

    void reset() {
        m_is_canceled = true;
        T value;
        while (m_queue.pop(value));

        m_timer.cancel();
        m_is_canceled = false;
    }

    bool canceled() const { return m_is_canceled; }

    bool enqueue(T const& data, bool notify = true) {
        if (m_is_canceled) return false;

        if (!m_queue.push(data))
            return false;

        if (!notify) return true;

        m_timer.cancel();

        return true;
    }

    bool dequeue(T& value) {
        if (m_is_canceled) return false;
        return m_queue.pop(value);
    }

    bool async_dequeue(async_handler a_on_data,
        std::chrono::milliseconds a_wait_duration = std::chrono::milliseconds(-1),
        bool repeat = false)
    {
        if (m_is_canceled) return false;

        T value;
        if (m_queue.pop(value)) {
            a_on_data(value, boost::system::error_code());
            return true;
        } else if (a_wait_duration == std::chrono::milliseconds(0))
            return false;

        m_timer.cancel();
        std::chrono::milliseconds timeout =
            a_wait_duration < std::chrono::milliseconds(0)
                ? std::chrono::milliseconds::max()
                : a_wait_duration;

        auto repeat_msec = repeat ? timeout : std::chrono::milliseconds(0);
        m_timer.expires_from_now(timeout);
        m_timer.async_wait(
            [this, &a_on_data, repeat_msec]
            (const boost::system::error_code& ec) {
                (*this)(a_on_data, ec, repeat_msec);
            }
        );

        return false;
    }

    void cancel_async() { m_timer.cancel(); }
};

} // namespace util
} // namespace EIXX_NAMESPACE

#endif // _EIXX_ASYNC_QUEUE_HPP
