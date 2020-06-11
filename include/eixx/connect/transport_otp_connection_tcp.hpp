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

#include <eixx/config.h>

#ifdef HAVE_EI_EPMD
extern "C" {

#include <epmd/ei_epmd.h>           // see erl_interface/src
#include <misc/eiext.h>             // ERL_VERSION_MAGIC
#include <connect/ei_connect_int.h> // see erl_interface/src

}
#endif

namespace eixx {
namespace connect {

#ifndef HAVE_EI_EPMD
// These constants are not exposed by EI headers:
static const int   ERL_VERSION_MAGIC            = 131;
static const short EPMD_PORT                    = 4369;
static const int   EPMDBUF                      = 512;
static const char  EI_EPMD_PORT2_REQ            = 122;
static const char  EI_EPMD_PORT2_RESP           = 119;
static const char  EI_DIST_HIGH                 = 5;
static const int   DFLAG_PUBLISHED              = 1;
static const int   DFLAG_ATOM_CACHE             = 2;
static const int   DFLAG_EXTENDED_REFERENCES    = 4;
static const int   DFLAG_DIST_MONITOR           = 8;
static const int   DFLAG_FUN_TAGS               = 16;
static const int   DFLAG_NEW_FUN_TAGS           = 0x80;
static const int   DFLAG_EXTENDED_PIDS_PORTS    = 0x100;
static const int   DFLAG_NEW_FLOATS             = 0x800;
#endif

//----------------------------------------------------------------------------
/// TCP connection channel
//----------------------------------------------------------------------------
template <typename Handler, typename Alloc>
class tcp_connection
    : public connection<Handler, Alloc>
{
public:
    typedef connection<Handler, Alloc> base_t;

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

    int native_socket() { return m_socket.native(); }

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
    uint32_t     m_remote_challenge;
    uint32_t     m_our_challenge;

    void connect(atom a_this_node, atom a_remote_nodename, atom a_cookie)
        throw(std::runtime_error);

    boost::shared_ptr<tcp_connection<Handler, Alloc> > shared_from_this() {
        boost::shared_ptr<connection<Handler, Alloc> > p = base_t::shared_from_this();
        return *reinterpret_cast<boost::shared_ptr<tcp_connection<Handler, Alloc> >*>(&p);
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

