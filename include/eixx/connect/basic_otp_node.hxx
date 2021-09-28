//----------------------------------------------------------------------------
/// \file  basic_otp_mailbox.hxx
//----------------------------------------------------------------------------
/// \brief Implementation of mailbox interface functions.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
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

#include <memory>
#include <eixx/connect/test_helper.hpp>
#include <eixx/marshal/eterm_match.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/concept_check.hpp>

namespace eixx {
namespace connect {

namespace {
    using namespace std::placeholders;
    using marshal::var;
}

//-----------------------------------------------------------------------------
// basic_otp_node::atom_con_hash_fun
//-----------------------------------------------------------------------------
template <typename Alloc, typename Mutex>
class basic_otp_node<Alloc, Mutex>::atom_con_hash_fun {
    static constexpr size_t s_default_max_ports = 16*1024;

    conn_hash_map& map;

    static size_t init_default_hash_size() {
        // See: https://erlang.org/doc/efficiency_guide/advanced.html#ports
        const char* p = getenv("EI_MAX_NODE_CONNECTIONS");
        size_t n = 0;
        if (p) {
            std::stringstream ss(p);
            ss >> n;
        }
        // if (n > 64*1024) n = 64*1024;
        return n > 0 ? n : s_default_max_ports;
    }
public:
    static size_t get_default_hash_size() {
        static const size_t s_max_node_connections = init_default_hash_size();
        return s_max_node_connections;
    }

    atom_con_hash_fun(conn_hash_map* a_map) : map(*a_map) {}

