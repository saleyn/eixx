//----------------------------------------------------------------------------
/// \file  basic_otp_mailbox.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing mailbox functionality.
///        The mailbox is a queue of messages identified by a pid.
///        It implements asynchronous reception of messages.
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

#ifndef _EIXX_BASIC_OTP_MAILBOX_HPP_
#define _EIXX_BASIC_OTP_MAILBOX_HPP_

#include <boost/asio.hpp>
#include <eixx/util/async_wait_timeout.hpp>
#include <eixx/util/async_queue.hpp>
#include <eixx/marshal/eterm.hpp>
#include <eixx/connect/transport_msg.hpp>
#include <eixx/connect/verbose.hpp>
#include <eixx/eterm.hpp>
#include <chrono>
#include <list>
#include <set>

namespace eixx {
namespace connect {

template<typename Alloc, typename Mutex> class basic_otp_node;

using eixx::marshal::list;
using eixx::marshal::epid;
using eixx::marshal::tuple;
using eixx::marshal::varbind;
using namespace std::chrono;

/**
 * Provides a simple mechanism for exchanging messages with Erlang
 * processes or other instances of this class.
 *
 * Mailbox is the way to send an receive messages. Outgoing messages
 * will be forwarded to another local mailbox or a pid on another node.
 *
 * MailBox will receive and queue messages
 * delivered with the deliver() method. The messages can be
 * dequeued using the receive() family of functions.
 *
 * You can get the message in order of arrival, or use pattern matching
 * to explore the message queue.
 *
 * Each mailbox is associated with a unique {@link epid
 * pid} that contains information necessary for message delivery.
 *
 * Messages to remote nodes are encoded in the Erlang external binary
 * format for transmission.
 *
 * Mailboxes can be linked and monitored in much the same way as
 * Erlang processes. If a link is active when a mailbox is {@link
 * #close closed}, any linked Erlang processes or mailboxes will be
 * sent an exit signal.
 *
 * TODO: When retrieving messages from a mailbox that has received an exit
 * signal, an {@link OtpErlangExit OtpErlangExit} exception will be
 * raised. Note that the exception is queued in the mailbox along with
 * other messages, and will not be raised until it reaches the head of
 * the queue and is about to be retrieved.
 *
 */
template <typename Alloc, typename Mutex>
class basic_otp_mailbox
{
public:
    using pointer = boost::shared_ptr<basic_otp_mailbox<Alloc, Mutex>>;

    typedef std::function<
        bool (basic_otp_mailbox<Alloc, Mutex>&, transport_msg<Alloc>*&)
    > receive_handler_type;

    typedef util::async_queue<transport_msg<Alloc>*, Alloc> queue_type;

    template<typename A, typename M> friend class basic_otp_node;
    template<typename T, typename A> friend struct util::async_queue;
    template<typename A, typename M> friend class basic_otp_mailbox_registry;
    template<typename _R> friend class std::function;
    // template<typename _R, typename _F> friend class std::_Function_handler;

private:
    boost::asio::io_service&            m_io_service;
    basic_otp_node<Alloc, Mutex>&       m_node;
    epid<Alloc>                         m_self;
    atom                                m_name;
    std::set<epid<Alloc> >              m_links;
    std::map<ref<Alloc>, epid<Alloc> >  m_monitors;
    boost::shared_ptr<queue_type>       m_queue;
    system_clock::time_point            m_time_freed;   // Cache time of this mbox

    void do_deliver(transport_msg<Alloc>* a_msg);

    void name(const atom& a_name) { m_name = a_name; }

public:
    basic_otp_mailbox(
            basic_otp_node<Alloc, Mutex>& a_node, const epid<Alloc>& a_self,
            const atom& a_name = atom(), boost::asio::io_service* a_svc = NULL,
            const Alloc& a_alloc = Alloc())
        : basic_otp_mailbox(a_node, a_self, a_name, 255, a_svc, a_alloc)
    {}

    basic_otp_mailbox(
            basic_otp_node<Alloc, Mutex>& a_node, const epid<Alloc>& a_self,
            const atom& a_name = atom(), int a_queue_size = 255,
            boost::asio::io_service* a_svc = NULL, const Alloc& a_alloc = Alloc())
        : m_io_service(a_svc ? *a_svc : a_node.io_service())
        , m_node(a_node), m_self(a_self)
        , m_name(a_name)
        , m_queue(new queue_type(m_io_service, a_queue_size, a_alloc))
    {}

    ~basic_otp_mailbox() { close(); }

    /// @param a_reg_remove when true the mailbox's pid is removed from registry.
    ///          Only pass false when invoking from the registry on destruction.
    void close(const eterm<Alloc>& a_reason = am_normal, bool a_reg_remove = true);

    basic_otp_node<Alloc, Mutex>&   node()          const { return m_node;       }
    /// Pid associated with this mailbox.
    const epid<Alloc>&              self()          const { return m_self;       }
    /// Registered name of this mailbox (if it was given a name).
    const atom&                     name()          const { return m_name;       }
    boost::asio::io_service&        io_service()    const { return m_io_service; }
    /// Queue of pending received messages.
    //queue_type&                     queue()               { return m_queue;      }
    /// Indicates if mailbox doesn't have any pending messages
    bool                            empty()         const { return m_queue.empty(); }

