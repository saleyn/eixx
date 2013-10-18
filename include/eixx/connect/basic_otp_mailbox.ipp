//----------------------------------------------------------------------------
/// \file  basic_otp_mailbox.ipp
//----------------------------------------------------------------------------
/// \brief Implemention of basic mailbox functionality.
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

#ifndef _EIXX_BASIC_OTP_MAILBOX_IPP_
#define _EIXX_BASIC_OTP_MAILBOX_IPP_

namespace EIXX_NAMESPACE {
namespace connect {

//------------------------------------------------------------------------------
// basic_otp_mailbox implementation
//------------------------------------------------------------------------------

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::
close(const eterm<Alloc>& a_reason, bool a_reg_remove) {
    m_time_freed = std::chrono::system_clock::now();
    m_queue->reset();
    if (a_reg_remove)
        m_node.close_mailbox(this);
    break_links(a_reason);
    m_name = atom();
}

template <typename Alloc, typename Mutex>
bool basic_otp_mailbox<Alloc, Mutex>::
async_receive(receive_handler_type h, std::chrono::milliseconds a_timeout,
              int a_repeat_count) throw (std::runtime_error)
{
    return m_queue->async_dequeue(
        [this, h](transport_msg<Alloc>*& a_msg, const boost::system::error_code& ec) {
            if (this->m_time_freed.time_since_epoch().count() == 0 || !h)
                return false;
            bool res;
            if (ec) {
                transport_msg<Alloc>* p(nullptr);
                res = h(*this, p);
            } else {
                res = h(*this, a_msg);
                if (a_msg) {
                    delete a_msg;
                    a_msg = nullptr;
                }
            }

            return res;
        },
        a_timeout,
        a_repeat_count);
}

template <typename Alloc, typename Mutex>
bool basic_otp_mailbox<Alloc, Mutex>::
async_match(const marshal::eterm_pattern_matcher<Alloc>& a_matcher,
            const std::function<void (basic_otp_mailbox<Alloc, Mutex>&)>& a_on_timeout,
            std::chrono::milliseconds a_timeout,
            int a_repeat_count) throw (std::runtime_error)
{
    auto f =
        [this, &a_matcher, a_on_timeout]
        (transport_msg<Alloc>*& a_msg, const boost::system::error_code& ec) {
            if (this->m_time_freed.time_since_epoch().count() == 0)
                return false;
            bool res;
            if (ec) {
                a_on_timeout(*this);
                return false;
            }
            varbind<Alloc> binding;
            if (a_msg) {
                a_matcher.match(a_msg->msg(), &binding);
                delete a_msg;
                a_msg = nullptr;
            }
            return true;
        };

    return m_queue->async_dequeue(f, a_timeout, a_repeat_count);
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::
break_links(const eterm<Alloc>& a_reason)
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

/*
template <typename Alloc, typename Mutex>
transport_msg<Alloc>* basic_otp_mailbox<Alloc, Mutex>::
match(const eterm<Alloc>& a_pattern, varbind* a_binding)
{
    for (typename queue_type::iterator it = m_queue->begin(), e = m_queue->end();
            it != e; ++it)
    {
        transport_msg<Alloc>* p = *it;
        BOOST_ASSERT(p);
        if (a_pattern.match(p->msg(), a_binding)) {
            // Found a match
            m_queue.erase(it);
            return p;
        }
    }
    return NULL;
}
*/

template <typename Alloc, typename Mutex>
void basic_otp_mailbox<Alloc, Mutex>::
do_deliver(transport_msg<Alloc>* a_msg)
{
    try {
        switch (a_msg->type()) {
            case transport_msg<Alloc>::LINK:
                BOOST_ASSERT(a_msg->recipient_pid() == self());
                m_links.insert(a_msg->sender_pid());
                delete a_msg;
                return;

            case transport_msg<Alloc>::UNLINK:
                BOOST_ASSERT(a_msg->recipient_pid() == self());
                m_links.erase(a_msg->sender_pid());
                delete a_msg;
                return;

            case transport_msg<Alloc>::MONITOR_P:
                BOOST_ASSERT((a_msg->recipient().type() == PID && a_msg->recipient_pid() == self())
                           || a_msg->recipient().to_atom() == m_name);
                m_monitors.insert(
                    std::pair<ref<Alloc>, epid<Alloc> >(a_msg->get_ref(), a_msg->sender_pid()));
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
                BOOST_ASSERT(a_msg->recipient_pid() == self());
                m_links.erase(a_msg->sender_pid());
                m_queue.push_back(a_msg);
                break;

            default:
                m_queue.push_back(a_msg);
        }
    } catch (std::exception& e) {
        a_msg->set_error_flag();
        m_queue.push_back(a_msg);
    }
}

template <typename Alloc, typename Mutex>
std::ostream& basic_otp_mailbox<Alloc, Mutex>::
dump(std::ostream& out) const {
    out << "#Mbox{pid=" << self();
    if (m_name != atom()) out << ", name=" << m_name;
    return out << '}';
}

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_BASIC_OTP_MAILBOX_IPP_
