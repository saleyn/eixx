//----------------------------------------------------------------------------
/// \file  transport_msg.hpp
//----------------------------------------------------------------------------
/// \brief Erlang distributed transport message type
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-12
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

#ifndef _EIXX_TRANSPORT_MSG_HPP_
#define _EIXX_TRANSPORT_MSG_HPP_

#include <eixx/marshal/eterm.hpp>
#include <eixx/util/common.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace connect {

using EIXX_NAMESPACE::marshal::tuple;
using EIXX_NAMESPACE::marshal::list;
using EIXX_NAMESPACE::marshal::eterm;
using EIXX_NAMESPACE::marshal::epid;
using EIXX_NAMESPACE::marshal::ref;
using EIXX_NAMESPACE::marshal::trace;

/// Erlang distributed transport messages contain message type,
/// control message with message routing and other details, and 
/// optional user message for send and reg_send message types.
template <typename Alloc>
class transport_msg {
public:
    enum transport_msg_type {
          UNDEFINED         = 0
        , LINK              = 1 << ERL_LINK
        , SEND              = 1 << ERL_SEND
        , EXIT              = 1 << ERL_EXIT
        , UNLINK            = 1 << ERL_UNLINK
        , NODE_LINK         = 1 << ERL_NODE_LINK
        , REG_SEND          = 1 << ERL_REG_SEND
        , GROUP_LEADER      = 1 << ERL_GROUP_LEADER
        , EXIT2             = 1 << ERL_EXIT2
        , SEND_TT           = 1 << ERL_SEND_TT
        , EXIT_TT           = 1 << ERL_EXIT_TT
        , REG_SEND_TT       = 1 << ERL_REG_SEND_TT
        , EXIT2_TT          = 1 << ERL_EXIT2_TT
        , MONITOR_P         = 1 << ERL_MONITOR_P
        , DEMONITOR_P       = 1 << ERL_DEMONITOR_P
        , MONITOR_P_EXIT    = 1 << ERL_MONITOR_P_EXIT
        //---------------------------
        , EXCEPTION         = 1 << 31   // Non-Erlang defined code representing 
                                        // message handling exception
        , NO_EXCEPTION_MASK = (uint32_t)EXCEPTION-1
    };

private:
    // Note that the m_type is mutable so that we can call set_error_flag() on
    // constant objects.
    mutable transport_msg_type  m_type;
    tuple<Alloc>                m_cntrl;
    eterm<Alloc>                m_msg;

public:
    transport_msg() : m_type(UNDEFINED) {}

    transport_msg(int a_msgtype, const tuple<Alloc>& a_cntrl, const eterm<Alloc>* a_msg = NULL)
        : m_type(1 << a_msgtype), m_cntrl(a_cntrl)
    {
        if (a_msg)
            new (&m_msg) eterm<Alloc>(*a_msg);
    }

    transport_msg(const transport_msg& rhs)
        : m_type(rhs.m_type), m_cntrl(rhs.m_cntrl), m_msg(rhs.m_msg)
    {}

    transport_msg(transport_msg&& rhs)
        : m_type(rhs.m_type), m_cntrl(std::move(rhs.m_cntrl)), m_msg(std::move(rhs.m_msg))
    {
        rhs.m_type = UNDEFINED;
    }

    /// Return a string representation of the transport message type.
    const char* type_string() const;

    /// Transport message type
    transport_msg_type  type()      const { return m_type; }
    int                 to_type()   const { return bit_scan_forward(m_type); }
    const tuple<Alloc>& cntrl()     const { return m_cntrl;}
    const eterm<Alloc>& msg()       const { return m_msg;  }
    /// Returns true when the transport message contains message payload
    /// associated with SEND or REG_SEND message type.
    bool                has_msg()   const { return m_msg.type() != EIXX_NAMESPACE::UNDEFINED; }

    /// Indicates that there was an error processing this message
    bool  has_error()               const { return (m_type & EXCEPTION) == EXCEPTION; }

    /// Set an error flag indicating a problem processing this message
    void  set_error_flag() const {
        m_type = static_cast<transport_msg_type>(m_type | EXCEPTION);
    }

    /// Return the term representing the message sender. The sender is
    /// usually a pid, except for MONITOR_P_EXIT message type for which
    /// the sender can be either pid or atom name.
    const eterm<Alloc>& sender() const {
        switch (m_type) {
            case REG_SEND:
            case LINK:
            case UNLINK:
            case EXIT:
            case EXIT2:
            case GROUP_LEADER:
            case REG_SEND_TT:
            case EXIT_TT:
            case EXIT2_TT:
            case MONITOR_P:
            case DEMONITOR_P:
            case MONITOR_P_EXIT:
                return m_cntrl[1];
            default:
                throw err_wrong_type(m_type, "transport_msg.from()");
        }
    }

