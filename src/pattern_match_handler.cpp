//
// prioritised_handlers.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <iostream>
#include <queue>

#include <eixx/util/async_wait_timeout.hpp>
//#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::asio::ip::tcp;

class handler_priority_queue {
public:
    void add(int priority, boost::function<void()> function) {
        handlers_.push(queued_handler(priority, function));
    }

    void execute_all() {
        while (!handlers_.empty()) {
            queued_handler handler = handlers_.top();
            handler.execute();
            handlers_.pop();
        }
    }

    // A generic wrapper class for handlers to allow the invocation to be hooked.
    template <typename Handler>
    class wrapped_handler {
    public:
        wrapped_handler(handler_priority_queue& q, int p, Handler h)
            : queue_(q), priority_(p), handler_(h)
        { }

        void operator()() { handler_(); }

        template <typename Arg1>
            void operator()(Arg1 arg1) { handler_(arg1); }

        template <typename Arg1, typename Arg2>
            void operator()(Arg1 arg1, Arg2 arg2) { handler_(arg1, arg2); }

    //private:
        handler_priority_queue& queue_;
        int priority_;
        Handler handler_;
    };

    template <typename Handler>
    wrapped_handler<Handler> wrap(int priority, Handler handler) {
        return wrapped_handler<Handler>(*this, priority, handler);
    }

private:
    class queued_handler
    {
    public:
        queued_handler(int p, boost::function<void()> f)
            : priority_(p), function_(f)
        {}

        void execute() { function_(); }

        friend bool operator<(const queued_handler& a, const queued_handler& b) {
            return a.priority_ < b.priority_;
        }

    private:
        int priority_;
        boost::function<void()> function_;
    };

    std::priority_queue<queued_handler> handlers_;
};

// Custom invocation hook for wrapped handlers.
template <typename Function, typename Handler>
void asio_handler_invoke(Function f,
    handler_priority_queue::wrapped_handler<Handler>* h) {
    h->queue_.add(h->priority_, f);
}

//----------------------------------------------------------------------

void high_priority_handler(const boost::system::error_code& /*ec*/) {
    std::cout << "High priority handler\n";
}

void middle_priority_handler(const boost::system::error_code& /*ec*/) {
    std::cout << "Middle priority handler\n";
}

void low_priority_handler() {
    std::cout << "Low priority handler\n";
}

void async_handler(boost::asio::deadline_timer_ex& tm, const boost::system::error_code& ec) {
    std::cout << "Async handler called: " << ec.message() << std::endl
              << "   Timer.expires_at() = "
              << boost::posix_time::to_simple_string(tm.expires_at()) << std::endl;
}

void async_handler_canceler(boost::asio::deadline_timer_ex& tm,
    const boost::system::error_code& ec, boost::asio::deadline_timer_ex& tm2)
{
    std::cout << "Async handler canceler called: " << ec.message() << std::endl
              << "   Timer.expires_at() = "
              << boost::posix_time::to_simple_string(tm.expires_at()) << std::endl;
    //if (ec == boost::asio::error::timed_out)
        tm2.cancel();
}

int main()
{
    boost::asio::io_service io_service;

    handler_priority_queue pri_queue;

    // Post a completion handler to be run immediately.
    io_service.post(pri_queue.wrap(0, low_priority_handler));

    // Start an asynchronous accept that will complete immediately.
    tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 0);
    tcp::acceptor acceptor(io_service, endpoint);
    tcp::socket server_socket(io_service);
    acceptor.async_accept(server_socket,
        pri_queue.wrap(100, high_priority_handler));
    tcp::socket client_socket(io_service);
    client_socket.connect(acceptor.local_endpoint());

    if (boost::asio::deadline_timer::time_type() == boost::posix_time::pos_infin)
        std::cout << "  Time: Positive infinity match!" << std::endl;
    else if (boost::asio::deadline_timer::time_type() == boost::posix_time::not_a_date_time)
        std::cout << "  Time: Not a date time!" << std::endl;

    if (boost::asio::deadline_timer::time_type(boost::posix_time::pos_infin) == boost::posix_time::pos_infin)
        std::cout << "  Time: Positive infinity match!" << std::endl;
    else if (boost::asio::deadline_timer::time_type(boost::posix_time::pos_infin) == boost::posix_time::not_a_date_time)
        std::cout << "  Time: Not a date time!" << std::endl;

    std::cout << "Time: " << boost::posix_time::to_simple_string(
        boost::asio::deadline_timer::time_type()) << std::endl;

    // Set a deadline timer to expire immediately.
    boost::asio::deadline_timer timer(io_service);
    timer.expires_at(boost::posix_time::neg_infin);
    timer.async_wait(pri_queue.wrap(42, middle_priority_handler));

    while (io_service.run_one())
    {
        // The custom invocation hook adds the handlers to the priority queue
        // rather than executing them from within the poll_one() call.
        while (io_service.poll_one())
            ;

        pri_queue.execute_all();
    }

    boost::asio::deadline_timer_ex tm1(io_service);
    //tm1.async_wait_timeout(async_handler, 500);
    tm1.async_wait(boost::bind(&async_handler, boost::ref(tm1), _1));

    //io_service.reset();
    //io_service.run();

    boost::asio::deadline_timer_ex tm2(io_service);
    //tm1.async_wait_timeout(async_handler, 5000);
    tm2.async_wait_timeout(boost::bind(&async_handler_canceler, boost::ref(tm2), _1, boost::ref(tm1)), 300);

    io_service.reset();
    io_service.run();

    return 0;
}

