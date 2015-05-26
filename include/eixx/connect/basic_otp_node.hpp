//----------------------------------------------------------------------------
/// \file  basic_otp_node.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing OTP node functionality.
///        A node represents an object that can connect to other nodes
///        in the network.  Usually there is one node per executable program.
///        A node can have many mailboxes (\see otp_mailbox) and many 
///        connections (\see otp_connection) to other nodes.
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

#ifndef _EIXX_BASIC_OTP_NODE_HPP_
#define _EIXX_BASIC_OTP_NODE_HPP_

#include <atomic>
#include <time.h>
#include <boost/function.hpp>
#include <eixx/connect/basic_otp_node_local.hpp>
#include <eixx/connect/basic_otp_connection.hpp>
#include <eixx/connect/basic_otp_mailbox_registry.hpp>
#include <eixx/connect/transport_msg.hpp>
#include <eixx/connect/verbose.hpp>
#include <eixx/util/sync.hpp>
#include <eixx/marshal/eterm.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

using namespace EIXX_NAMESPACE;
using marshal::epid;
using marshal::port;
using marshal::ref;

/**
 * \brief Manually managed node.
 * An OTP node class implements a node that can connect to other distributed 
 * nodes.  Usually there is one instance of a node per application.  In order
 * for a node to connect to other nodes it must be provided with a cookie.
 * The default cookie is read from the "$HOME/.erlang.cookie" file.  A node
 * will be rejected connectivity to any other node that is not started with
 * the same cookie.
 *
 * A node is a container of basic_otp_mailboxes and basic_otp_connections.
 * Every mailbox has a unique epid assigned to it and can be used to 
 * deliver messages to its queue.  A connection is associated with a TCP
 * transport connection that allows sending and receiving distributed 
 * messages to its peer node.
 */
template <typename Alloc, typename Mutex = detail::mutex>
class basic_otp_node: public basic_otp_node_local {
    class atom_con_hash_fun;

    typedef basic_otp_node<Alloc, Mutex>        self;
    typedef transport_msg<Alloc>                transport_msg_t;
    typedef basic_otp_connection<Alloc,Mutex>   connection_t;

    typedef EIXX_NAMESPACE::detail::hash_map_base<
        atom, typename connection_t::pointer, atom_con_hash_fun
    > conn_hash_map;

    class rpc_server;

    uint8_t                                     m_creation;
    std::atomic_int                             m_pid_count;
    std::atomic_int                             m_port_count;
    std::atomic_uint_fast64_t                   m_refid0;
    std::atomic_int                             m_refid1;

    boost::asio::io_service&                    m_io_service;
    Mutex                                       m_lock;
    basic_otp_mailbox_registry<Alloc, Mutex>    m_mailboxes;
    conn_hash_map                               m_connections;
    Alloc                                       m_allocator;
    verbose_type                                m_verboseness;

    friend class basic_otp_connection<Alloc, Mutex>;

    void on_disconnect_internal(const connection_t& a_con,
        atom a_remote_nodename, const boost::system::error_code& err);

    void report_status(report_level a_level,
        const connection_t* a_con, const std::string& s);
    void rpc_call(const epid<Alloc>& a_from, const ref<Alloc>& a_ref,
        const atom& a_mod, const atom& a_fun, const list<Alloc>& a_args,
        const eterm<Alloc>& a_gleader);

protected:
    /// Publish the node port to epmd making this node known to the world.
    void publish_port() throw (err_connection);

    /// Unregister this node from epmd.
    void unpublish_port() throw (err_connection);

    /// Send a message to a process ToProc which is either epid<Alloc> or
    /// atom<Alloc> for registered names.
    template <typename ToProc>
    void send(const atom& a_to_node,
        ToProc a_to, const transport_msg<Alloc>& a_msg)
        throw (err_no_process, err_connection);
public:
    typedef basic_otp_mailbox_registry<Alloc, Mutex> mailbox_registry_t;

    /**
     * Create a new node, using given cookie
     * @param a_io_svc is the I/O service to use.
     * @param a_nodename node identifier with the protocol, alivename,
     *  hostname and port. ex: "ei:mynode@host.somewhere.com:3128"
     * @param a_cookie cookie to use
     * @param a_alloc is the allocator to use
     * @param a_creation is the creation value to use (0 .. 2). This
     *        argument is provided for being able to do determinitic testing.
     *        In production pass the default value, so that the creation value
     *        is determined automatically.
     * @throws err_bad_argument if node name is too long
     * @throws err_connection if there is a network problem
     * @throws eterm_exception if there is an error in transport creation
     */
    basic_otp_node(boost::asio::io_service& a_io_svc,
                   const std::string& a_nodename = std::string(),
                   const std::string& a_cookie = std::string(),
                   const Alloc& a_alloc = Alloc(),
                   int8_t a_creation = -1)
        throw (err_bad_argument, err_connection, eterm_exception);

