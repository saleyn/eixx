//----------------------------------------------------------------------------
/// \file  basic_otp_mailbox_registry.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing mailbox registration functionality.
///        The registry allows resolving mailboxes by pids and names.
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

#ifndef _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_HPP_
#define _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_HPP_

#include <eixx/marshal/eterm.hpp>
#include <eixx/connect/basic_otp_mailbox.hpp>
#include <eixx/util/hashtable.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

using detail::lock_guard;

template <typename Alloc, typename Mutex>
class basic_otp_mailbox_registry {
    basic_otp_node<Alloc, Mutex>& m_owner_node;
    mutable Mutex m_lock;

    typedef basic_otp_mailbox<Alloc, Mutex> mailbox_type;
    typedef mailbox_type*                   mailbox_ptr;

    // These are made mutable so that intrinsic cleanup is possible
    // of orphant entries.
    mutable std::map<atom, mailbox_ptr>        m_by_name;
    mutable std::map<epid<Alloc>, mailbox_ptr> m_by_pid;
public:
    basic_otp_mailbox_registry(basic_otp_node<Alloc, Mutex>& a_owner) : m_owner_node(a_owner) {}

    ~basic_otp_mailbox_registry() {
        clear();
    }

    mailbox_ptr create_mailbox(
        const atom& a_name = atom(), boost::asio::io_service* a_svc=NULL);

    void clear();

    bool add(const atom& a_name, mailbox_ptr a_mbox);

    /// Unregister a name so that no mailbox is any longer associated with \a a_name.
    bool unregister(const atom& a_name);

    /// Remove \a a_mbox mailbox from the registry
    void erase(mailbox_ptr a_mbox);

    /**
     * Look up a mailbox based on its name or pid.
     */
    mailbox_ptr
    get(const eterm<Alloc>& a_proc) const throw (err_bad_argument, err_no_process);

    /**
     * Look up a mailbox based on its name. If the mailbox has gone out
     * of scope we also remove the reference from the hashtable so we
     * don't find it again.
     */
    mailbox_ptr
    get(const atom& a_name) const throw(err_no_process);

    /**
     * Look up a mailbox based on its pid. If the mailbox has gone out
     * of scope we also remove the reference from the hashtable so we
     * don't find it again.
     */
    mailbox_ptr
    get(const epid<Alloc>& a_pid) const throw(err_no_process);

    void names(std::list<atom>& list);

    void pids(std::list<epid<Alloc> >& list);

    /**
     * This method is not thread-safe - use for debugging!
     */
    size_t count() const { return m_by_pid.size(); }
};

//------------------------------------------------------------------------------
// otp_mailbox_registry implementation
//------------------------------------------------------------------------------


template <typename Alloc, typename Mutex>
typename basic_otp_mailbox_registry<Alloc, Mutex>::mailbox_ptr
basic_otp_mailbox_registry<Alloc, Mutex>::create_mailbox(
    const atom& a_name, boost::asio::io_service* a_svc)
{
    lock_guard<Mutex> guard(m_lock);
    if (!a_name.empty()) {
        typename std::map<atom, mailbox_ptr>::iterator it = m_by_name.find(a_name);
        if (it != m_by_name.end())
            return it->second;   // Already registered!
    }

    epid<Alloc> l_pid = m_owner_node.create_pid();
    mailbox_ptr mbox = new mailbox_type(m_owner_node, l_pid, a_name, a_svc);
    if (!a_name.empty())
        m_by_name.insert(std::pair<atom, mailbox_ptr>(a_name, mbox));
    m_by_pid.insert(std::pair<epid<Alloc>, mailbox_ptr>(l_pid, mbox));
    return mbox;
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox_registry<Alloc, Mutex>::clear()
{
    if (!m_by_name.empty() || !m_by_pid.empty()) {
        static const atom s_am_normal("normal");
        lock_guard<Mutex> guard(m_lock);
        m_by_name.clear();
        typename std::map<epid<Alloc>, mailbox_ptr>::iterator it;
        for(it = m_by_pid.begin(); it != m_by_pid.end(); ++it) {
            mailbox_ptr p = it->second;
            p->close(s_am_normal, false);
        }
        m_by_pid.clear();
    }
}

template <typename Alloc, typename Mutex>
bool basic_otp_mailbox_registry<Alloc, Mutex>::add(const atom& a_name, mailbox_ptr a_mbox)
{
    if (a_name.empty())
        throw err_bad_argument("Empty registering name!");
    if (!a_mbox->name().empty())
        throw err_bad_argument("Mailbox already registered as", a_mbox->name());
    lock_guard<Mutex> guard(m_lock);
    if (m_by_name.find(a_name) != m_by_name.end())
        return false;
    m_by_name.insert(std::pair<atom, mailbox_ptr>(a_name, a_mbox));
    a_mbox->name(a_name);
    return true;
}

/// Unregister a name so that no mailbox is any longer associated with \a a_name.
template <typename Alloc, typename Mutex>
bool basic_otp_mailbox_registry<Alloc, Mutex>::unregister(const atom& a_name)
{
    if (!a_name.empty())
        return false;
    lock_guard<Mutex> guard(m_lock);
    typename std::map<atom, mailbox_ptr>::iterator it = m_by_name.find(a_name);
    if (it == m_by_name.end())
        return;
    it->second.name("");
    m_by_name.erase(it);
}

/// Remove \a a_mbox mailbox from the registry
template <typename Alloc, typename Mutex>
void basic_otp_mailbox_registry<Alloc, Mutex>::erase(mailbox_ptr a_mbox)
{
    if (!a_mbox)
        return;
    lock_guard<Mutex> guard(m_lock);
    m_by_pid.erase(a_mbox->self());
    if (!a_mbox->name().empty())
        m_by_name.erase(a_mbox->name());
    a_mbox->name("");
}

/**
 * Look up a mailbox based on its name or pid.
 */
template <typename Alloc, typename Mutex>
typename basic_otp_mailbox_registry<Alloc, Mutex>::mailbox_ptr
basic_otp_mailbox_registry<Alloc, Mutex>::get(const eterm<Alloc>& a_proc) const
    throw (err_bad_argument, err_no_process)
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
basic_otp_mailbox_registry<Alloc, Mutex>::get(const atom& a_name) const
    throw(err_no_process)
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
    lock_guard<Mutex> guard(m_lock);
    for(typename std::map<atom, mailbox_ptr>::const_iterator
        it = m_by_name.begin(), end = m_by_name.end(); it != end; ++it)
        list.push_back(it->first);
}

template <typename Alloc, typename Mutex>
void basic_otp_mailbox_registry<Alloc, Mutex>::pids(std::list<epid<Alloc> >& list)
{
    lock_guard<Mutex> guard(m_lock);
    for(typename std::map<epid<Alloc>, mailbox_ptr>::const_iterator
        it = m_by_pid.begin(), end = m_by_pid.eend(); it != end; ++it)
        list.push_back(it->first);
}

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_HPP_
