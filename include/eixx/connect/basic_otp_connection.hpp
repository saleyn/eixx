//----------------------------------------------------------------------------
/// \file  basic_otp_connection.hpp
//----------------------------------------------------------------------------
/// \brief A class handling OTP connection session management.  It
///        implements functionality that allows to connect to an Erlang
///        OTP node and gives callbacks on important events and messages.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
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

#ifndef _EIXX_BASIC_OTP_CONNECTION_HPP_
#define _EIXX_BASIC_OTP_CONNECTION_HPP_

#include <memory>
#include <eixx/marshal/eterm.hpp>
#include <eixx/connect/transport_otp_connection.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

template <typename Alloc, typename Mutex> class basic_otp_node;

template <typename Alloc, typename Mutex>
class basic_otp_connection
    : public std::enable_shared_from_this<basic_otp_connection<Alloc,Mutex> > {
public:
    typedef basic_otp_connection<Alloc,Mutex> self;
    typedef connection<basic_otp_connection<Alloc,Mutex>, Alloc> connection_type;

    typedef boost::function<
        void (basic_otp_connection<Alloc,Mutex>* a_con,
              const std::string& err)>  connect_completion_handler;
private:
    boost::asio::io_service&            m_io_service;
    typename connection_type::pointer   m_transport;
    basic_otp_node<Alloc,Mutex>*        m_node;
    atom                                m_remote_nodename;
    atom                                m_cookie;
    const Alloc&                        m_alloc;
    connect_completion_handler          m_on_connect_status;
    bool                                m_connected;
    boost::asio::deadline_timer         m_reconnect_timer;
    int                                 m_reconnect_secs;
    bool                                m_abort;

    basic_otp_connection(
            connect_completion_handler      h,
            boost::asio::io_service&        a_svc,
            basic_otp_node<Alloc,Mutex>*    a_node,
            atom                            a_remote_nodename,
            atom                            a_cookie,
            int                             a_reconnect_secs = 0,
            const Alloc&                    a_alloc = Alloc())
        : m_io_service(a_svc)
        , m_node(a_node)
        , m_remote_nodename(a_remote_nodename)
        , m_cookie(a_cookie)
        , m_alloc(a_alloc)
        , m_connected(false)
        , m_reconnect_timer(m_io_service)
        , m_reconnect_secs(a_reconnect_secs)
        , m_abort(false)
    {
        BOOST_ASSERT(a_node != NULL);
        m_on_connect_status = h;
        m_transport = connection_type::create(
            m_io_service, this, a_node->nodename(),
            a_remote_nodename, a_cookie, a_alloc);
    }

    void reconnect() {
        if (m_abort || m_reconnect_secs <= 0)
            return;
        m_reconnect_timer.expires_from_now(boost::posix_time::seconds(m_reconnect_secs));
        m_reconnect_timer.async_wait(
            std::bind(&self::timer_reconnect, this->shared_from_this(),
            std::placeholders::_1));
    }

    void timer_reconnect(const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
            return;     // timer reset

        if (unlikely(verbose() >= VERBOSE_TRACE)) {
            std::stringstream s;
            s << "basic_otp_connection::timer_reconnect: " << ec.message();
            report_status(REPORT_INFO, s.str());
        }

        m_transport = connection_type::create(
            m_io_service, this, m_node->nodename(),
            m_remote_nodename, m_cookie, m_alloc);
    }

public:
    typedef std::shared_ptr<basic_otp_connection<Alloc,Mutex> > pointer;

    boost::asio::io_service&     io_service()             { return m_io_service;      }
    connection_type*             transport()              { return m_transport.get(); }
    verbose_type                 verbose()          const { return m_node->verbose(); }
    basic_otp_node<Alloc,Mutex>* node()                   { return m_node;            }
    atom                         remote_nodename()  const { return m_remote_nodename; }

    bool  connected()                               const { return m_connected;       }
    int   reconnect_timeout()                       const { return m_reconnect_secs;  }

    /// Set new reconnect timeout in seconds
    void reconnect_timeout(size_t a_reconnect_secs) { m_reconnect_secs = a_reconnect_secs; }

    static pointer
    connect(connect_completion_handler      h,
            boost::asio::io_service&        a_svc,
            basic_otp_node<Alloc,Mutex>*    a_node,
            atom                            a_remote_nodename,
            atom                            a_cookie,
            int                             a_reconnect_secs = 0,
            const Alloc&                    a_alloc = Alloc())
    {
        pointer p(new basic_otp_connection<Alloc,Mutex>(
            h, a_svc, a_node, a_remote_nodename, a_cookie, a_reconnect_secs, a_alloc));
        return p;
    }

    /// Bounce or permanently disconnect the connection.
    /// @param a_permanent if true will not result in subsequent reconnection attempts.
    ///             Otherwise will attempt to reconnect in m_reconnect_secs seconds.
    void disconnect(bool a_permanent = false) {
        m_abort = a_permanent;
        if (m_transport)
            m_transport->stop();
    }

    void send(const transport_msg<Alloc>& a_msg) throw (err_connection) {
        if (!m_transport) {
            if (m_abort)
                return;
            else
                throw err_connection("Not connected to node", remote_nodename());
        } else if (m_connected)
            m_transport->send(a_msg);
        // If not connected, the message sending will be ignored
    }

    /// Callback executed on successful connection.  It calls the connection
    /// status handler passed to the instance of this class on connect()
    /// call.
    void on_connect(connection_type* a_con) {
        BOOST_ASSERT(m_transport.get() == a_con);
        m_connected = true;
        if (m_on_connect_status)
            m_on_connect_status(this, std::string());
        if (unlikely(verbose() > VERBOSE_NONE)) {
            report_status(REPORT_INFO,
                "Connected to node: " + a_con->remote_nodename().to_string());
        }
    }

    /// Callback executed on failed connection attempt.  It calls the connection
    /// status handler passed to the instance of this class on connect()
    /// call. The second argument
    void on_connect_failure(connection_type* a_con, const std::string& a_error) {
        BOOST_ASSERT(m_transport.get() == a_con);
        m_connected = false;
        if (m_on_connect_status)
            m_on_connect_status(this, a_error);
        else if (unlikely(verbose() > VERBOSE_NONE)) {
            std::stringstream s;
            s << "Failed to connect to node "
              << a_con->remote_nodename() << ": " << a_error;
            report_status(REPORT_ERROR, s.str());
        }
        reconnect();
    }

    void on_disconnect(connection_type* a_con, const boost::system::error_code& err) {
        m_connected = false;

        if (unlikely(verbose() > VERBOSE_DEBUG)) {
            std::stringstream s;
            s << "Disconnected from node: " << a_con->remote_nodename()
              << " (" << err.message() << ')';
            report_status(REPORT_ERROR, s.str());
        }
        if (m_node)
            m_node->on_disconnect_internal(*this, a_con->remote_nodename(), err);

        m_transport.reset();
        reconnect();
    }

    void on_error(connection_type* a_con, const std::string& s) {
        std::stringstream str;
        str << "Error in communication with node: " << a_con->remote_nodename()
                  << "\n  " << s;
        report_status(REPORT_ERROR, str.str());
    }

    void on_message(connection_type* a_con, const transport_msg<Alloc>& a_tm) {
        try {
            m_node->deliver(a_tm);
        } catch (std::exception& e) {
            std::stringstream s;
            s << "Got message " << a_tm.type_string() << std::endl
                      << "  cntrl: " << a_tm.cntrl() << std::endl
                      << "  msg..: " << a_tm.msg() << std::endl
                      << "  error: " << e.what();
            report_status(REPORT_INFO, s.str());
        }
    }

    void report_status(report_level a_level, const std::string& s) {
        node()->report_status(a_level, this, s);
    }
};

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_BASIC_OTP_CONNECTION_HPP_

