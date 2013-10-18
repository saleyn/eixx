//----------------------------------------------------------------------------
/// \file  basic_otp_mailbox_registry.ipp
//----------------------------------------------------------------------------
/// \brief Implemention details of mailbox registration functionality
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

#ifndef _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_IPP_
#define _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_IPP_
#include <chrono>

namespace EIXX_NAMESPACE {
namespace connect {

//-----------------------------------------------------------------------------
// Static members instantiation
//-----------------------------------------------------------------------------
template <typename Alloc, typename Mutex>
std::queue<typename basic_otp_mailbox_registry<Alloc,Mutex>::mailbox_ptr>
basic_otp_mailbox_registry<Alloc, Mutex>::s_free_list;

template <typename Alloc, typename Mutex>
Mutex
basic_otp_mailbox_registry<Alloc, Mutex>::s_free_list_lock;

//-----------------------------------------------------------------------------
// Member functions implementation
//-----------------------------------------------------------------------------

template <typename Alloc, typename Mutex>
typename basic_otp_mailbox_registry<Alloc, Mutex>::mailbox_ptr
basic_otp_mailbox_registry<Alloc, Mutex>::
create_mailbox(const atom& a_name, boost::asio::io_service* a_svc)
{
    static const std::chrono::seconds s_min_retention(180);

    lock_guard<Mutex> guard(m_lock);
    if (!a_name.empty()) {
        typename std::map<atom, mailbox_ptr>::iterator it = m_by_name.find(a_name);
        if (it != m_by_name.end())
            return it->second;   // Already registered!
    }

    mailbox_ptr p = nullptr;

    // Mailboxes are cached, so that they can be reused at some later point
    // to prevent overflowing the max number of available pid numbers
    {
        lock_guard<Mutex> g(s_free_list_lock);
        if (!s_free_list.empty() &&
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now() -
                s_free_list.back()->time_freed()) > s_min_retention)
        {
            p = s_free_list.back();
            s_free_list.pop();
            p->name(a_name);
        }
    }

    if (p == nullptr) {
        epid<Alloc> l_pid = m_owner_node.create_pid();
        p = new mailbox_type(m_owner_node, l_pid, a_name, a_svc);
    }

    if (!a_name.empty())
        m_by_name.insert(std::pair<atom, mailbox_ptr>(a_name, p));
    m_by_pid.insert(std::pair<epid<Alloc>, mailbox_ptr>(p->self(), p));
    return p;
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox_registry<Alloc, Mutex>::
clear()
{
    if (!m_by_name.empty() || !m_by_pid.empty()) {
        lock_guard<Mutex> guard(m_lock);
        m_by_name.clear();
        typename std::map<epid<Alloc>, mailbox_ptr>::iterator it;
        for(it = m_by_pid.begin(); it != m_by_pid.end(); ++it) {
            mailbox_ptr p = it->second;
            p->close(am_normal, false);
        }
        m_by_pid.clear();
    }
}

template <typename Alloc, typename Mutex>
bool basic_otp_mailbox_registry<Alloc, Mutex>::
add(atom a_name, mailbox_ptr a_mbox)
{
    if (a_name.empty())
        throw err_bad_argument("Empty registering name!");
    if (!a_mbox->name().empty())
        throw err_bad_argument("Mailbox already registered as", a_mbox->name());
    lock_guard<Mutex> guard(m_lock);
    auto it = m_by_name.insert(std::pair<atom, mailbox_ptr>(a_name, a_mbox));
    if (it.second)
        a_mbox->name(a_name);
    return it.second;
}

/// Unregister a name so that no mailbox is any longer associated with \a a_name.
template <typename Alloc, typename Mutex>
bool basic_otp_mailbox_registry<Alloc, Mutex>::
erase(const atom& a_name)
{
    if (!a_name.empty())
        return false;
    lock_guard<Mutex> guard(m_lock);
    typename std::map<atom, mailbox_ptr>::iterator it = m_by_name.find(a_name);
    if (it == m_by_name.end())
        return false;
    it->second.name(atom());
    m_by_name.erase(it);
    return true;
}

/// Remove \a a_mbox mailbox from the registry
template <typename Alloc, typename Mutex>
void basic_otp_mailbox_registry<Alloc, Mutex>::
erase(mailbox_ptr a_mbox)
{
    if (!a_mbox)
        return;
    lock_guard<Mutex> guard(m_lock);
    m_by_pid.erase(a_mbox->self());
    if (!a_mbox->name().empty())
        m_by_name.erase(a_mbox->name());
    a_mbox->name(atom());
}

/**
 * Look up a mailbox based on its name or pid.
 */
template <typename Alloc, typename Mutex>
typename basic_otp_mailbox_registry<Alloc, Mutex>::mailbox_ptr
basic_otp_mailbox_registry<Alloc, Mutex>::
get(const eterm<Alloc>& a_proc) const throw (err_bad_argument, err_no_process)
{
    switch (a_proc.type()) {
        case ATOM:  return get(a_proc.to_atom());
        case PID:   return get(a_proc.to_pid());
        default:    throw err_bad_argument("Unknown process identifier", a_proc);
    }
}

/**
 * Look up a mailbox based on its name. If the mailbox has gone out
 * of scope we also remove the reference from the hashtable so we
 * don't find it again.
 */
template <typename Alloc, typename Mutex>
typename basic_otp_mailbox_registry<Alloc, Mutex>::mailbox_ptr
basic_otp_mailbox_registry<Alloc, Mutex>::
get(atom a_name) const throw(err_no_process)
{
    lock_guard<Mutex> guard(m_lock);
    typename std::map<atom, mailbox_ptr>::iterator it = m_by_name.find(a_name);
    if (it != m_by_name.end())
        return it->second;
    throw err_no_process("Process not registered", a_name);
}

/**
 * Look up a mailbox based on its pid. If the mailbox has gone out
 * of scope we also remove the reference from the hashtable so we
 * don't find it again.
 */
template <typename Alloc, typename Mutex>
typename basic_otp_mailbox_registry<Alloc, Mutex>::mailbox_ptr
basic_otp_mailbox_registry<Alloc, Mutex>::get(const epid<Alloc>& a_pid) const
    throw(err_no_process)
{
    lock_guard<Mutex> guard(m_lock);
    typename std::map<epid<Alloc>, mailbox_ptr>::iterator it = m_by_pid.find(a_pid);
    if (it != m_by_pid.end())
        return it->second;
    throw err_no_process("Process not found", a_pid);
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox_registry<Alloc, Mutex>::names(std::list<atom>& list)
{
    list.clear();
    lock_guard<Mutex> guard(m_lock);
    list.resize(m_by_name.size());
    for(typename std::map<atom, mailbox_ptr>::const_iterator
        it = m_by_name.begin(), end = m_by_name.end(); it != end; ++it)
        list.push_back(it->first);
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox_registry<Alloc, Mutex>::pids(std::list<epid<Alloc> >& list)
{
    list.clear();
    lock_guard<Mutex> guard(m_lock);
    list.resize(m_by_pid.size());
    for(typename std::map<epid<Alloc>, mailbox_ptr>::const_iterator
        it = m_by_pid.begin(), end = m_by_pid.eend(); it != end; ++it)
        list.push_back(it->first);
}

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_IPP_