    size_t operator()(const atom& data) const {
        return data.index() % map.bucket_count();
    }
};

//-----------------------------------------------------------------------------
// basic_otp_node
//-----------------------------------------------------------------------------
template <typename Alloc, typename Mutex>
basic_otp_node<Alloc, Mutex>::
basic_otp_node(
    boost::asio::io_service& a_io_svc,
    const std::string& a_nodename, const std::string& a_cookie,
    const Alloc& a_alloc, int8_t a_creation)
    : basic_otp_node_local(a_nodename, a_cookie)
    , m_creation((a_creation < 0 ? time(NULL) : (int)a_creation) & 0x03)
    , m_pid_count(1)
    , m_port_count(1)
    , m_refid0(1)
    , m_refid1(0)
    , m_io_service(a_io_svc)
    , m_mailboxes(*this)
    , m_connections(atom_con_hash_fun::get_default_hash_size(), atom_con_hash_fun(&m_connections))
    , m_allocator(a_alloc)
    , m_verboseness(verboseness::level())
{}

template <typename Alloc, typename Mutex>
epid<Alloc> basic_otp_node<Alloc, Mutex>::
create_pid()
{
    #ifdef NEW_PID_EXT
    uint32_t n = m_pid_count.fetch_add(1, std::memory_order_relaxed);
    #else
    uint32_t n;
    while (true) {
        n = uint32_t(m_pid_count.fetch_add(1, std::memory_order_relaxed));
        if (n < 0x0fffffff /* 28 bits */) break;

        if (m_pid_count.exchange(1, std::memory_order_acquire) == 1) {
            n = 1;
            break;
        }
    }
    #endif

    return epid<Alloc>(m_nodename, n, m_creation, m_allocator);
}

template <typename Alloc, typename Mutex>
port<Alloc> basic_otp_node<Alloc, Mutex>::
create_port()
{
    #ifdef NEW_PID_EXT
    uint64_t n = m_port_count.fetch_add(1, std::memory_order_relaxed);
    #else
    uint64_t n;
    while (true) {
        n = m_port_count.fetch_add(1, std::memory_order_relaxed);
        if (n < 0x0fffffff /* 28 bits */) break;

        if (m_port_count.exchange(1, std::memory_order_acquire) == 1) {
            n = 1;
            break;
        }
    }
    #endif

    return port<Alloc>(m_nodename, n, m_creation, m_allocator);
}

template <typename Alloc, typename Mutex>
ref<Alloc> basic_otp_node<Alloc, Mutex>::
create_ref()
{
    uint_fast64_t      n;
    decltype(m_refid1) mo;

    while (true) {
        mo = m_refid1.load(std::memory_order_consume);
        n  = m_refid0.fetch_add(1, std::memory_order_relaxed);
        if (n) break;

        auto mn = mo+1;

        #ifndef NEWER_REFERENCE_EXT
        if (mn > 0x3ffff) mn = 0;
        #endif

        if (m_refid1.compare_exchange_weak(mo, mn, std::memory_order_acquire))
            break;
    }
    #ifndef NEWER_REFERENCE_EXT
    return ref<Alloc>(m_nodename, {mo >> 32, mo & 0xFFFFffff, n >> 32, n & 0xFFFFffff},
                      m_creation, m_allocator);
    #else
    return ref<Alloc>(m_nodename, mo, n, m_creation, m_allocator);
    #endif
}

template <typename Alloc, typename Mutex>
inline basic_otp_mailbox<Alloc, Mutex>*
basic_otp_node<Alloc, Mutex>::
create_mailbox(const atom& a_name, boost::asio::io_service* a_svc)
{
    boost::asio::io_service* p_svc = a_svc ? a_svc : &m_io_service;
    return m_mailboxes.create_mailbox(a_name, p_svc);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
close_mailbox(basic_otp_mailbox<Alloc, Mutex>* a_mbox)
{
    if (a_mbox)
        m_mailboxes.erase(a_mbox);
}


template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
set_nodename(const atom& a_nodename, const std::string& a_cookie)
{
    close();
    if (a_nodename != atom())
        basic_otp_node_local::set_nodename(a_nodename.to_string(), a_cookie);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
close()
{
    m_mailboxes.clear();
    for(typename conn_hash_map::iterator
        it = m_connections.begin(), end = m_connections.end(); it != end; ++it)
        it->second->disconnect();
    m_connections.clear();
}

template <typename Alloc, typename Mutex>
template <typename CompletionHandler>
inline void basic_otp_node<Alloc, Mutex>::
connect(CompletionHandler h, const atom& a_remote_nodename, int a_reconnect_secs)
{
    connect(h, a_remote_nodename, atom(), a_reconnect_secs);
}

template <typename Alloc, typename Mutex>
inline typename basic_otp_node<Alloc, Mutex>::connection_t&
basic_otp_node<Alloc, Mutex>::
connection(atom a_nodename) const
{
    auto l_con = m_connections.find(a_nodename);
    if (l_con == m_connections.end())
        throw err_connection("Not connected to node", a_nodename);
    return *l_con->second.get();
}

template <typename Alloc, typename Mutex>
template <typename CompletionHandler>
void basic_otp_node<Alloc, Mutex>::
connect(CompletionHandler h, const atom& a_remote_node, const atom& a_cookie,
        int a_reconnect_secs)
{
    lock_guard<Mutex> guard(m_lock);
    typename conn_hash_map::iterator it = m_connections.find(a_remote_node);
    if (it == m_connections.end()) {
        atom l_cookie = a_cookie.empty() ? cookie() : a_cookie;
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
void basic_otp_node<Alloc, Mutex>::
on_disconnect_internal(const connection_t& a_con,
    atom a_remote_nodename, const boost::system::error_code& err)
{
    if (on_disconnect)
        on_disconnect(*this, a_con, a_remote_nodename, err);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
rpc_call(const epid<Alloc>& a_from, const ref<Alloc>& a_ref,
    const atom& a_mod, const atom& a_fun, const list<Alloc>& a_args,
    const eterm<Alloc>& a_gleader)
{
    auto res = on_rpc_call
        ? on_rpc_call(a_from, a_ref, a_mod, a_fun, a_args, a_gleader)
        : eterm<Alloc>(
            tuple<Alloc>::make(a_ref,
                tuple<Alloc>::make(am_error, am_unsupported)));
    send(a_from, res);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
publish_port()
{
    throw err_connection("Not implemented!");
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
unpublish_port()
{
    throw std::runtime_error("Not implemented");
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::start_server()
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
void basic_otp_node<Alloc, Mutex>::
deliver(const transport_msg<Alloc>& a_msg)
{
    try {
        const eterm<Alloc>& l_to = a_msg.recipient();
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
void basic_otp_node<Alloc, Mutex>::
send(const atom& a_to_node, ToProc a_to, const transport_msg<Alloc>& a_msg)
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
void inline basic_otp_node<Alloc, Mutex>::
send(const epid<Alloc>& a_to, const eterm<Alloc>& a_msg)
{
    transport_msg<Alloc> tm;
    tm.set_send(a_to, a_msg, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send(const atom& a_node, const epid<Alloc>& a_to, const eterm<Alloc>& a_msg)
{
    transport_msg<Alloc> tm;
    tm.set_send(a_to, a_msg, m_allocator);
    send(a_node, a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send(const epid<Alloc>& a_from, const atom& a_to, const eterm<Alloc>& a_msg)
{
    transport_msg<Alloc> tm;
    tm.set_reg_send(a_from, a_to, a_msg, m_allocator);
    send(nodename(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send(const epid<Alloc>& a_from, const atom& a_to_node, const atom& a_to, const eterm<Alloc>& a_msg)
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
{
    transport_msg<Alloc> tm;
    tm.set_exit(a_from, a_to, a_reason, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void inline basic_otp_node<Alloc, Mutex>::
send_exit2(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
          const eterm<Alloc>& a_reason)
{
    transport_msg<Alloc> tm;
    tm.set_exit2(a_from, a_to, a_reason, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
send_link(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
{
    transport_msg<Alloc> tm;
    tm.set_link(a_from, a_to, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
send_unlink(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
{
    transport_msg<Alloc> tm;
    tm.set_unlink(a_from, a_to, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
const ref<Alloc>& basic_otp_node<Alloc, Mutex>::
send_monitor(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
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
{
    transport_msg<Alloc> tm;
    tm.set_demonitor(a_from, a_to, a_ref, m_allocator);
    send(a_to.node(), a_to, tm);
}

template <typename Alloc, typename Mutex>
void basic_otp_node<Alloc, Mutex>::
send_monitor_exit(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
                  const ref<Alloc>& a_ref, const eterm<Alloc>& a_reason)
{
    transport_msg<Alloc> tm;
    tm.set_monitor_exit(a_from, a_to, a_ref, a_reason, m_allocator);
    send(a_to.node(), a_to, tm);
}

} // namespace connect
} // namespace eixx