    /// Time when this mailbox was placed in the free list
    system_clock::time_point        time_freed()    const { return m_time_freed; }

    /// Register current mailbox under the given name
    bool reg(const atom& a_name) { return m_node.register_mailbox(a_name, *this); }

    bool operator== (const basic_otp_mailbox& rhs) const { return self() == rhs.self(); }
    bool operator!= (const basic_otp_mailbox& rhs) const { return self() != rhs.self(); }

    /// Clear mailbox's queue of awaiting messages
    void clear() { m_queue->reset(); }

    /// Print pid and regname of the mailbox to the given stream
    std::ostream& dump(std::ostream& out) const;

    /*
    /// Find the first message in the mailbox matching a pattern.
    /// Return the message, and if \a a_binding is not NULL set the binding variables.
    /// The call is not thread-safe and should be evaluated in the thread running the
    /// mailbox node's service.
    transport_msg<Alloc>*
    match(const eterm<Alloc>& a_pattern, varbind* a_binding = NULL);
    */

    /// Dequeue the next message from the mailbox.  The call is non-blocking and
    /// returns NULL if no messages are waiting.
    transport_msg<Alloc>* receive() {
        transport_msg<Alloc>* m;
        return m_queue->dequeue(m) ? m : nullptr;
    }

    /**
     * Call a handler on asynchronous delivery of message(s).
     *
     * The call is non-blocking. If returns Upon timeout
     * or delivery of a message to the mailbox the handler \a h will be
     * invoked.  The handler must have a signature with three arguments:
     * \code
     * void handler(basic_otp_mailbox<Alloc, Mutex>& a_mailbox,
     *              transport_msg<Alloc>*&           a_msg,
     *              ::boost::system::error_code&     a_err);
     * \endcode
     * In case of timeout the error will be set to non-zero value
     *
     * @param h is the handler to call upon arrival of a message
     * @param a_timeout is the timeout interval to wait for message (-1 = infinity)
     * @param a_repeat_count is the number of messages to wait (-1 = infinite)
     * @return true if the message was synchronously received
     * @throws std::runtime_error
     **/
    template <typename OnReceive>
    bool async_receive
    (
        const OnReceive& h,
        std::chrono::milliseconds a_timeout = std::chrono::milliseconds(-1),
        int a_repeat_count = 0
    );

    /**
     * Cancel pending asynchronous receive operation
     */
    void cancel_async_receive() {
        m_queue->cancel();
    }

    /**
     * Wait for messages and perform pattern match when a message arives
     *
     * @param a_matcher is the pattern matcher to run
     * @param a_on_timeout is the callback to be executed on timeout. It should
     *   have the following signature
     *   \code
     *      void on_timeout(basic_otp_mailbox<Alloc,Mutex>&);
     *   \endcode
     * @param a_repeat_count number of messages to wait (-1 = infinite)
     * @throws std::runtime_error
     */
    template <typename OnTimeout>
    bool async_match
    (
        const marshal::eterm_pattern_matcher<Alloc>& a_matcher,
        const OnTimeout& a_on_timeout,
        std::chrono::milliseconds a_timeout = std::chrono::milliseconds(-1),
        int a_repeat_count = 0
    );

    /// Deliver a message to this mailbox. The call is thread-safe.
    void deliver(const transport_msg<Alloc>& a_msg) {
        std::unique_ptr<transport_msg<Alloc>> p(new transport_msg<Alloc>(a_msg));
        m_queue->enqueue(p.get());
        p.release();
    }

    /// Deliver a message to this mailbox. The call is thread-safe.
    void deliver(transport_msg<Alloc>&& a_msg) {
        std::unique_ptr<transport_msg<Alloc>> p(new transport_msg<Alloc>(std::move(a_msg)));
        m_queue->enqueue(p.get());
        p.release();
    }

    /// Send a message \a a_msg to a pid \a a_to.
    void send(const epid<Alloc>& a_to, const eterm<Alloc>& a_msg) {
        m_node.send(self(), a_to, a_msg);
    }
    /// Send a message \a a_msg to the local process registered as \a a_to.
    void send(const atom& a_to, const eterm<Alloc>& a_msg) {
        m_node.send(self(), a_to, a_msg);
    }
    /// Send a message \a a_msg to the process registered as \a a_to on remote node \a a_node.
    void send(const atom& a_node, const atom& a_to, const eterm<Alloc>& a_msg) {
        m_node.send(self(), a_node, a_to, a_msg);
    }

    /**
     * Block until response for a RPC call arrives.
     * @return a pointer to ErlTerm containing the response
     * @exception EpiConnectionException if there was an connection error
     * @throws EpiBadRPC if the corresponding RPC was incorrect
     */
    //bool receive_rpc_reply(const eterm<Alloc>& a_reply);

