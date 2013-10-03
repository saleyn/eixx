//----------------------------------------------------------------------------
/// \file  basic_otp_mailbox.ipp
//----------------------------------------------------------------------------
/// \brief Implementation of mailbox interface functions.
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

#include <eixx/connect/test_helper.hpp>
#include <boost/interprocess/detail/atomic.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

//////////////////////////////////////////////////////////////////////////
// enode_local
template <typename Alloc, typename Mutex>
basic_otp_node<Alloc, Mutex>::basic_otp_node(
    boost::asio::io_service& a_io_svc,
    const atom& a_nodename, const std::string& a_cookie,
    const Alloc& a_alloc, int8_t a_creation)
    throw (err_bad_argument, err_connection, eterm_exception)
    : basic_otp_node_local(a_nodename.to_string(), a_cookie)
    , m_creation((a_creation < 0 ? time(NULL) : (int)a_creation) & 0x03)  // Creation counter
    , m_pid_count(1)
    , m_port_count(1)
    , m_serial(0)
    , m_io_service(a_io_svc)
    , m_mailboxes(*this)
    , m_connections(atom_con_hash_fun::get_default_hash_size(), atom_con_hash_fun(&m_connections))
    , m_allocator(a_alloc)
{
    // Init the counters
    m_refid[0]      = 1;
    m_refid[1]      = 0;
    m_refid[2]      = 0;
    m_verboseness   = verboseness::level();
}

template <typename Alloc, typename Mutex>
epid<Alloc> basic_otp_node<Alloc, Mutex>::create_pid() {
    lock_guard<Mutex> guard(m_inc_lock);
    epid<Alloc> p(m_nodename, m_pid_count++, m_serial, m_creation, m_allocator);
    if (m_pid_count > 0x7fff) {
        m_pid_count = 0;
        m_serial = (m_serial + 1) & 0x1fff; /* 13 bits */
    }
    return p;
}

template <typename Alloc, typename Mutex>
port<Alloc> basic_otp_node<Alloc, Mutex>::create_port() {
    lock_guard<Mutex> guard(m_inc_lock);
    port<Alloc> p(m_nodename, m_port_count++, m_creation, m_allocator);

    if (m_port_count > 0x0fffffff) /* 28 bits */
        m_port_count = 0;

    return p;
}

template <typename Alloc, typename Mutex>
ref<Alloc> basic_otp_node<Alloc, Mutex>::create_ref() {
    lock_guard<Mutex> guard(m_inc_lock);
    ref<Alloc> r(m_nodename, m_refid, m_creation, m_allocator);

    // increment ref ids (3 ints: 18 + 32 + 32 bits)
    if (++m_refid[0] > 0x3ffff) {
        m_refid[0] = 0;

        if (++m_refid[1] == 0)
            ++m_refid[2];
    }

    return r;
}

