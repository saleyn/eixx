//----------------------------------------------------------------------------
/// \file connection_tcp.hxx
//----------------------------------------------------------------------------
/// \brief Implementation of TCP connectivity transport with an Erlang node.
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

#ifndef _EIXX_CONNECTION_TCP_IPP_
#define _EIXX_CONNECTION_TCP_IPP_

#include <sys/utsname.h>
#include <openssl/md5.h>

namespace eixx {
namespace connect {

//------------------------------------------------------------------------------
// connection_tcp implementation
//------------------------------------------------------------------------------

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::start() 
{
#if BOOST_VERSION >= 104700
    m_socket.non_blocking(true);
#else
    boost::asio::ip::tcp::socket::non_blocking_io nb(true);
    m_socket.io_control(nb);
#endif

    base_t::start(); // trigger on_connect callback
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::connect(
    uint32_t a_this_creation,
    atom a_this_node, atom a_remote_node, atom a_cookie)
{
    using boost::asio::ip::tcp;

    BOOST_ASSERT(a_this_node.to_string().find('@') != std::string::npos);

    if (a_remote_node.to_string().find('@') == std::string::npos)
        THROW_RUNTIME_ERROR("Invalid format of remote_node: " << a_remote_node);

    base_t::connect(a_this_creation, a_this_node, a_remote_node, a_cookie);

    //boost::system::error_code err = boost::asio::error::host_not_found;
    std::stringstream es;

    boost::system::error_code ec;
    m_socket.close(ec);

    // First resolve remote host name and connect to EPMD to find out the
    // node's port number.

    const char* epmd_port_s = getenv("ERL_EPMD_PORT");

    auto epmd_port = (epmd_port_s != NULL) ? epmd_port_s : std::to_string(EPMD_PORT);
    auto host      = remote_hostname();

    tcp::resolver::query q(host, epmd_port);
    m_state    = CS_WAIT_RESOLVE;
    auto pthis = this->shared_from_this();
    m_resolver.async_resolve(q, [pthis](auto& err, auto& ep_iterator) {
        pthis->handle_resolve(err, ep_iterator);
    });
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_resolve(
    const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator ep_iterator)
{
    BOOST_ASSERT(m_state == CS_WAIT_RESOLVE);
    if (err) {
        std::stringstream ss;
        ss << "Error resolving address of node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        return;
    }
    // Attempt a connection to the first endpoint in the list. Each endpoint
    // will be tried until we successfully establish a connection.
    m_peer_endpoint = *ep_iterator;
    m_state     = CS_WAIT_EPMD_CONNECT;
    auto pthis  = this->shared_from_this();
    auto epnext = ++ep_iterator;
    m_socket.async_connect(m_peer_endpoint, [pthis, epnext](auto& a_err) {
        pthis->handle_epmd_connect(a_err, epnext);
    });
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_epmd_connect(
    const boost::system::error_code& err, boost::asio::ip::tcp::resolver::iterator ep_iterator)
{
    BOOST_ASSERT(m_state == CS_WAIT_EPMD_CONNECT);
    if (!err) {
        // The connection was successful. Send the request.
        std::string alivename = remote_alivename();
        char* w = m_buf_epmd;
        size_t sz = alivename.size();
        BOOST_ASSERT(sz < UINT16_MAX);
        uint16_t len = (uint16_t)sz + 1;
        put16be(w, len);
        put8(w,EI_EPMD_PORT2_REQ);
        strncpy(w, alivename.c_str(), sz);

        if (this->handler()->verbose() >= VERBOSE_TRACE) {
            std::stringstream ss;
            ss << "-> sending epmd port req for '" << alivename << "': "
               << to_binary_string(m_buf_epmd, len+2);
            this->handler()->report_status(REPORT_INFO, ss.str());
        }

        m_state    = CS_WAIT_EPMD_WRITE_DONE;
        auto pthis = this->shared_from_this();
        /*
        boost::asio::async_write(m_socket, boost::asio::buffer(m_buf_epmd, len+2),
            [pthis](auto& err) { pthis->handle_epmd_write(err); });
        */
        boost::asio::async_write(m_socket, boost::asio::buffer(m_buf_epmd, len+2),
            std::bind(&tcp_connection<Handler, Alloc>::handle_epmd_write, pthis,
                      std::placeholders::_1));
    } else if (ep_iterator != boost::asio::ip::tcp::resolver::iterator()) {
        // The connection failed. Try the next endpoint in the list.
        boost::system::error_code ec;
        m_socket.close(ec);
        m_peer_endpoint = *ep_iterator;
        auto pthis      = this->shared_from_this();
        auto epnext     = ++ep_iterator;
        m_socket.async_connect(m_peer_endpoint,
                               [pthis, epnext](auto& a_err) {
                                   pthis->handle_epmd_connect(a_err, epnext);
                               });
    } else {
        std::stringstream ss;
        ss << "Error connecting to epmd at host '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
    }
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_epmd_write(const boost::system::error_code& err)
{
    BOOST_ASSERT(m_state == CS_WAIT_EPMD_WRITE_DONE);
    if (err) {
        std::stringstream ss;
        ss << "Error writing to epmd at host '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_state = CS_WAIT_EPMD_REPLY;
    m_epmd_wr = m_buf_epmd;
    boost::asio::async_read(m_socket, boost::asio::buffer(m_buf_epmd, sizeof(m_buf_epmd)),
        boost::asio::transfer_at_least(2),
        std::bind(&tcp_connection<Handler, Alloc>::handle_epmd_read_header, shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_epmd_read_header(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_EPMD_REPLY);
    if (err) {
        std::stringstream ss;
        ss << "Error reading response from epmd at host '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    const char* l_epmd_rd = m_buf_epmd;
    m_expect_size = 8;

    int res = get8(l_epmd_rd);

    if (res != EI_EPMD_PORT2_RESP) { // response type
        std::stringstream ss;
        ss << "Error unknown response from epmd at host '" 
           << this->remote_nodename() << "': " << res;
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    int n = get8(l_epmd_rd);

    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "<- response from epmd: " << n 
           << " (" << (n ? "failed" : "ok") << ')';
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    if (n) { // Got negative response
        std::stringstream ss;
        ss << "Node " << this->remote_nodename() << " not known to epmd!";
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_epmd_wr = m_buf_epmd + bytes_transferred;

    size_t got_bytes = bytes_transferred - 2;
    size_t need_bytes = m_expect_size > got_bytes ? m_expect_size - got_bytes : 0;
    if (need_bytes > 0) {
        boost::asio::async_read(m_socket, 
            boost::asio::buffer(m_epmd_wr, sizeof(m_buf_epmd)-bytes_transferred),
            boost::asio::transfer_at_least(need_bytes),
            std::bind(&tcp_connection<Handler, Alloc>::handle_epmd_read_body, shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    } else {
        handle_epmd_read_body(boost::system::error_code(), 0);
    }
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_epmd_read_body(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_EPMD_REPLY);
    if (err) {
        std::stringstream ss;
        ss << "Error reading response body from epmd at host '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_epmd_wr += bytes_transferred;
    #ifndef NDEBUG
    size_t got_bytes = m_epmd_wr - m_buf_epmd;
    #endif
    BOOST_ASSERT(got_bytes >= 10);

    const char* l_epmd_rd = m_buf_epmd + 2;
    port_t port           = get16be(l_epmd_rd);
    int ntype             = get8(l_epmd_rd);
    int proto             = get8(l_epmd_rd);
    uint16_t dist_high    = get16be(l_epmd_rd);
    uint16_t dist_low     = get16be(l_epmd_rd);
    m_dist_version        = (dist_high > EI_DIST_HIGH ? EI_DIST_HIGH : dist_high);

    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "<- epmd returned: port=" << port << ",ntype=" << ntype
           << ",proto=" << proto << ",dist_high=" << dist_high
           << ",dist_low=" << dist_low;
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    m_peer_endpoint.port(port);
    m_peer_endpoint.address( m_socket.remote_endpoint().address() );
    boost::system::error_code ec;
    m_socket.close(ec);

    if (m_dist_version <= 4) {
        std::stringstream ss;
        ss << "Incompatible version " << m_dist_version
           << " of remote node '" << this->remote_nodename() << "'";
        this->handler()->on_connect_failure(this, ss.str());
        return;
    }

    // Change the endpoint's port to the one resolved from epmd
    m_state = CS_WAIT_CONNECT;

    m_socket.async_connect(m_peer_endpoint,
        std::bind(&tcp_connection<Handler, Alloc>::handle_connect, shared_from_this(),
            std::placeholders::_1));
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_connect(const boost::system::error_code& err)
{
    BOOST_ASSERT(m_state == CS_WAIT_CONNECT);
    if (err) {
        std::stringstream ss;
        ss << "Cannot connect to node " << this->remote_nodename() 
           << " at port " << m_peer_endpoint.port() << ": " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "<- TCP_OPEN (ok) from node '" << this->remote_nodename() << "'";
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    m_our_challenge = gen_challenge();

    auto flags = LOCAL_FLAGS;
#ifdef EI_DIST_5
    char tag = (m_dist_version == EI_DIST_5) ? 'n' : 'N';
#else
    char tag = 'n';
#endif
#ifdef DFLAG_NAME_ME
    if (this->local_nodename().empty()) {
        /* dynamic node name */
        tag = 'N'; /* presume ver 6 */
        flags |= DFLAG_NAME_ME;
    }
#endif

    uint16_t siz;
    if (tag == 'n')
        siz = 2 + 1 + 2 + 4 + this->local_nodename().size();
    else /* tag == 'N' */
        siz = 2 + 1 + 8 + 4 + 2 + this->local_nodename().size();

    if (siz > sizeof(m_buf_node)) {
        std::stringstream ss;
        ss << "-> SEND_NAME (error) nodename too long: " << this->local_nodename()
           << " [" << __FILE__ << ':' << __LINE__ << ']';
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    char* w = m_buf_node;
    put16be(w, siz - 2);
    put8(w, static_cast<uint8_t>(tag));
    if (tag == 'n') {
#ifdef EI_DIST_5
        m_dist_version = EI_DIST_5;
        put16be(w, EI_DIST_5); /* spec demands ver==5 */
#else
        m_dist_version = EI_DIST_LOW;
        put16be(w, EI_DIST_LOW); /* spec demands ver==5 */
#endif
        put32be(w, flags & 0xffffffff /* FlagsLow */);
    } else { /* tag == 'N' */
        put64be(w, flags);
        put32be(w, this->local_creation());
        put16be(w, this->local_nodename().size());
    }
    memcpy(w, this->local_nodename().c_str(), this->local_nodename().size());

    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "-> SEND_NAME sending creation=" << this->local_creation()
           << " to node '" << this->remote_nodename() << "': " 
           << to_binary_string(m_buf_node, siz);
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    m_state = CS_WAIT_WRITE_CHALLENGE_DONE;
    boost::asio::async_write(m_socket, boost::asio::buffer(m_buf_node, siz),
        std::bind(&tcp_connection<Handler, Alloc>::handle_write_name, shared_from_this(),
                    std::placeholders::_1));
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_write_name(const boost::system::error_code& err)
{
    BOOST_ASSERT(m_state == CS_WAIT_WRITE_CHALLENGE_DONE);
    if (err) {
        std::stringstream ss;
        ss << "-> SEND_NAME (error) sending name to node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }
    m_state = CS_WAIT_STATUS;
    boost::asio::async_read(m_socket, boost::asio::buffer(m_buf_node, sizeof(m_buf_node)),
        boost::asio::transfer_at_least(2),
        std::bind(&tcp_connection<Handler, Alloc>::handle_read_status_header, shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_read_status_header(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_STATUS);
    if (err) {
        std::stringstream ss;
        ss << "<- RECV_STATUS (error) reading status header from node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_rd = m_buf_node;
    m_expect_size = get16be(m_node_rd);

    if (m_expect_size > static_cast<size_t>(MAXNODELEN + 8) || m_expect_size > static_cast<size_t>((m_buf_node+1) - m_node_rd)) {
        std::stringstream ss;
        ss << "<- RECV_STATUS (error) in status length from node '" 
           << this->remote_nodename() << "': " << m_expect_size;
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_wr = m_buf_node + bytes_transferred;

    size_t got_bytes = static_cast<uintptr_t>(m_node_wr - m_node_rd);
    size_t need_bytes = m_expect_size > got_bytes ? m_expect_size - got_bytes : 0;
    if (need_bytes > 0) {
        boost::asio::async_read(m_socket, 
            boost::asio::buffer(m_node_wr, static_cast<size_t>((m_buf_node+1) - m_node_wr)),
            boost::asio::transfer_at_least(need_bytes),
            std::bind(&tcp_connection<Handler, Alloc>::handle_read_status_body, shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    } else {
        handle_read_status_body(boost::system::error_code(), 0);
    }
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_read_status_body(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_STATUS);
    if (err) {
        std::stringstream ss;
        ss << "<- RECV_STATUS (error) reading status body from node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_wr += bytes_transferred;
    size_t got_bytes = static_cast<uintptr_t>(m_node_wr - m_node_rd);
    BOOST_ASSERT(got_bytes >= 3);

    if (!this->local_nodename().empty()) {
        if (memcmp(m_node_rd, "sok", 3) != 0) {
            std::stringstream ss;
            ss << "<- RECV_STATUS (error) in auth status from node '" 
               << this->remote_nodename() << "': " << std::string(m_node_rd, 1, got_bytes);
            this->handler()->on_connect_failure(this, ss.str());
            boost::system::error_code ec;
            m_socket.close(ec);
            return;
        }
    } else {
        /* dynamic node name */
        if (memcmp(m_node_rd, "snamed:", 7) != 0) {
            std::stringstream ss;
            ss << "<- RECV_STATUS (error) in auth status from node '" 
               << this->remote_nodename() << "': " << std::string(m_node_rd, 1, got_bytes);
            this->handler()->on_connect_failure(this, ss.str());
            boost::system::error_code ec;
            m_socket.close(ec);
            return;
        }

        uint16_t nodename_len = get16be(m_node_rd);
        if (nodename_len > MAXNODELEN) {
            std::stringstream ss;
            ss << "<- RECV_STATUS (error) nodename too long from node '" 
               << this->remote_nodename() << "': " << nodename_len;
            this->handler()->on_connect_failure(this, ss.str());
            boost::system::error_code ec;
            m_socket.close(ec);
            return;
        }
        atom l_this_node(m_node_rd, nodename_len);
        this->m_this_node = l_this_node;

        uint32_t l_cre = get32be(m_node_rd);
        this->m_this_creation = l_cre;
    }

    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "<- RECV_STATUS (ok) from node '" << this->remote_nodename()
           << "': version=" << m_dist_version 
           << ", status=" << std::string(m_node_rd, 1, got_bytes);
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    m_state = CS_WAIT_CHALLENGE;

    // recv challenge
    m_node_rd += 3;
    got_bytes -= 3;
    if (got_bytes >= 2)
        handle_read_challenge_header(boost::system::error_code(), 0);
    else
        boost::asio::async_read(m_socket, boost::asio::buffer(m_node_wr, static_cast<size_t>((m_buf_node+1) - m_node_wr)),
            boost::asio::transfer_at_least(2),
            std::bind(&tcp_connection<Handler, Alloc>::handle_read_challenge_header, shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_read_challenge_header(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_CHALLENGE);
    if (err) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE (error) reading challenge header from node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_wr += bytes_transferred;
    m_expect_size = get16be(m_node_rd);

#ifdef EI_DIST_5
    unsigned short l_size = (m_dist_version == EI_DIST_5) ? 11 : 19;
#else
    unsigned short l_size = 11;
#endif

    if (m_expect_size > static_cast<size_t>(MAXNODELEN + l_size) || m_expect_size > static_cast<size_t>((m_buf_node+1) - m_node_rd)) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE (error) in challenge length from node '" 
           << this->remote_nodename() << "': " << m_expect_size;
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    size_t got_bytes = static_cast<uintptr_t>(m_node_wr - m_node_rd);
    size_t need_bytes = m_expect_size > got_bytes ? m_expect_size - got_bytes : 0;
    if (need_bytes > 0) {
        boost::asio::async_read(m_socket, 
            boost::asio::buffer(m_node_wr, static_cast<size_t>((m_buf_node+1) - m_node_wr)),
            boost::asio::transfer_at_least(need_bytes),
            std::bind(&tcp_connection<Handler, Alloc>::handle_read_challenge_body, shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    } else {
        handle_read_challenge_body(boost::system::error_code(), 0);
    }
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_read_challenge_body(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_CHALLENGE);
    if (err) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE (error) reading challenge body from node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_wr += bytes_transferred;
    #ifndef NDEBUG
    size_t got_bytes = static_cast<uintptr_t>(m_node_wr - m_node_rd);
    #endif
    BOOST_ASSERT(got_bytes >= m_expect_size);

    const char tag = static_cast<char>(get8(m_node_rd));
    if (tag != 'n' && tag != 'N') {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE (error) incorrect tag, "
           << "expected 'n' or 'N', got '" << tag << "' from node '"
           << this->remote_nodename() << "'";
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    size_t   nodename_len;
    uint16_t dist_version = m_dist_version;

    // See: https://github.com/erlang/otp/blob/OTP-24.0.5/lib/erl_interface/src/connect/ei_connect.c#L2478
    if (tag == 'n') { /* OLD */
        m_dist_version = get16be(m_node_rd);
#ifdef EI_DIST_5
        if (m_dist_version != EI_DIST_5) {
            std::stringstream ss;
            ss << "<- RECV_CHALLENGE 'n' (error) incorrect version from node '" 
               << this->remote_nodename() << "': " << m_dist_version;
            this->handler()->on_connect_failure(this, ss.str());
            boost::system::error_code ec;
            m_socket.close(ec);
            return;
        }
#endif
    
        m_remote_flags     = get32be(m_node_rd);
        m_remote_challenge = get32be(m_node_rd);
        nodename_len       = static_cast<uintptr_t>(m_node_wr - m_node_rd);
#ifndef EI_DIST_6
    }
#else
    } else { /* NEW */
        m_dist_version     = EI_DIST_6;
        m_remote_flags     = get64be(m_node_rd);
        m_remote_challenge = get32be(m_node_rd);
        m_node_rd          += 4; /* ignore peer 'creation' */
        nodename_len       = get16be(m_node_rd);

        if (nodename_len > static_cast<uintptr_t>(m_node_wr - m_node_rd)) {
            std::stringstream ss;
            ss << "<- RECV_CHALLENGE 'N' (error) nodename too long from node '" 
               << this->remote_nodename() << "': " << nodename_len;
            this->handler()->on_connect_failure(this, ss.str());
            boost::system::error_code ec;
            m_socket.close(ec);
            return;
        }
    }
#endif

    if (nodename_len > MAXNODELEN) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE (error) nodename too long from node '" 
           << this->remote_nodename() << "': " << nodename_len;
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    if (!(m_remote_flags & DFLAG_EXTENDED_REFERENCES)) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE peer cannot handle extended references";
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    if (!(m_remote_flags & DFLAG_EXTENDED_PIDS_PORTS)) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE peer cannot handle extended pids and ports";
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    if (!(m_remote_flags & DFLAG_NEW_FLOATS)) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE peer cannot handle binary float encoding";
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE (ok) version=" << m_dist_version 
           << ", flags=" << m_remote_flags << ", challenge=" << m_remote_challenge;
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    uint8_t our_digest[16];
    gen_digest(m_remote_challenge, this->m_cookie.c_str(), our_digest);

    char* w = m_buf_node;
    size_t siz = 0;

    // send complement (if dist_version upgraded in-flight)
    if (m_dist_version > dist_version) {
        siz += 2 + 1 + 4 + 4;
        put16be(w, 9);
        put8(w, 'c');
        put32be(w, LOCAL_FLAGS >> 32 /* FlagsHigh */);
        put32be(w, this->local_creation());
    }

    // send challenge reply
    siz += 2 + 1 + 4 + 16;
    put16be(w, 21);
    put8(w, 'r');
    put32be(w, m_our_challenge);
    memcpy (w, our_digest, 16);

    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "-> SEND_CHALLENGE_REPLY sending " << siz << " bytes: "
           << to_binary_string(m_buf_node, siz);
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    m_state = CS_WAIT_WRITE_CHALLENGE_REPLY_DONE;
    boost::asio::async_write(m_socket, boost::asio::buffer(m_buf_node, siz),
        std::bind(&tcp_connection<Handler, Alloc>::handle_write_challenge_reply, shared_from_this(),
                    std::placeholders::_1));
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_write_challenge_reply(const boost::system::error_code& err)
{
    BOOST_ASSERT(m_state == CS_WAIT_WRITE_CHALLENGE_REPLY_DONE);
    if (err) {
        std::stringstream ss;
        ss << "-> SEND_CHALLENGE_REPLY (error) sending reply to node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }
    m_state = CS_WAIT_CHALLENGE_ACK;
    boost::asio::async_read(m_socket, boost::asio::buffer(m_buf_node, sizeof(m_buf_node)),
        boost::asio::transfer_at_least(2),
        std::bind(&tcp_connection<Handler, Alloc>::handle_read_challenge_ack_header, shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_read_challenge_ack_header(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_CHALLENGE_ACK);
    if (err) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE_ACK (error) reading ack header from node '" 
           << this->remote_nodename() << "': "
           << (err == boost::asio::error::eof ? "Possibly bad cookie?" : err.message());
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_rd = m_buf_node;
    m_expect_size = get16be(m_node_rd);

    if (m_expect_size > sizeof(m_buf_node) - 2) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE_ACK (error) in ack length from node '" 
           << this->remote_nodename() << "': " << m_expect_size;
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_wr = m_buf_node + bytes_transferred;

    size_t got_bytes = static_cast<uintptr_t>(m_node_wr - m_node_rd);
    size_t need_bytes = m_expect_size > got_bytes ? m_expect_size - got_bytes : 0;
    if (need_bytes > 0) {
        boost::asio::async_read(m_socket, 
            boost::asio::buffer(m_node_wr, static_cast<size_t>((m_buf_node+1)-m_node_wr)),
            boost::asio::transfer_at_least(need_bytes),
            std::bind(&tcp_connection<Handler, Alloc>::handle_read_challenge_ack_body,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    } else {
        handle_read_challenge_ack_body(boost::system::error_code(), 0);
    }
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::handle_read_challenge_ack_body(
    const boost::system::error_code& err, size_t bytes_transferred)
{
    BOOST_ASSERT(m_state == CS_WAIT_CHALLENGE_ACK);
    if (err) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE_ACK (error) reading ack body from node '" 
           << this->remote_nodename() << "': " << err.message();
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_node_wr += bytes_transferred;
    #ifndef NDEBUG
    size_t got_bytes = static_cast<uintptr_t>(m_node_wr - m_node_rd);
    #endif
    BOOST_ASSERT(got_bytes >= m_expect_size);

    const char tag = static_cast<char>(get8(m_node_rd));
    if (this->handler()->verbose() >= VERBOSE_TRACE) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE_ACK received (tag=" << tag << "): " 
           << to_binary_string(m_node_rd, m_expect_size-1);
        this->handler()->report_status(REPORT_INFO, ss.str());
    }

    if (tag != 'a') {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE_ACK incorrect tag from '" 
           << this->remote_nodename() << "', expected 'a' got '" << tag << "'";
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    char her_digest[16], expected_digest[16];
    memcpy(her_digest, m_node_rd, 16);
    gen_digest(m_our_challenge, this->m_cookie.c_str(), (uint8_t*)expected_digest);
    if (memcmp(her_digest, expected_digest, 16) != 0) {
        std::stringstream ss;
        ss << "<- RECV_CHALLENGE_ACK authorization failure for node '" 
           << this->remote_nodename() << "'!";
        this->handler()->on_connect_failure(this, ss.str());
        boost::system::error_code ec;
        m_socket.close(ec);
        return;
    }

    m_socket.set_option(boost::asio::ip::tcp::no_delay(true));
    m_socket.set_option(boost::asio::socket_base::keep_alive(true));

    m_state = CS_CONNECTED;

    this->start();
}

template <class Handler, class Alloc>
uint32_t tcp_connection<Handler, Alloc>::gen_challenge(void)
{
#if defined(__WIN32__)
    struct {
        SYSTEMTIME tv;
        DWORD cpu;
        int pid;
    } s;
    GetSystemTime(&s.tv);
    s.cpu  = GetTickCount();
    s.pid  = getpid();
    
#else
    struct { 
        struct timeval tv; 
        clock_t cpu; 
        pid_t pid; 
        long hid; 
        uid_t uid; 
        gid_t gid; 
        struct utsname name; 
    } s; 

    gettimeofday(&s.tv, 0); 
    uname(&s.name); 
    s.cpu  = clock(); 
    s.pid  = getpid(); 
    s.hid  = gethostid(); 
    s.uid  = getuid(); 
    s.gid  = getgid(); 
#endif
    return md_32((char*) &s, sizeof(s));
}

template <class Handler, class Alloc>
uint32_t tcp_connection<Handler, Alloc>::
md_32(char* string, size_t length)
{
    BOOST_STATIC_ASSERT(MD5_DIGEST_LENGTH == 16);
    union {
        char c[MD5_DIGEST_LENGTH];
        unsigned x[4];
    } digest;
    MD5((unsigned char *) string, length, (unsigned char *) digest.c);
    return (digest.x[0] ^ digest.x[1] ^ digest.x[2] ^ digest.x[3]);
    return (digest.x[0] ^ digest.x[1] ^ digest.x[2] ^ digest.x[3]);
}

template <class Handler, class Alloc>
void tcp_connection<Handler, Alloc>::
gen_digest(unsigned challenge, const char cookie[], uint8_t digest[16])
{
    MD5_CTX c;
    char chbuf[21];
    snprintf(chbuf, sizeof(chbuf), "%u", challenge);
    MD5_Init(&c);
    MD5_Update(&c, (unsigned char *) cookie, (uint32_t) strlen(cookie));
    MD5_Update(&c, (unsigned char *) chbuf,  (uint32_t) strlen(chbuf));
    MD5_Final(digest, &c);
}

} // namespace connect
} // namespace eixx

#endif // _EIXX_CONNECTION_TCP_IPP_