    /**
     * Send an RPC request to a remote Erlang node.
     * @param node remote node where execute the funcion.
     * @param mod the name of the Erlang module containing the
     * function to be called.
     * @param fun the name of the function to call.
     * @param args a list of Erlang terms, to be used as arguments
     * to the function.
     * @throws EpiBadArgument if function, module or nodename are too big
     * @throws EpiInvalidTerm if any of the args is invalid
     * @throws EpiEncodeException if encoding fails
     * @throws EpiConnectionException if send fails
     */
    void send_rpc(const atom& a_node,
                  const atom& a_mod,
                  const atom& a_fun,
                  const list<Alloc>& args,
                  const epid<Alloc>* /*gleader*/ = NULL) {
        m_node.send_rpc(self(), a_node, a_mod, a_fun, args);
    }

    /// Send an RPC request to a remote Erlang node.
    void send_rpc(const atom&        a_node,
                  const std::string& a_mod,
                  const std::string& a_fun,
                  const list<Alloc>& args,
                  const epid<Alloc>* /*gleader*/ = NULL) {
        m_node.send_rpc(self(), a_node, atom(a_mod), atom(a_fun), args);
    }

    /// Send an RPC request to a remote Erlang node.
    void send_rpc(const atom&        a_node,
                  const char*        a_mod,
                  const char*        a_fun,
                  const list<Alloc>& args,
                  const epid<Alloc>* /*gleader*/ = NULL) {
        m_node.send_rpc(self(), a_node, atom(a_mod), atom(a_fun), args);
    }

    /**
     * Execute an equivalent of rpc:cast(...). Doesn't return any value.
     * @throws err_bad_argument
     * @throws err_no_process
     * @throws err_connection
     */
    void send_rpc_cast(const atom& a_node, const atom& a_mod,
            const atom& a_fun, const list<Alloc>& args,
            const epid<Alloc>* gleader = NULL)
    {
        m_node.send_rpc_cast(self(), a_node, a_mod, a_fun, args, gleader);
    }

    /**
     * Execute an equivalent of rpc:cast(...). Doesn't return any value.
     * @throws err_bad_argument
     * @throws err_no_process
     * @throws err_connection
     */
    void send_rpc_cast(const atom& a_node, const std::string& a_mod,
            const std::string& a_fun, const list<Alloc>& args,
            const epid<Alloc>* gleader = NULL)
    {
        m_node.send_rpc_cast(self(), a_node, atom(a_mod), atom(a_fun), args, gleader);
    }

    /**
     * Execute an equivalent of rpc:cast(...). Doesn't return any value.
     * @throws err_bad_argument
     * @throws err_no_process
     * @throws err_connection
     */
    void send_rpc_cast(const atom& a_node, const char* a_mod,
            const char* a_fun, const list<Alloc>& args,
            const epid<Alloc>* gleader = NULL)
    {
        m_node.send_rpc_cast(self(), a_node, atom(a_mod), atom(a_fun), args, gleader);
    }

    /// Send exit message to all linked pids and monitoring pids
    void break_links(const eterm<Alloc>& a_reason);

    /// Attempt to kill a remote process by sending it
    /// an exit message to a_pid, with reason \a a_reason
    void exit(const epid<Alloc>& a_pid, const eterm<Alloc>& a_reason) {
        m_node.send_exit(transport_msg<Alloc>::EXIT2, self(), a_pid, a_reason);
    }

    /// Link mailbox to the given pid.
    /// The given pid will receive an exit message when \a a_pid dies.
    /// @throws err_no_process
    /// @throws err_connection
    void link(const epid<Alloc>& a_to) {
        if (self() == a_to)
            return;
        if (m_links.find(a_to) != m_links.end())
            return;
        m_node.link(self(), a_to);
        m_links.insert(a_to);
    }

    /// UnLink the given pid
    void unlink(const epid<Alloc>& a_to) {
        if (m_links.erase(a_to) == 0)
            return;
        m_node.unlink(self(), a_to);
    }

    /// Set up a monitor of a remote \a a_target_pid.
    //const ref<Alloc>& monitor(const epid<Alloc>& a_target_pid) {
    void monitor(const epid<Alloc>& a_target_pid) {
        if (self() == a_target_pid)
            return;
        const ref<Alloc>& l_ref = m_node.send_monitor(self(), a_target_pid);
        m_monitors[l_ref] = a_target_pid;
    }

    /// Demonitor the pid monitored using \a a_ref reference.
    void demonitor(const ref<Alloc>& a_ref) {
        typename std::map<ref<Alloc>, epid<Alloc> >::iterator it = m_monitors.find(a_ref);
        if (it == m_monitors.end())
            return;
        m_node.send_demonitor(self(), it->second, a_ref);
        m_monitors.erase(it);
    }
};

} // namespace connect
} // namespace eixx

namespace std {

    template <typename Alloc, typename Mutex>
    ostream& operator<< (ostream& out,
        const eixx::connect::basic_otp_mailbox<Alloc,Mutex>& a_mbox)
    {
        return a_mbox.dump(out);
    }

} // namespace std

#include <eixx/connect/basic_otp_mailbox.hxx>

#endif // _EIXX_BASIC_OTP_MAILBOX_HPP_
