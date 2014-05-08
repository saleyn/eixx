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

This file is part of the eixx (Erlang C++ Interface) library.

Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _EIXX_TRANSPORT_OTP_CONNECTION_TCP_HPP_
#define _EIXX_TRANSPORT_OTP_CONNECTION_TCP_HPP_

#include <eixx/connect/transport_otp_connection.hpp>
#include <ei.h>
#include <erl_interface.h>

#include <eixx/config.h>

#ifdef EIXX_HAVE_EI_EPMD
extern "C" {

#include <epmd/ei_epmd.h>           // see erl_interface/src
#include <misc/eiext.h>             // ERL_VERSION_MAGIC
#include <connect/ei_connect_int.h> // see erl_interface/src

}
#endif

namespace EIXX_NAMESPACE {
namespace connect {

#ifndef EIXX_HAVE_EI_EPMD
// These constants are not exposed by EI headers:
static const char  ERL_VERSION_MAGIC            = 131;
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
        m_socket.close(); 
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

    std::shared_ptr<tcp_connection<Handler, Alloc> > shared_from_this() {
        std::shared_ptr<connection<Handler, Alloc> > p = base_t::shared_from_this();
        return *reinterpret_cast<std::shared_ptr<tcp_connection<Handler, Alloc> >*>(&p);
    }

    /// Set the socket to non-blocking mode and issue on_connect() callback.
    ///
    /// When implementing a server this method is to be called after 
    /// accepting a new connection.  When implementing a client, call
    /// connect() method instead, which invokes start() automatically. 
    void start();

    std::string remote_alivename() const {
        auto s = this->remote_nodename().to_string();
        auto n = s.find('@');
        BOOST_ASSERT(n != std::string::npos);
        return s.substr(0, s.find('@'));
    }
    std::string remote_hostname() const {
        auto s = this->remote_nodename().to_string();
        auto n = s.find('@');
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
} // namespace EIXX_NAMESPACE

//------------------------------------------------------------------------------
// connection_tcp implementation
//------------------------------------------------------------------------------
#include <eixx/connect/transport_otp_connection_tcp.ipp>

#endif // _EIXX_TRANSPORT_OTP_CONNECTION_TCP_HPP_