    /// This function may only raise exception for MONITOR_P_EXIT
    /// message types if the message sender is given by name rather than by pid.
    const epid<Alloc>& sender_pid() const throw (err_wrong_type) {
        return sender().to_pid();
    }

    /// Return the term representing the message sender. The sender is
    /// usually a pid, except for MONITOR_P|DEMONITOR_P message type for which 
    /// the sender can be either pid or atom name.
    const eterm<Alloc>& recipient() const {
        switch (m_type) {
            case REG_SEND:
                return m_cntrl[3];
            case REG_SEND_TT:
                return m_cntrl[4];
            case SEND:
            case LINK:
            case UNLINK:
            case EXIT:
            case EXIT2:
            case GROUP_LEADER:
            case SEND_TT:
            case EXIT_TT:
            case EXIT2_TT:
            case MONITOR_P:
            case DEMONITOR_P:
            case MONITOR_P_EXIT:
                return m_cntrl[2];
            default:
                throw err_wrong_type(m_type, "transport_msg.to()");
        }
    }

    /// This function may only raise exception for MONITOR_P|DEMONITOR_P
    /// message types if the message sender is given by name rather than by pid.
    const epid<Alloc>& recipient_pid() const throw (err_wrong_type) {
        return recipient().to_pid();
    }

    const atom& recipient_name() const throw (err_wrong_type) {
        return recipient().to_atom();
    }

    const eterm<Alloc>& trace_token() const throw (err_wrong_type) {
        switch (m_type) {
            case SEND_TT:
            case EXIT_TT:
            case EXIT2_TT:      return m_cntrl[3];
            case REG_SEND_TT:   return m_cntrl[4];
            default:
                throw err_wrong_type(m_type, "SEND_TT|EXIT_TT|EXIT2_TT|REG_SEND_TT");
        }
    }

    const ref<Alloc>& get_ref() const throw (err_wrong_type) {
        switch (m_type) {
            case MONITOR_P:
            case DEMONITOR_P:
            case MONITOR_P_EXIT:
                return m_cntrl[3].to_ref();
            default:
                throw err_wrong_type(m_type, "MONITOR_P|DEMONITOR_P|MONITOR_P_EXIT");
        }
    }

    const eterm<Alloc>& reason() const throw (err_wrong_type) {
        switch (m_type) {
            case EXIT:
            case EXIT2:         return m_cntrl[3];
            case EXIT_TT:
            case EXIT2_TT:
            case MONITOR_P_EXIT:return m_cntrl[4];
            default:
                throw err_wrong_type(m_type, "EXIT|EXIT2|EXIT_TT|EXIT2_TT|MONITOR_P_EXIT");
        }
    }

    /// Initialize the object with given components.
    void set(int a_msgtype, const tuple<Alloc>& a_cntrl, const eterm<Alloc>* a_msg = NULL) {
        m_type = static_cast<transport_msg_type>(1 << a_msgtype);
        m_cntrl = a_cntrl;
        if (a_msg)
            m_msg = *a_msg;
        else
            m_msg.clear();
    }

    /// Set the current message to represent a SEND message containing \a a_msg to
    /// be sent to \a a_to pid.
    void set_send(const epid<Alloc>& a_to, const eterm<Alloc>& a_msg,
                  const Alloc& a_alloc = Alloc())
    {
        const trace<Alloc>* token = trace<Alloc>::tracer(marshal::TRACE_GET);
        if (unlikely(token)) {
            const tuple<Alloc>& l_cntrl =
                tuple<Alloc>::make(ERL_SEND_TT, atom(), a_to, *token, a_alloc);
            set(ERL_SEND_TT, l_cntrl, &a_msg);
        } else {
            const tuple<Alloc>& l_cntrl =
                tuple<Alloc>::make(ERL_SEND, atom(), a_to, a_alloc);
            set(ERL_SEND, l_cntrl, &a_msg);
        }
    }

    /// Set the current message to represent a REG_SEND message containing
    /// \a a_msg to be sent from \a a_from pid to \a a_to registered mailbox.
    void set_reg_send(const epid<Alloc>& a_from, const atom& a_to,
                      const eterm<Alloc>& a_msg, const Alloc& a_alloc = Alloc())
    {
        const trace<Alloc>* token = trace<Alloc>::tracer(marshal::TRACE_GET);
        if (unlikely(token)) {
            const tuple<Alloc>& l_cntrl =
                tuple<Alloc>::make(ERL_REG_SEND_TT, a_from, atom(), a_to, *token, a_alloc);
            set(ERL_REG_SEND_TT, l_cntrl, &a_msg);
        } else {
            const tuple<Alloc>& l_cntrl =
                tuple<Alloc>::make(ERL_REG_SEND, a_from, atom(), a_to, a_alloc);
            set(ERL_REG_SEND, l_cntrl, &a_msg);
        }
    }

