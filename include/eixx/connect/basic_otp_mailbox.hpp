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

#ifndef _EIXX_BASIC_OTP_MAILBOX_HPP_
#define _EIXX_BASIC_OTP_MAILBOX_HPP_

#include <boost/asio.hpp>
#include <eixx/marshal/eterm.hpp>
#include <eixx/connect/transport_msg.hpp>
#include <eixx/connect/verbose.hpp>
#include <eixx/util/async_wait_timeout.hpp>
#include <list>
#include <set>

namespace EIXX_NAMESPACE {
namespace connect {

template<typename Alloc, typename Mutex> class basic_otp_node;

using EIXX_NAMESPACE::marshal::list;
using EIXX_NAMESPACE::marshal::epid;

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
    typedef boost::shared_ptr<basic_otp_mailbox<Alloc, Mutex> > pointer;

    typedef boost::function<
        void (basic_otp_mailbox<Alloc, Mutex>&, boost::system::error_code&)
    > receive_handler_type;

    typedef std::list<transport_msg<Alloc>*> queue_type;

private:
    boost::asio::io_service&                m_io_service;
    basic_otp_node<Alloc, Mutex>&           m_node;
    epid<Alloc>                             m_self;
    atom                                    m_name;
    std::set<epid<Alloc> >                  m_links;
    std::map<ref<Alloc>, epid<Alloc> >      m_monitors;
    queue_type                              m_queue;
    boost::asio::deadline_timer_ex          m_deadline_timer;
    boost::function<
        void (receive_handler_type f,
              boost::system::error_code&)>  m_deadline_handler;

    void do_deliver(transport_msg<Alloc>* a_msg);

    void do_on_deadline_timer(receive_handler_type f, boost::system::error_code& ec);

public:
    basic_otp_mailbox(
            basic_otp_node<Alloc, Mutex>& a_node, const epid<Alloc>& a_self,
            const atom& a_name = atom(), boost::asio::io_service* a_svc = NULL)
        : m_io_service(a_svc ? *a_svc : a_node.io_service())
        , m_node(a_node), m_self(a_self)
        , m_name(a_name)
        , m_deadline_timer(m_io_service)
    {}

    ~basic_otp_mailbox() {
        close();
    }

    /// @param a_reg_remove when true the mailbox's pid is removed from registry.
    ///          Only pass false when invoking from the registry on destruction.
    void close(const eterm<Alloc>& a_reason = atom("normal"), bool a_reg_remove = true) {
        m_deadline_timer.cancel();
        if (a_reg_remove)
            m_node.close_mailbox(this);
        break_links(a_reason);
    }

    basic_otp_node<Alloc, Mutex>&   node()          const { return m_node;       }
    /// Pid associated with this mailbox.
    const epid<Alloc>&              self()          const { return m_self;       }
    /// Registered name of this mailbox (if it was given a name).
    const atom&                     name()          const { return m_name;       }
    boost::asio::io_service&        io_service()    const { return m_io_service; }
    /// Queue of pending received messages.
    queue_type&                     queue()               { return m_queue;      }

    void name(const atom& a_name) { m_name = a_name; }

    bool operator== (const basic_otp_mailbox& rhs) const { return self() == rhs.self(); }
    bool operator!= (const basic_otp_mailbox& rhs) const { return self() != rhs.self(); }

    std::ostream& dump(std::ostream& out) const {
        out << "#Mbox{pid=" << self();
        if (m_name != atom()) out << ", name=" << m_name;
        return out << '}';
    }

