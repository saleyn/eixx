//----------------------------------------------------------------------------
/// \file  test_mailbox.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for basic_otp_mailbox.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-23
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

#define BOOST_TEST_MODULE test_async_queue

//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING

#include <atomic>
#include <thread>
#include <boost/test/included/unit_test.hpp>
#include <boost/thread.hpp>
#include <eixx/util/async_queue.hpp>
#include <eixx/connect/test_helper.hpp>

using namespace eixx::util;

int b, n, i;

BOOST_AUTO_TEST_CASE( test_async_queue )
{
    boost::asio::io_service io;

    std::shared_ptr<async_queue<int>> q(new async_queue<int>(io));

    bool res = q->async_dequeue(
        [](int& /*a*/, const boost::system::error_code& /*ec*/) {
            throw std::exception(); // This handler is never called
            return false;
        },
        std::chrono::milliseconds(0));

    BOOST_REQUIRE(res); // Returns true because there was no async dispatch

    for (i = 10; i < 13; i++)
        BOOST_REQUIRE(q->enqueue(i));

    for (int j = 10; j < 13; j++) {
        BOOST_REQUIRE(q->async_dequeue(
            [j](int& a, const boost::system::error_code& /*ec*/) {
                BOOST_REQUIRE_EQUAL(j, a);
                return true;
            },
            std::chrono::milliseconds(0)));
    }

    b = 15;
    bool r = q->async_dequeue(
        [](int& a, const boost::system::error_code& /*ec*/) {
            BOOST_REQUIRE_EQUAL(b++, a);
            n++;
            return true;
        }, 3);
    BOOST_REQUIRE(!r);

    i = 15;

    BOOST_REQUIRE(q->enqueue(i++));
    BOOST_REQUIRE(q->enqueue(i++));
    BOOST_REQUIRE(q->enqueue(i++));

    io.run();

    BOOST_REQUIRE_EQUAL(3,  n);
    BOOST_REQUIRE_EQUAL(18, i);
    BOOST_REQUIRE_EQUAL(18, b);
}

namespace {
    const int iterations = getenv("ITERATIONS") ? atoi(getenv("ITERATIONS")) : 1000000;

    std::atomic_int producer_count(0);
    std::atomic_int consumer_count(0);

    const int producer_thread_count = 4;
    std::atomic_int done_producer_count(0);

    std::atomic<bool> done (false);
}

void producer(async_queue<int>& q, int id)
{
    for (int idx = 0; idx != iterations; ++idx) {
        int value = ++producer_count;
        while (!q.enqueue(value));
    }

    if (eixx::verboseness::level() >= eixx::connect::VERBOSE_DEBUG)
        std::cout << "Producer thread " << id << " done" << std::endl;

    done = ++done_producer_count == producer_thread_count;
}

BOOST_AUTO_TEST_CASE( test_async_queue_concurrent )
{
    boost::asio::io_service io;

    boost::shared_ptr<async_queue<int>> q(new async_queue<int>(io, 128));

    boost::thread_group producer_threads;

    for (int idx = 0; idx < producer_thread_count; ++idx)
        producer_threads.create_thread(
            [&q, idx] () { producer(*q, idx+1); }
        );

    while(q->async_dequeue(
        [] (int& /*v*/, const boost::system::error_code& ec) {
            if (!ec)
                ++consumer_count;
            return !(done && consumer_count >= producer_count);
        },
        std::chrono::milliseconds(1000),
        -1));

    io.run();
    io.reset();

    producer_threads.join_all();

    if (eixx::verboseness::level() >= eixx::connect::VERBOSE_DEBUG) {
        std::cout << "Produced " << producer_count << " objects." << std::endl;
        std::cout << "Consumed " << consumer_count << " objects." << std::endl;
    }

    bool abort = false;

    auto r = q->async_dequeue(
        [&abort] (int& /*v*/, const boost::system::error_code& /*ec*/) { return !abort; },
        std::chrono::milliseconds(8000),
        -1);

    BOOST_REQUIRE(!r);

    boost::asio::system_timer t(io);
    t.expires_from_now(std::chrono::milliseconds(1));
    t.async_wait([&q, &abort](const boost::system::error_code& /*e*/) {
        q->cancel();
        abort = true;
        if (eixx::verboseness::level() >= eixx::connect::VERBOSE_DEBUG)
            std::cout << "Canceled timer" << std::endl;
    });

    io.run();

    if (eixx::verboseness::level() >= eixx::connect::VERBOSE_DEBUG)
        std::cout << "Done!" << std::endl;
}