    /// Set the current message to represent a LINK message.
    void set_link(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
                  const Alloc& a_alloc = Alloc())
    {
        const tuple<Alloc>& l_cntrl =
            tuple<Alloc>::make(ERL_LINK, a_from, a_to, a_alloc);
        set(LINK, l_cntrl, NULL);
    }

    /// Set the current message to represent an UNLINK message.
    void set_unlink(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const Alloc& a_alloc = Alloc())
    {
        const tuple<Alloc>& l_cntrl =
            tuple<Alloc>::make(ERL_UNLINK, a_from, a_to, a_alloc);
        set(UNLINK, l_cntrl, NULL);
    }

    /// Set the current message to represent an EXIT message with \a a_reason
    /// sent from \a a_from pid to \a a_to pid.
    void set_exit(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const eterm<Alloc>& a_reason, const Alloc& a_alloc = Alloc())
    {
        set_exit_internal(ERL_EXIT, ERL_EXIT_TT, a_from, a_to, a_reason, a_alloc);
    }

    /// Set the current message to represent an EXIT2 message.
    void set_exit2(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const eterm<Alloc>& a_reason, const Alloc& a_alloc = Alloc())
    {
        set_exit_internal(ERL_EXIT2, ERL_EXIT2_TT, a_from, a_to, a_reason, a_alloc);
    }

    /// Set the current message to represent a MONITOR message.
    void set_monitor(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const ref<Alloc>& a_ref, const Alloc& a_alloc = Alloc())
    {
        set_monitor_internal(ERL_MONITOR_P, a_from, a_to, a_ref, a_alloc);
    }

    /// Set the current message to represent a MONITOR message.
    void set_monitor(const epid<Alloc>& a_from, const atom& a_to,
        const ref<Alloc>& a_ref, const Alloc& a_alloc = Alloc())
    {
        set_monitor_internal(ERL_MONITOR_P, a_from, a_to, a_ref, a_alloc);
    }

    /// Set the current message to represent a DEMONITOR message.
    void set_demonitor(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const ref<Alloc>& a_ref, const Alloc& a_alloc = Alloc())
    {
        set_monitor_internal(ERL_DEMONITOR_P, a_from, a_to, a_ref, a_alloc);
    }

    /// Set the current message to represent a DEMONITOR message.
    void set_demonitor(const epid<Alloc>& a_from, const atom& a_to,
        const ref<Alloc>& a_ref, const Alloc& a_alloc = Alloc())
    {
        set_monitor_internal(ERL_DEMONITOR_P, a_from, a_to, a_ref, a_alloc);
    }

    /// Set the current message to represent a MONITOR_EXIT message.
    void set_monitor_exit(const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const ref<Alloc>& a_ref, const eterm<Alloc>& a_reason,
        const Alloc& a_alloc = Alloc())
    {
        const tuple<Alloc>& l_cntrl = tuple<Alloc>::make(
            ERL_MONITOR_P_EXIT, a_from, a_to, a_ref, a_reason, a_alloc);
        set(ERL_MONITOR_P_EXIT, l_cntrl, NULL);
    }

    /// Set the current message to represent a MONITOR_EXIT message.
    void set_monitor_exit(const atom& a_from, const epid<Alloc>& a_to, 
        const ref<Alloc>& a_ref, const eterm<Alloc>& a_reason,
        const Alloc& a_alloc = Alloc())
    {
        const tuple<Alloc>& l_cntrl = tuple<Alloc>::make(
            ERL_MONITOR_P_EXIT, a_from, a_to, a_ref, a_reason, a_alloc);
        set(ERL_MONITOR_P_EXIT, l_cntrl, NULL);
    }

    /// Send RPC call request.
    /// Result will be delivered to the mailbox of the \a a_from pid.
    /// The RPC consists of two parts, send and receive.
    /// Here is the send part: <tt>{ PidFrom, { call, Mod, Fun, Args, user }}</tt>
    void set_send_rpc(const epid<Alloc>& a_from,
        const atom& mod, const atom& fun, const list<Alloc>& args,
        const epid<Alloc>* a_gleader = NULL, const Alloc& a_alloc = Alloc())
    {
        static atom s_cmd("call");
        static eterm<Alloc> s_user(atom("user"));
        eterm<Alloc> gleader(a_gleader == NULL ? s_user : eterm<Alloc>(*a_gleader));
        const tuple<Alloc>& inner_tuple =
            tuple<Alloc>::make(a_from,
                tuple<Alloc>::make(s_cmd, mod, fun, args, gleader, a_alloc),
                a_alloc);

        set_send_rpc_internal(a_from, inner_tuple, a_alloc);
    }