    /// Deliver a message to this mailbox
	void deliver(const transport_msg<Alloc>& a_msg) {
        transport_msg<Alloc>* l_msg = new transport_msg<Alloc>(a_msg);
        m_io_service.post(
            boost::bind(
                &basic_otp_mailbox<Alloc, Mutex>::do_deliver, this, l_msg));
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
     * Get a message from this mailbox. The call is non-blocking. Upon timeout
     * or delivery of a message to the mailbox the handler \a h will be
     * invoked.  The handler must have a signature with two arguments:
     * \verbatim
     * void handler(transport_msg& a_msg, boost::system::error_code& ec);
     * \endverbatim
     * In case of timeout the error will be set to: <tt>asio::error::timeout</tt>
     * that is defined in <tt>eixx/util/async_wait_timeout.hpp</tt>.
     **/
    void async_receive(receive_handler_type h, long msec_timeout = -1)
        throw (std::runtime_error);

    /**
     * Get a message from mailbox that matches the given pattern.
     * It will block until an apropiate message arrives.
     * @param pattern ErlTerm with pattern to check
     * @param binding VariableBinding to use. It can be 0. Default = 0
     * @return an pointer to the ErlTerm representing
     * the body of the next message waiting in this mailbox.
     * @exception EpiConnectionException if there was an connection error
     */
    //bool receive(const eterm<Alloc>& a_pattern, varbind<Alloc>* a_binding = NULL);

    /**
     * Block until response for a RPC call arrives.
     * @return a pointer to ErlTerm containing the response
     * @exception EpiConnectionException if there was an connection error
	 * @throw EpiBadRPC if the corresponding RPC was incorrect
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
                  const list<Alloc>& args) {
        m_node.send_rpc(a_node, a_mod, a_fun, args);
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
    void link(const epid<Alloc>& a_to) throw (err_no_process, err_connection) {
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
    const ref<Alloc>& monitor(const epid<Alloc>& a_target_pid) {
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

//------------------------------------------------------------------------------
// basic_otp_mailbox implementation
//------------------------------------------------------------------------------

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::break_links(const eterm<Alloc>& a_reason)
{
    for (typename std::set<epid<Alloc> >::const_iterator
            it=m_links.begin(), end = m_links.end(); it != end; ++it)
        try { m_node.send_exit(self(), *it, a_reason); } catch(...) {}
    for (typename std::map<ref<Alloc>, epid<Alloc> >::const_iterator
            it = m_monitors.begin(), end = m_monitors.end(); it != end; ++it)
        try { m_node.send_monitor_exit(self(), it->second, it->first, a_reason); } catch(...) {}
    if (!m_links.empty())    m_links.clear();
    if (!m_monitors.empty()) m_monitors.clear();
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::
do_on_deadline_timer(receive_handler_type f, boost::system::error_code& ec)
{
    m_deadline_timer.expires_at(boost::asio::deadline_timer_ex::time_type());

    f(*this, ec); // In case of timeout ec would contain boost::asio::error::timeout
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::async_receive(receive_handler_type h, long msec_timeout)
    throw (std::runtime_error)
{
    m_deadline_timer.cancel();

    // expires_at() == boost::posix_time::not_a_date_time
    /*
    if (m_deadline_timer.expires_at() != boost::asio::deadline_timer_ex::time_type())
        throw eterm_exception(
            "Another receive() is already scheduled for mailbox", self());
    */

    if (msec_timeout < 0)
        m_deadline_timer.async_wait(
            boost::bind(
                &basic_otp_mailbox<Alloc,Mutex>::do_on_deadline_timer, 
                this, h, boost::asio::placeholders::error));
    else
        m_deadline_timer.async_wait_timeout(
            boost::bind(
                &basic_otp_mailbox<Alloc,Mutex>::do_on_deadline_timer, 
                this, h, boost::asio::placeholders::error),
            msec_timeout);
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::do_deliver(transport_msg<Alloc>* a_msg)
{
    try {
        switch (a_msg->type()) {
            case transport_msg<Alloc>::LINK:
                BOOST_ASSERT(a_msg->to_pid() == self());
                m_links.insert(a_msg->from_pid());
                delete a_msg;
                return;

            case transport_msg<Alloc>::UNLINK:
                BOOST_ASSERT(a_msg->to_pid() == self());
                m_links.erase(a_msg->from_pid());
                delete a_msg;
                return;

            case transport_msg<Alloc>::MONITOR_P:
                BOOST_ASSERT((a_msg->to().type() == PID && a_msg->to_pid() == self())
                           || a_msg->to().to_atom() == m_name);
                m_monitors.insert(
                    std::pair<ref<Alloc>, epid<Alloc> >(a_msg->get_ref(), a_msg->from_pid()));
                delete a_msg;
                return;

            case transport_msg<Alloc>::DEMONITOR_P:
                m_monitors.erase(a_msg->get_ref());
                delete a_msg;
                return;

            case transport_msg<Alloc>::MONITOR_P_EXIT:
                m_monitors.erase(a_msg->get_ref());
                m_queue.push_back(a_msg);
                break;

            case transport_msg<Alloc>::EXIT2:
            case transport_msg<Alloc>::EXIT2_TT:
                BOOST_ASSERT(a_msg->to_pid() == self());
                m_links.erase(a_msg->from_pid());
                m_queue.push_back(a_msg);
                break;

            default:
                m_queue.push_back(a_msg);
        }
    } catch (std::exception& e) {
        a_msg->set_error_flag();
        m_queue.push_back(a_msg);
    }

    // If the timer's expiration is set to some non-default value, it means that 
    // there's an outstanding asynchronous receive operation.  We cancel the timer
    // that will cause invocation of the handler passed to the deadline timer 
    // upon executing mailbox->async_receive(Handler, Timeout).
    if (m_deadline_timer.expires_at() != boost::asio::deadline_timer_ex::time_type())
        m_deadline_timer.cancel();
}

} // namespace connect
} // namespace EIXX_NAMESPACE

namespace std {

    template <typename Alloc, typename Mutex>
    ostream& operator<< (ostream& out, 
        const EIXX_NAMESPACE::connect::basic_otp_mailbox<Alloc,Mutex>& a_mbox)
    {
        return a_mbox.dump(out);
    }

} // namespace std

#endif // _EIXX_BASIC_OTP_MAILBOX_HPP_
