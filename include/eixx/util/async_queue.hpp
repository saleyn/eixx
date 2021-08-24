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
#pragma once

#include <chrono>
#include <stdexcept>
#include <functional>
#include <limits>
#include <boost/lockfree/queue.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio.hpp>
#include <boost/concept_check.hpp>

#include <eixx/util/timeout.hpp>

namespace eixx {
namespace util {

using namespace boost::system::errc;

/**
 * Implements an asyncronous multiple-writer-single-reader queue
 * for use with BOOST ASIO.
 */
template<typename T, typename Alloc = std::allocator<char>>
struct async_queue : boost::enable_shared_from_this<async_queue<T, Alloc>>
{
    using queue_type = boost::lockfree::queue
        <T, boost::lockfree::allocator<Alloc>, boost::lockfree::capacity<1023>,
            boost::lockfree::fixed_sized<false>>;

    using async_handler =
        std::function<bool (T&, const boost::system::error_code& ec)>;
private:
    boost::asio::io_service&        m_io;
    queue_type                      m_queue;
    int                             m_batch_size;
    boost::asio::system_timer       m_timer;

    int dec_repeat_count(int n) {
        return n == std::numeric_limits<int>::max() || !n ? n : n-1;
    }

    // Dequeue up to m_batch_size of items and for each one call
    // m_wait_handler
    template <typename Handler>
    void process_queue(const Handler& h, [[maybe_unused]] const boost::system::error_code& ec,
                       std::chrono::milliseconds repeat, int repeat_count) {
        // Process up to m_batch_size items waiting on the queue.
        // For each dequeued item call m_wait_handler

        int  i = 0;        // Number of handler invocations

        T value;
        while (i < m_batch_size && m_queue.pop(value)) {
            i++;
            repeat_count = dec_repeat_count(repeat_count);
            if (!h(value, boost::system::error_code()))
                return;
        }

        static const auto s_timeout =
            boost::system::errc::make_error_code(boost::system::errc::timed_out);

        auto pthis = this->shared_from_this();

        // If we reached the batch size and queue has more data
        // to process - give up the time slice and reschedule the handler
        if (i == m_batch_size && !m_queue.empty()) {
            m_io.post([pthis, h, repeat, repeat_count]() {
                (*pthis)(h, boost::asio::error::operation_aborted, repeat, repeat_count);
            });
            return;
        } else if (!i && !h(value, s_timeout)) {
            // If we haven't processed any data and the timer was canceled.
            // Invoke the callback to see if we need to remove the handler.
            return;
        }

        int n = dec_repeat_count(repeat_count);

        // If requested repeated timer, schedule new timer invocation
        if (repeat > std::chrono::milliseconds(0) && n > 0) {
            m_timer.cancel();
            m_timer.expires_from_now(repeat);
            m_timer.async_wait(
                [pthis, h, repeat, n]
                (const boost::system::error_code& ec) {
                    (*pthis)(h, ec, repeat, n);
                });
        }
    }

    // Called by io_service on timeout of m_timer
    template <typename Handler>
    void operator() (const Handler& h, const boost::system::error_code& ec,
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
    {}

    ~async_queue() {
        reset();
    }

    void reset() {
        cancel();

        T value;
        while (m_queue.pop(value));
    }

    int  batch_size() const { return m_batch_size; }
    void batch_size(int sz) { m_batch_size = sz;   }

    bool cancel() {
        boost::system::error_code ec;
        return m_timer.cancel(ec);
    }

    bool enqueue(T const& data, bool notify = true) {
        if (!m_queue.push(data))
            return false;

        if (!notify) return true;

        boost::system::error_code ec;
        m_timer.cancel(ec);

        return true;
    }

    bool dequeue(T& value) {
        return m_queue.pop(value);
    }

    /// Call \a a_on_data handler asyncronously on next message in the queue.
    ///
    /// @returns true if the call was handled synchronously
    template <typename Handler>
    bool async_dequeue(const Handler& a_on_data, int repeat_count = 0) {
        return async_dequeue(a_on_data, std::chrono::milliseconds(-1), repeat_count);
    }

    /// Call \a a_on_data handler asyncronously on next message in the queue.
    ///
    /// @returns true if the call was handled synchronously
    template <typename Handler>
    bool async_dequeue(const Handler& a_on_data,
        std::chrono::milliseconds a_wait_duration = std::chrono::milliseconds(-1),
        int repeat_count = 0)
    {
        T value;
        if (m_queue.pop(value)) {
            if (!a_on_data(value, boost::system::error_code()))
                return true;
            if (repeat_count > 0) --repeat_count;
        }

        if (!repeat_count)
            return true;

        auto rep = repeat_count < 0 ? std::numeric_limits<int>::max() : repeat_count;

        std::chrono::milliseconds timeout =
            a_wait_duration < std::chrono::milliseconds(0)
                ? std::chrono::milliseconds::max()
                : a_wait_duration;

        if (timeout == std::chrono::milliseconds(0))
            m_io.post([this, &a_on_data, timeout, rep]() {
                (*this->shared_from_this())(
                    a_on_data, boost::system::error_code(), timeout, rep);
            });
        else {
            boost::system::error_code ec;
            m_timer.cancel(ec);
            m_timer.expires_from_now(timeout);
            auto pthis = this->shared_from_this();
            m_timer.async_wait(
                [pthis, &a_on_data, timeout, rep]
                (const boost::system::error_code& e) {
                    (*pthis)(a_on_data, e, timeout, rep);
                }
            );
        }

        return false;
    }
};

} // namespace util
} // namespace eixx
