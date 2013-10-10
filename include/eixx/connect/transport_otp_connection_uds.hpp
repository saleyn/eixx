//----------------------------------------------------------------------------
/// \file transport_otp_connection_uds.hpp
//----------------------------------------------------------------------------
/// \brief Implementation of connection handling Unix Domain Socket 
///        communications.
/// Note that this UDS functionality is incomplete because Erlang doesn't
/// provide UDS connection setup by default.
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

#ifndef _EIXX_TRANSPORT_OTP_CONNECTION_UDS_HPP_
#define _EIXX_TRANSPORT_OTP_CONNECTION_UDS_HPP_

#include <eixx/connect/transport_otp_connection.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

//----------------------------------------------------------------------------
/// \class uds_connection
/// \brief UDS connection channel
/// @todo This implementation wasn't tested, because the agent UDS
///       functionality is currently not implemented.
//----------------------------------------------------------------------------
template <class Handler, class Alloc>
class uds_connection
    : public connection<Handler, Alloc>
{
public:
    typedef connection<Handler, Alloc> base_t;

    uds_connection(boost::asio::io_service& a_svc, Handler* a_h, const Alloc& a_alloc)
        : connection<Handler, Alloc>(UDS, a_svc, a_h, a_alloc)
        , m_socket(a_svc)
    {}

    /// Get the socket associated with the connection.
    boost::asio::local::stream_protocol::socket& socket() { return m_socket; }

    void start() {
        boost::asio::local::stream_protocol::socket::non_blocking_io nb(true);
        m_socket.io_control(nb);
        base_t::start();
    }

    void stop() { m_socket.close(); base_t::stop(); }

    std::string peer_address() const {
        return m_uds_filename;
    }

    int native_socket() { return m_socket.native(); }

private:
    /// Socket for the connection.
    boost::asio::local::stream_protocol::socket m_socket;
    std::string m_uds_filename;

    void connect(atom a_this_node, atom a_remote_nodename, atom a_cookie)
        throw(std::runtime_error)
    {
        base_t::connect(a_this_node, a_remote_nodename, a_cookie);

        boost::system::error_code err;
        boost::asio::local::stream_protocol::endpoint endpoint(a_remote_nodename.to_string());
        m_socket.connect(endpoint, err);
        if (err)
            THROW_RUNTIME_ERROR("Error connecting to: " << m_uds_filename 
                << ':' << err.message());
        auto s = a_remote_nodename.to_string();
        auto n = s.find_last_of('/');
        if (n != std::string::npos) s.erase(n);
        this->m_remote_nodename = atom(s);
        m_uds_filename = a_remote_nodename.to_string();
        this->start();
    }

};

//------------------------------------------------------------------------------
// implementation
//------------------------------------------------------------------------------

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_TRANSPORT_OTP_CONNECTION_UDS_HPP_
