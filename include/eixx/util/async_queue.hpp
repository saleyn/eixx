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
#include <boost/lockfree/queue.hpp>
#include <boost/asio/basic_waitable_timer.hpp>

#include <eixx/util/defaults.hpp>
#include <eixx/util/timeout.hpp>

namespace EIXX_NAMESPACE {
namespace util {

using namespace boost::system::errc;

class queue_canceled : public std::exception {};

template<typename T, typename Alloc = std::allocator<char> >
class blocking_queue
{
public:
    typedef boost::lockfree::queue<
        T, boost::lockfree::allocator<Alloc>
    > queue_type;

    typedef std::function<bool (T&, const system::error_code& ec)> async_handler;
private:
    boost::asio::io_service&        m_io;
    queue_type                      m_queue;
    int                             m_batch_size;
    async_handler                   m_wait_handler;
    boost::asio::waitable_timer     m_timer;
    bool                            m_is_canceled;

    static const system::error_code s_success;
    static const system::error_code s_operation_canceled =
        make_error_code(operation_canceled);

    // Dequeue up to m_batch_size of items and for each one call
    // m_wait_handler
    void process_queue(const system::error_code& ec) {
        auto h = m_wait_handler;
        if (h == nullptr) return;

        // Process up to m_batch_size items waiting on the queue.
        // For each dequeued item call m_wait_handler

        bool remove = false;    // Indicates if the callee wants to remove the handler
        int  i      = 0;        // Number of handler invocations

        bool canceled = m_is_canceled || ec == s_operation_canceled;

        for (; i < m_batch_size && !remove && !canceled; i++) {
            T value;
            if (m_queue.pop(value))
                remove = h(value, s_success);
            else if (!i && ec == boost::asio::error:timeout)
                remove = h(value, ec);

            if (remove) break;
        }

        if (!remove)
            if (!i && canceled) {
                T dummy;
                if (!h(dummy, s_operation_canceled)) // If returns false - don't remove the handler
                    return;
            } else if (i == m_batch_size && !canceled && !m_queue.empty()) {
                m_io.post(*this);
                return;
            }
        }

        m_wait_handler = nullptr;
    }

    // Called by io_service on timeout of m_timer
    void operator() (const system::error_code& ec) {
        process_queue(ec);
    }

    // Called by io_service in response to enqueue(T&)
    void operator() () {
        process_queue(system::error_code());
    }

public:
    concurrent_queue(boost::asio::io_service& a_io,
        int a_batch_size = 1, const Alloc& a_alloc = Alloc())
        : m_io(a_io)
        , m_queue(255, a_alloc)
        , m_batch_size(a_batch_size)
        , m_timer(a_io)
        , m_is_canceled(false)
    {}

    void reset() {
        m_is_canceled = true;
        T value;
        while (m_queue.pop(value));

        if (m_wait_handler != nullptr)
            m_io.post([this]() { (*this)(s_operation_canceled); });

        m_is_canceled = false;
    }

    bool canceled() const { return m_is_canceled; }

    bool enqueue(T const& data) {
        if (m_is_canceled) return false;

        if (!m_queue.push(data))
            return false;

        if (m_wait_handler != nullptr)
            m_io.post(*this);
    }

    bool dequeue(T& value, const async_handler& a_on_data,
        std::chrono::milliseconds a_wait_duration = -1)
    {
        if (m_queue.pop(value))
            return true;
        else if (a_wait_duration.count() == 0)
            return false;

        if (m_waiter_handler != nullptr)
            throw std::runtime_error("Another waiting operation in progress!");
        m_wait_handler = a_on_data;

        if (m_queue.pop(value)) {
            m_wait_handler = nullptr;
            return true;
        }

        if (a_wait_duration.count() < 0 || m_is_canceled)
            return false;

        m_timer.expires_from_now(a_wait_duration);
        m_timer.async_wait([this]() {
            (*this)(boost::asio::error::make_error_code(boost::asio::error::timeout));
        });

        return false;
    }
};

} // namespace util
} // namespace EIXX_NAMESPACE

#endif // _EIXX_ASYNC_QUEUE_HPP
