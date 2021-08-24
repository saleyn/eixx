//----------------------------------------------------------------------------
/// \file  transport_otp_connection_tcp.hpp
//----------------------------------------------------------------------------
/// \brief Interface of TCP connectivity transport with an Erlang node.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-11
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

#ifndef _EIXX_TRANSPORT_OTP_CONNECTION_TCP_HPP_
#define _EIXX_TRANSPORT_OTP_CONNECTION_TCP_HPP_

#include <eixx/connect/transport_otp_connection.hpp>
#include <ei.h>

#ifdef HAVE_CONFIG_H
#include <eixx/config.h>
#endif

namespace eixx {
namespace connect {

//----------------------------------------------------------------------------
/// TCP connection channel
//----------------------------------------------------------------------------
template <typename Handler, typename Alloc>
class tcp_connection
    : public connection<Handler, Alloc>
{
public:
    typedef connection<Handler, Alloc> base_t;
    typedef boost::asio::ip::port_type port_t;

    tcp_connection(boost::asio::io_service& a_svc, Handler* a_h, const Alloc& a_alloc)
        : connection<Handler, Alloc>(TCP, a_svc, a_h, a_alloc)
        , m_socket(a_svc)
        , m_resolver(a_svc)
        , m_state(CS_INIT)
    {}

    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket() { return m_socket; }

    void stop(const boost::system::error_code& e) {
        if (this->handler()->verbose() >= VERBOSE_TRACE)
            std::cout << "Calling connection_tcp::stop(" << e.message() << ')' << std::endl;
        boost::system::error_code ec;
        m_socket.close(ec);
        m_state = CS_INIT;
        base_t::stop(e);
    }

    std::string peer_address() const {
        std::stringstream s;
        s << m_peer_endpoint.address() << ':' << m_peer_endpoint.port();
        return s.str();
    }

    int native_socket() {
#if BOOST_VERSION >= 104700
        return m_socket.native_handle();
#else
        return m_socket.native();
#endif
    }

    uint64_t remote_flags() const { return m_remote_flags; }

private:
    /// Authentication state
    enum connect_state {
          CS_INIT
        , CS_WAIT_RESOLVE
        , CS_WAIT_EPMD_CONNECT
        , CS_WAIT_EPMD_WRITE_DONE
        , CS_WAIT_EPMD_REPLY
        , CS_WAIT_CONNECT
        , CS_WAIT_WRITE_CHALLENGE_DONE
        , CS_WAIT_STATUS
        , CS_WAIT_CHALLENGE
        , CS_WAIT_WRITE_CHALLENGE_REPLY_DONE
        , CS_WAIT_CHALLENGE_ACK
        , CS_CONNECTED
    };

    /// Socket for the connection.
    boost::asio::ip::tcp::socket    m_socket;
    boost::asio::ip::tcp::resolver  m_resolver;
    boost::asio::ip::tcp::endpoint  m_peer_endpoint;
    connect_state                   m_state;  // Async connection state

    size_t       m_expect_size;
    char         m_buf_epmd[EPMDBUF];
    char*        m_epmd_wr;

    char         m_buf_node[256];
    const char*  m_node_rd;
    char*        m_node_wr;
    uint16_t     m_dist_version;
    uint64_t     m_remote_flags;
    uint32_t     m_remote_challenge;
    uint32_t     m_our_challenge;

    /// @throws std::runtime_error
    void connect(uint32_t a_this_creation, atom a_this_node, atom a_remote_nodename, atom a_cookie);

    boost::shared_ptr<tcp_connection<Handler, Alloc> > shared_from_this() {
        boost::shared_ptr<connection<Handler, Alloc> > p = base_t::shared_from_this();
        return boost::reinterpret_pointer_cast<tcp_connection<Handler, Alloc> >(p);
    }

    /// Set the socket to non-blocking mode and issue on_connect() callback.
    ///
    /// When implementing a server this method is to be called after 
    /// accepting a new connection.  When implementing a client, call
    /// connect() method instead, which invokes start() automatically. 
    void start();

    std::string remote_alivename() const {
        auto s = this->remote_nodename().to_string();
#ifndef NDEBUG
        auto n = s.find('@');
#endif
        BOOST_ASSERT(n != std::string::npos);
        return s.substr(0, s.find('@'));
    }
    std::string remote_hostname() const {
        auto s = this->remote_nodename().to_string();
#ifndef NDEBUG
        auto n = s.find('@');
#endif
        BOOST_ASSERT(n != std::string::npos);
        return s.substr(s.find('@')+1);
    }

    void handle_resolve(
        const boost::system::error_code& err, 
        boost::asio::ip::tcp::resolver::iterator ep_iterator);
    void handle_epmd_connect(
        const boost::system::error_code& err,
        boost::asio::ip::tcp::resolver::iterator ep_iterator);
    void handle_epmd_write(const boost::system::error_code& err);
    void handle_epmd_read_header(
        const boost::system::error_code& err, size_t bytes_transferred);
    void handle_epmd_read_body(
        const boost::system::error_code& err, size_t bytes_transferred);
    void handle_connect(const boost::system::error_code& err);
    void handle_write_name(const boost::system::error_code& err);
    void handle_read_status_header(
        const boost::system::error_code& err, size_t bytes_transferred);
    void handle_read_status_body(
        const boost::system::error_code& err, size_t bytes_transferred);
    void handle_read_challenge_header(
        const boost::system::error_code& err, size_t bytes_transferred);
    void handle_read_challenge_body(
        const boost::system::error_code& err, size_t bytes_transferred);
    void handle_write_challenge_reply(const boost::system::error_code& err);
    void handle_read_challenge_ack_header(
        const boost::system::error_code& err, size_t bytes_transferred);
    void handle_read_challenge_ack_body(
        const boost::system::error_code& err, size_t bytes_transferred);

    uint32_t gen_challenge(void);
    void     gen_digest(unsigned challenge, const char cookie[], uint8_t digest[16]);
    uint32_t md_32(char* string, int length);
};

} // namespace connect
} // namespace eixx

//------------------------------------------------------------------------------
// connection_tcp implementation
//------------------------------------------------------------------------------
#include <eixx/connect/transport_otp_connection_tcp.hxx>

#endif // _EIXX_TRANSPORT_OTP_CONNECTION_TCP_HPP_

