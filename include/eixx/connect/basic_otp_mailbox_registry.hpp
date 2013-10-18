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
#include <queue>

namespace EIXX_NAMESPACE {
namespace connect {

using detail::lock_guard;

template <typename Alloc, typename Mutex>
struct basic_otp_mailbox_registry {
    typedef basic_otp_mailbox<Alloc, Mutex>     mailbox_type;
    typedef mailbox_type*                       mailbox_ptr;

private:
    basic_otp_node<Alloc, Mutex>&               m_owner_node;
    // These are made mutable so that intrinsic cleanup is possible
    // of orphant entries.
    mutable Mutex                               m_lock;
    mutable std::map<atom, mailbox_ptr>         m_by_name;
    mutable std::map<epid<Alloc>, mailbox_ptr>  m_by_pid;

    // Cache of freed mailboxes
    static std::queue<mailbox_ptr>              s_free_list;
    static Mutex                                s_free_list_lock;

public:
    basic_otp_mailbox_registry(basic_otp_node<Alloc, Mutex>& a_owner)
        : m_owner_node(a_owner)
    {}

    ~basic_otp_mailbox_registry() {
        clear();
    }

    mailbox_ptr create_mailbox(
        const atom& a_name = atom(), boost::asio::io_service* a_svc=NULL);

    void clear();

    bool add(atom a_name, mailbox_ptr a_mbox);

    /// Unregister a name so that no mailbox is any longer associated with \a a_name.
    bool erase(const atom& a_name);

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
    get(atom a_name) const throw(err_no_process);

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

} // namespace connect
} // namespace EIXX_NAMESPACE

#include <eixx/connect/basic_otp_mailbox_registry.ipp>

#endif // _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_HPP_
