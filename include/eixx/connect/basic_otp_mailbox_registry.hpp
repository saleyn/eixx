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

#ifndef _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_HPP_
#define _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_HPP_

#include <eixx/marshal/eterm.hpp>
#include <eixx/connect/basic_otp_mailbox.hpp>
#include <eixx/util/hashtable.hpp>
#include <queue>

namespace eixx {
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
     * @throws err_bad_argument
     * @throws err_no_process
     */
    mailbox_ptr
    get(const eterm<Alloc>& a_proc) const;

    /**
     * Look up a mailbox based on its name. If the mailbox has gone out
     * of scope we also remove the reference from the hashtable so we
     * don't find it again.
     * @throws err_no_process
     */
    mailbox_ptr
    get(atom a_name) const;

    /**
     * Look up a mailbox based on its pid. If the mailbox has gone out
     * of scope we also remove the reference from the hashtable so we
     * don't find it again.
     * @throws err_no_process
     */
    mailbox_ptr
    get(const epid<Alloc>& a_pid) const;

    void names(std::list<atom>& list);

    void pids(std::list<epid<Alloc> >& list);

    /**
     * This method is not thread-safe - use for debugging!
     */
    size_t count() const { return m_by_pid.size(); }
};

} // namespace connect
} // namespace eixx

#include <eixx/connect/basic_otp_mailbox_registry.hxx>

#endif // _EIXX_BASIC_OTP_MAILBOX_REGISTRTY_HPP_