    /// Send RPC cast request.
    /// The result of this RPC cast is discarded.
    /// Here is the send part:
    ///   <tt>{PidFrom, {'$gen_cast', {cast, Mod, Fun, Args, user}}}</tt>
    void set_send_rpc_cast(const epid<Alloc>& a_from,
        const atom& mod, const atom& fun, const list<Alloc>& args,
        const epid<Alloc>* a_gleader = NULL, const Alloc& a_alloc = Alloc())
    {
        static atom s_gen_cast("$gen_cast");
        static atom s_cmd("cast");
        static eterm<Alloc> s_user(atom("user"));
        eterm<Alloc> gleader(a_gleader == NULL ? s_user : eterm<Alloc>(*a_gleader));

        const tuple<Alloc>& inner_tuple =
            tuple<Alloc>::make(s_gen_cast,
                tuple<Alloc>::make(s_cmd, mod, fun, args, gleader, a_alloc), a_alloc);

        set_send_rpc_internal(a_from, inner_tuple, a_alloc);
    }

    std::ostream& dump(std::ostream& out) const {
        out << "#DistMsg{" << (has_error() ? "has_error, " : "")
            << "type=" << type_string() << ", cntrl=" << cntrl();
        if (has_msg())
            out << ", msg=" << msg().to_string();
        return out << '}';
    }

    std::string to_string() const {
        std::stringstream s; dump(s); return s.str();
    }

private:
    void set_exit_internal(int a_type, int a_trace_type,
        const epid<Alloc>& a_from, const epid<Alloc>& a_to,
        const eterm<Alloc>& a_reason, const Alloc& a_alloc = Alloc())
    {
        const trace<Alloc>* token = trace<Alloc>::tracer(marshal::TRACE_GET);
        if (unlikely(token)) {
            const tuple<Alloc>& l_cntrl =
                tuple<Alloc>::make(a_trace_type, a_from, a_to, *token, a_reason, a_alloc);
            set(a_trace_type, l_cntrl, NULL);
        } else {
            const tuple<Alloc>& l_cntrl =
                tuple<Alloc>::make(a_type, a_from, a_to, a_reason, a_alloc);
            set(a_type, l_cntrl, NULL);
        }
    }

    template <typename FromProc, typename ToProc>
    void set_monitor_internal(int a_type,
        FromProc a_from, ToProc a_to, const ref<Alloc>& a_ref,
        const Alloc& a_alloc = Alloc())
    {
        const tuple<Alloc>& l_cntrl = tuple<Alloc>::make(a_type, a_from, a_to, a_ref, a_alloc);
        set(a_type, l_cntrl, NULL);
    }

    void set_send_rpc_internal(const epid<Alloc>& a_from, const tuple<Alloc>& a_cmd, 
        const Alloc& a_alloc = Alloc())
    {
        static atom rex("rex");
        set_reg_send(a_from, rex, eterm<Alloc>(a_cmd), a_alloc);
    }

};

template <typename Alloc>
const char* transport_msg<Alloc>::type_string() const {
    switch (m_type & NO_EXCEPTION_MASK) {
        case UNDEFINED:         return "UNDEFINED";
        case LINK:              return "LINK";
        case SEND:              return "SEND";
        case EXIT:              return "EXIT";
        case UNLINK:            return "UNLINK";
        case NODE_LINK:         return "NODE_LINK";
        case REG_SEND:          return "REG_SEND";
        case GROUP_LEADER:      return "GROUP_LEADER";
        case EXIT2:             return "EXIT2";
        case SEND_TT:           return "SEND_TT";
        case EXIT_TT:           return "EXIT_TT";
        case REG_SEND_TT:       return "REG_SEND_TT";
        case EXIT2_TT:          return "EXIT2_TT";
        case MONITOR_P:         return "MONITOR_P";
        case DEMONITOR_P:       return "DEMONITOR_P";
        case MONITOR_P_EXIT:    return "MONITOR_P_EXIT";
        default: {
            std::stringstream str; str << "UNSUPPORTED(" << bit_scan_forward(m_type) << ')';
            return str.str().c_str();
        }
    }
}

} // namespace connect
} // namespace EIXX_NAMESPACE

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, EIXX_NAMESPACE::connect::transport_msg<Alloc>& a_msg) {
        return a_msg.dump(out);
    }
} // namespace std

#endif // _EIXX_TRANSPORT_MSG_HPP_
