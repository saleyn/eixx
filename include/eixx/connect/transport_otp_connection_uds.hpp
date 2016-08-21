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

#ifndef _EIXX_TRANSPORT_OTP_CONNECTION_UDS_HPP_
#define _EIXX_TRANSPORT_OTP_CONNECTION_UDS_HPP_

#include <eixx/connect/transport_otp_connection.hpp>

namespace eixx {
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
} // namespace eixx

#endif // _EIXX_TRANSPORT_OTP_CONNECTION_UDS_HPP_