template <typename Alloc, typename Mutex>
template <typename CompletionHandler>
void basic_otp_node<Alloc, Mutex>::connect(
    CompletionHandler h, const atom& a_remote_node, const std::string& a_cookie,
    size_t a_reconnect_secs)
    throw(err_connection)
{
    lock_guard<Mutex> guard(m_lock);
    typename conn_hash_map::iterator it = m_connections.find(a_remote_node);
    if (it == m_connections.end()) {
        const std::string& l_cookie = a_cookie.empty() ? cookie() : a_cookie;
        typename connection_t::pointer con(
            connection_t::connect(h, m_io_service, this, a_remote_node,
                                  l_cookie, a_reconnect_secs));
        m_connections[a_remote_node] = con;
    } else {
        std::string e;
        m_io_service.post(std::bind(h, &*it->second, e));
    }
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::publish_port() throw (err_connection)
{
    throw err_connection("Not implemented!");
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::unpublish_port() throw (err_connection)
{
    throw std::runtime_error("Not implemented");
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::start_server() throw(err_connection)
{
    throw std::runtime_error("Not implemented");
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::stop_server()
{
    throw std::runtime_error("Not implemented");
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
report_status(report_level a_level, const connection_t* a_con, const std::string& s)
{
    static const char* s_levels[] = {"INFO", "WARN", "ERROR"};
    if (on_status)
        on_status(*this, a_con, a_level, s);
    else
        std::cerr << s_levels[a_level] << "| " << s << std::endl;
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::deliver(const transport_msg<Alloc>& a_msg)
    throw (err_bad_argument, err_no_process, err_connection)
{
    try {
        const eterm<Alloc>& l_to = a_msg.to();
        basic_otp_mailbox<Alloc, Mutex>* l_mbox = get_mailbox(l_to);
        l_mbox->deliver(a_msg);
    } catch (std::exception& e) {
        // FIXME: Add proper error reporting.
        std::stringstream s;
        s << "Cannot deliver message " << a_msg.to_string() << ": " << e.what();
        report_status(REPORT_WARNING, NULL, s.str());
    }
}

template <typename Alloc, typename Mutex>
template <typename ToProc>
void basic_otp_node<Alloc, Mutex>::send(
    const atom& a_to_node, ToProc a_to, const transport_msg<Alloc>& a_msg)
    throw (err_no_process, err_connection)
{
    if (a_to_node == nodename()) {
        basic_otp_mailbox<Alloc, Mutex>* mbox = m_mailboxes.get(a_to);
        if (!mbox)
            throw err_no_process(eterm<Alloc>::cast(a_to).to_string());
        mbox->deliver(a_msg);
    } else {
        connection_t& l_con = connection(a_to_node);
        l_con.send(a_msg);
    }
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::send(
    const epid<Alloc>& a_to, const eterm<Alloc>& a_msg)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_send(a_to, a_msg, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::send(
    const atom& a_node, const epid<Alloc>& a_to, const eterm<Alloc>& a_msg)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_send(a_to, a_msg, m_allocator);
    send(a_node, a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::send(
    const epid<Alloc>& a_from, const atom& a_to, const eterm<Alloc>& a_msg)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_reg_send(a_from, a_to, a_msg, m_allocator);
    send(nodename(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::send(
    const epid<Alloc>& a_from, const atom& a_to_node, const atom& a_to, const eterm<Alloc>& a_msg)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_reg_send(a_from, a_to, a_msg, m_allocator);
    send(a_to_node, a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send_rpc(const epid<Alloc>& a_from,
         const atom& a_node, const atom& a_mod, const atom& a_fun, 
         const list<Alloc>& args, const epid<Alloc>* gleader)
    throw (err_bad_argument, err_no_process, err_connection)
{
    static const atom rex("rex");
    transport_msg<Alloc> tm;
    tm.set_send_rpc(a_from, a_mod, a_fun, args, gleader, m_allocator);
    send(a_node, rex, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send_rpc_cast(const epid<Alloc>& a_from,
         const atom& a_node, const atom& a_mod, const atom& a_fun,
         const list<Alloc>& args, const epid<Alloc>* gleader)
    throw (err_bad_argument, err_no_process, err_connection)
{
    static const atom rex("rex");
    transport_msg<Alloc> tm;
    tm.set_send_rpc_cast(a_from, a_mod, a_fun, args, gleader, m_allocator);
    send(a_node, rex, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send_exit(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
          const eterm<Alloc>& a_reason)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_exit(a_from, a_to, a_reason, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send_exit2(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
          const eterm<Alloc>& a_reason)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_exit2(a_from, a_to, a_reason, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
send_link(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_link(a_from, a_to, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
send_unlink(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_unlink(a_from, a_to, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
const ref<Alloc>& basic_otp_node<Alloc, Mutex>::
send_monitor(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    ref<Alloc> r = create_ref();
    tm.set_monitor(a_from, a_to, r, m_allocator);
    send(a_to.node(), a_to, tm);
    return r;
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
send_demonitor(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
               const ref<Alloc>& a_ref)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_demonitor(a_from, a_to, a_ref, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
send_monitor_exit(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
                  const ref<Alloc>& a_ref, const eterm<Alloc>& a_reason)
    throw (err_no_process, err_connection)
{
    transport_msg<Alloc> tm;
    tm.set_monitor_exit(a_from, a_to, a_ref, a_reason, m_allocator);
    send(a_to.node(), a_to, tm);
}

} // namespace connect
} // namespace EIXX_NAMESPACE
