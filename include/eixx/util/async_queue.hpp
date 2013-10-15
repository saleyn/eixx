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
#include <limits>
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
        boost::lockfree::capacity<256>
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

    int dec_repeat_count(int n) {
        return n == std::numeric_limits<int>::max() || !n ? n : n-1;
    }

    // Dequeue up to m_batch_size of items and for each one call
    // m_wait_handler
    void process_queue(const async_handler& h, const boost::system::error_code& ec,
                       std::chrono::milliseconds repeat, int repeat_count) {
        if (h == nullptr || m_is_canceled) return;

        // Process up to m_batch_size items waiting on the queue.
        // For each dequeued item call m_wait_handler

        int  i = 0;        // Number of handler invocations

        bool canceled = ec;

        T value;
        while (i < m_batch_size && m_queue.pop(value)) {
            i++;
            repeat_count = dec_repeat_count(repeat_count);
            if (!h(value, boost::system::error_code())) {
                canceled = true;
                break;
            }
        }

        // If we reached the batch size and queue has more data
        // to process - give up the time slice and reschedule the handler
        if (i == m_batch_size && !m_queue.empty()) {
            m_io.post([this, h, repeat, repeat_count]() {
                (*this->shared_from_this())(
                    h, boost::asio::error::operation_aborted, repeat, repeat_count);
            });
        } else if (!i && canceled) {
            // If we haven't processed any data and the timer was canceled.
            // Invoke the callback to see if we need to remove the handler.
            T dummy;
            if (!h(dummy, ec))
                return;
        }

        int n = dec_repeat_count(repeat_count);

        // If requested repeated timer, schedule new timer invocation
        if (repeat > std::chrono::milliseconds(0) && n > 0) {
            m_timer.expires_from_now(repeat);
            m_timer.async_wait(
                [this, h, repeat, n]
                (const boost::system::error_code& ec) {
                    (*this->shared_from_this())(h, ec, repeat, n);
                });
        }
    }

    // Called by io_service on timeout of m_timer
    void operator() (const async_handler& h, const boost::system::error_code& ec,
                     std::chrono::milliseconds repeat, int repeat_count) {
        process_queue(h, ec, repeat, repeat_count);
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
        cancel();

        T value;
        while (m_queue.pop(value));

        m_is_canceled = false;
    }

    void cancel() {
        m_is_canceled = true;
        boost::system::error_code ec;
        m_timer.cancel(ec);
    }

    bool canceled() const { return m_is_canceled; }

    bool enqueue(T const& data, bool notify = true) {
        if (m_is_canceled) return false;

        if (!m_queue.push(data))
            return false;

        if (!notify) return true;

        boost::system::error_code ec;
        m_timer.cancel(ec);

        return true;
    }

    bool dequeue(T& value) {
        if (m_is_canceled) return false;
        return m_queue.pop(value);
    }

    bool async_dequeue(const async_handler& a_on_data, int repeat_count = 0) {
        return async_dequeue(a_on_data, std::chrono::milliseconds(-1), repeat_count);
    }

    bool async_dequeue(const async_handler& a_on_data,
        std::chrono::milliseconds a_wait_duration = std::chrono::milliseconds(-1),
        int repeat_count = 0)
    {
        if (m_is_canceled) return false;

        T value;
        if (m_queue.pop(value))
            return a_on_data(value, boost::system::error_code());
        else if (a_wait_duration == std::chrono::milliseconds(0))
            return false;

        std::chrono::milliseconds timeout =
            a_wait_duration < std::chrono::milliseconds(0)
                ? std::chrono::milliseconds::max()
                : a_wait_duration;

        auto rep = repeat_count < 0 ? std::numeric_limits<int>::max() : repeat_count;
        auto repeat_msec = rep  > 0 ? timeout : std::chrono::milliseconds(0);
        boost::system::error_code ec;
        m_timer.cancel(ec);
        m_timer.expires_from_now(timeout);
        m_timer.async_wait(
            [this, &a_on_data, repeat_msec, rep]
            (const boost::system::error_code& e) {
                (*this->shared_from_this())(a_on_data, e, repeat_msec, rep);
            }
        );

        return false;
    }
};

} // namespace util
} // namespace EIXX_NAMESPACE

#endif // _EIXX_ASYNC_QUEUE_HPP