    virtual ~basic_otp_node() { close(); }

    /// Change name of current node
    void set_nodename(const atom& a_nodename, const std::string& a_cookie = "");

    /// Get current verboseness level
    verbose_type verbose() const { return m_verboseness; }

    /// Set verboseness level.  This controls the amount of debugging
    /// printouts.
    void verbose(verbose_type a_type) { m_verboseness = a_type; }

    /// Get the service object used by this node.
    boost::asio::io_service& io_service() { return m_io_service; }

    /// Run the node's service dispatch
    void run()  { m_io_service.run();  }
    /// Stop the node's service dispatch
    void stop() { m_io_service.stop(); }

    /// Close all connections and empty the mailbox
    void close();

    /**
     * Create a new mailbox with a new mailbox that can be used to send and
     * receive messages with other mailboxes and with remote Erlang processes.
     * Messages can be sent to this mailbox by using its associated
     * {@link mailbox#self pid}.
     * @param a_reg_name if not null the mailbox pid will be associated with a
     *        registered name.
     * @return new mailbox with a new pid.
     */
    basic_otp_mailbox<Alloc, Mutex>*
    create_mailbox(const atom& a_name = atom(), boost::asio::io_service* a_svc = NULL);

    void close_mailbox(basic_otp_mailbox<Alloc, Mutex>* a_mbox);

    /// Get a mailbox associated with a given atom name or epid.
    /// @param a_proc is either an atom name of a registered local process or epid.
    /// @returns NULL if there's no mailbox associated with given identifier or
    ///          a mailbox smart pointer.
    basic_otp_mailbox<Alloc, Mutex>*
    get_mailbox(const eterm<Alloc>& a_proc) const { return m_mailboxes.get(a_proc); }

    /// Get a mailbox registered by a given atom name.
    basic_otp_mailbox<Alloc, Mutex>*
    get_mailbox(atom a_name)                const { return m_mailboxes.get(a_name); }

    /// Get a mailbox registered by a given epid.
    basic_otp_mailbox<Alloc, Mutex>*
    get_mailbox(const epid<Alloc>& a_pid)   const { return m_mailboxes.get(a_pid); }

    const mailbox_registry_t& registry()    const { return m_mailboxes; }

    /// Register mailbox by given name
    bool register_mailbox(const atom& a_name, basic_otp_mailbox<Alloc, Mutex>& a_mbox);

    /// Create a new unique pid
    epid<Alloc> create_pid();

    /// Create a new unique port
    port<Alloc> create_port();

    /// Create a new unique ref
    ref<Alloc> create_ref();

    /// Get creation number
    uint8_t creation() const { return m_creation; }

    /**
     * Set up a connection to an Erlang node, using given cookie
     * @param a_remote_node node name to connect
     * @param a_cookie cookie to use
     * @param a_reconnect_secs is the interval in seconds between reconnecting
     *          attempts in case of connection drops.
     * @returns A new connection. The connection has no receiver defined
     * and is not started.
     */
    template <typename CompletionHandler>
    void connect(CompletionHandler h, const atom& a_remote_node,
                 const atom& a_cookie = atom(), size_t a_reconnect_secs = 0)
        throw(err_connection);

    /**
     * Set up a connection to an Erlang node, using default cookie
     * @param a_remote_node node name to connect
     * @param a_reconnect_secs is the interval in seconds between reconnecting
     *          attempts in case of connection drops.
     * @returns A new connection. The connection has no receiver defined
     * and is not started.
     */
    template <typename CompletionHandler>
    void connect(CompletionHandler h, const atom& a_remote_nodename,
                 size_t a_reconnect_secs = 0) throw(err_connection);

    /// Get connection identified by the \a a_node name.
    /// @throws err_connection if not connected to \a a_node._
    connection_t& connection(atom a_nodename) const;

    /**
     * Callback invoked on disconnect from a peer node
     */
    boost::function<
        //    OtpNode      OtpConnection  RemoteNodeName         ErrorCode
        void (self&, const connection_t&, atom, const boost::system::error_code&)
    > on_disconnect;

    /**
     * Callback invoked if verbosity is different from VERBOSE_NONE. If not assigned,
     * the content is printed to stderr.
     */
    boost::function<
        //    OtpNode      OtpConnection     Status         Message
        void (self&, const connection_t*, report_level, const std::string&)
    > on_status;

