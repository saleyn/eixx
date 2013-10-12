#ifndef _EIXX_TIMEOUT_HPP_
#define _EIXX_TIMEOUT_HPP_

#include <boost/asio.hpp>

namespace boost {
namespace asio {
namespace error {

    enum timer_errors {
        timeout = ETIMEDOUT
    };

    namespace detail {

        struct timer_category : public boost::system::error_category {
            const char* name() const noexcept { return "asio.timer"; }

            std::string message(int value) const {
                return value == error::timeout
                    ? "Operation timed out"
                    : "asio.timer error";
            }
        };

    } // namespace detail

    inline const boost::system::error_category& get_timer_category() {
        static detail::timer_category instance;
        return instance;
    }

    inline boost::system::error_code make_error_code(boost::asio::error::timer_errors e) {
        return boost::system::error_code(
            static_cast<int>(e), boost::asio::error::get_timer_category());
    }

} // namespace error
} // namespace asio

namespace system {

    template<> struct is_error_code_enum<boost::asio::error::timer_errors> {
        static const bool value = true;
    };

} // namespace system
} // namespace boost

#endif // _EIXX_TIMEOUT_HPP_