    boost::function<
        eterm<Alloc> (const epid<Alloc>& a_from, const ref<Alloc>& a_ref,
                      const atom& a_mod, const atom& a_fun, const list<Alloc>& a_args,
                      const eterm<Alloc>& a_gleader)
    > on_rpc_call;
    /**
     * Accept connections from client processes.
     * This method sets the socket listener for incoming connections and
     * registers the port with local epmd daemon.
     * @throws err_connection if cannot connect to epmd.
     */
    void start_server() throw(err_connection);

    /**
     * Stop accepting connections from client processes.
     * This method closes the socket listener and 
     * unregisters the port with local epmd daemon.
     */
    void stop_server();

    /// Deliver a message to its local receipient mailbox.
    void deliver(const transport_msg<Alloc>& a_tm)
        throw (err_bad_argument, err_no_process, err_connection);

    /// Send a message \a a_msg from \a a_from pid to \a a_to pid.
    /// @param a_to is a remote process.
    /// @param a_msg is the message to send.
    void send(const epid<Alloc>& a_to, const eterm<Alloc>& a_msg)
        throw (err_no_process, err_connection);

    /// Send a message \a a_msg to the remote process \a a_to on node \a a_node.
    /// The remote process \a a_to need not belong to node \a a_node.
    /// @param a_node is the node to send the message to.
    /// @param a_to is a remote process.
    /// @param a_msg is the message to send.
    void send(const atom& a_node, const epid<Alloc>& a_to, const eterm<Alloc>& a_msg)
        throw (err_no_process, err_connection);

    /// Send a message \a a_msg to the local process registered as \a a_to.
    void send(const epid<Alloc>& a_from, const atom& a_to, const eterm<Alloc>& a_msg)
        throw (err_no_process, err_connection);

    /// Send a message \a a_msg to the process registered as \a a_to_name
    /// on remote node \a a_node.
    void send(const epid<Alloc>& a_from, const atom& a_to_node, const atom& a_to_name,
        const eterm<Alloc>& a_msg) throw (err_no_process, err_connection);

    /**
	 * Send an RPC request to a remote Erlang node.
	 * @param a_from the caller's mailbox pid.
     * @param a_to_node remote node where execute the funcion.
	 * @param a_mod the name of the Erlang module containing the
	 * function to be called.
	 * @param a_fun the name of the function to call.
	 * @param args a list of Erlang terms, to be used as arguments
	 * to the function.
	 * @param gleader is optional group leader's pid that will receive
	 *                io requests from the caller.
     * @throws err_bad_argument if function, module or nodename are too big
     * @throws err_no_process if this is a local request and there's no rex mailbox.
     * @throws err_connection if there's no connection to \a a_node.
	 */
    void send_rpc(const epid<Alloc>& a_from, const atom& a_to_node,
                  const atom& a_mod, const atom& a_fun, const list<Alloc>& args,
                  const epid<Alloc>* gleader = NULL)
        throw (err_bad_argument, err_no_process, err_connection);

    /// Execute an equivalent of rpc:cast(...). Doesn't return any value.
    void send_rpc_cast(const epid<Alloc>& a_from, const atom& a_to_node,
                   const atom& a_mod, const atom& a_fun, const list<Alloc>& args,
                  const epid<Alloc>* gleader = NULL)
        throw (err_bad_argument, err_no_process, err_connection);

    /// Attempt to kill a remote process by sending
    /// an exit message to a_pid, with reason \a a_reason
    void send_exit(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const eterm<Alloc>& a_reason) throw (err_no_process, err_connection);

    /// Attempt to kill a remote process by sending
    /// an exit2 message to a_pid, with reason \a a_reason
    void send_exit2(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const eterm<Alloc>& a_reason) throw (err_no_process, err_connection);

    /// Link mailbox to the given pid.
    /// The given pid will receive an exit message when \a a_pid dies.
    /// @throws err_no_process
    /// @throws err_connection
    void send_link(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
        throw (err_no_process, err_connection);

    /// UnLink the given pid
    void send_unlink(const epid<Alloc>& a_from, const epid<Alloc>& a_to)
        throw (err_no_process, err_connection);

    const ref<Alloc>&
    send_monitor(const epid<Alloc>& a_from, const epid<Alloc>& a_to_pid)
        throw (err_no_process, err_connection);

    /// Demonitor the \a a_to pid monitored by \a a_from pid using \a a_ref reference.
    void send_demonitor(const epid<Alloc>& a_from, const epid<Alloc>& a_to, const ref<Alloc>& a_ref)
        throw (err_no_process, err_connection);

    void send_monitor_exit(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const ref<Alloc>& a_ref, const eterm<Alloc>& a_reason)
        throw (err_no_process, err_connection);

};

} // namespace connect
} // namespace EIXX_NAMESPACE

#include <eixx/connect/basic_otp_node.ipp>
#include <eixx/connect/detail/basic_rpc_server.hpp>

#endif // _EIXX_BASIC_OTP_NODE_HPP_
