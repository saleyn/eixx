//----------------------------------------------------------------------------
/// \file  basic_rpc_server.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing RPC protocol support for an Erlang node
//----------------------------------------------------------------------------
// Copyright (c) 2013 Serge Aleynikov <saleyn@gmail.com>
// Created: 2013-10-21
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) library.

Copyright (c) 2013 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _EIXX_BASIC_RPC_SERVER_HPP_
#define _EIXX_BASIC_RPC_SERVER_HPP_

#include <string>
#include <eixx/marshal/atom.hpp>
#include <eixx/marshal/eterm_match.hpp>
#include <eixx/eterm_exception.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

using marshal::atom;

/**
 * Implements Erlang RPC protocol
 */
template <typename Alloc, typename Mutex>
class basic_otp_node<Alloc, Mutex>::rpc_server : private Alloc
{
    static const var A;
    static const var F;
    static const var G;
    static const var M;
    static const var P;
    static const var R;
    static const var T;

    basic_otp_node&                                     m_node;
    bool                                                m_active;
    std::unique_ptr<basic_otp_mailbox<Alloc, Mutex>>    m_self;

    void rpc_call(const epid<Alloc>& a_from, const ref<Alloc>& a_ref,
        const atom& a_mod, const atom& a_fun, const list<Alloc>& a_list,
        const eterm<Alloc>& a_gleader)
    {
        if (m_node.on_rpc_call)
            m_node.on_rpc_call(a_from, a_ref, a_mod, a_fun, a_list, a_gleader);
    }

public:
    rpc_server(basic_otp_node& a_owner, const Alloc& a_alloc = Alloc())
        : Alloc(a_alloc)
        , m_node(a_owner)
        , m_active(true)
        , m_self(a_owner.create_mailbox(am_rex))
    {}

    ~rpc_server() {
        close();
    }

    void close() {
        m_active = false;
        m_self.reset();
    }

    /// Encode a tuple containing RPC call details
    static tuple<Alloc> encode_rpc(const epid<Alloc>& a_from,
                            const atom& a_mod, const atom& a_fun,
                            const list<Alloc>& a_args,
                            const eterm<Alloc>& a_gleader)
    {
        // {Self, {call, Mod, Fun, Args, GroupLeader}}
        return tuple<Alloc>::make(a_from,
                    tuple<Alloc>::make(am_call, a_mod, a_fun, a_args, a_gleader));
    }

    /// Encode a tuple containing RPC cast details
    static tuple<Alloc> encode_rpc_cast(const epid<Alloc>& a_from,
                            const atom& a_mod, const atom& a_fun,
                            const list<Alloc>& a_args,
                            const eterm<Alloc>& a_gleader)
    {
        // {'$gen_cast', { cast, Mod, Fun, Args, GroupLeader}}
        return tuple<Alloc>::make(am_gen_cast,
                    tuple<Alloc>::make(am_cast, a_mod, a_fun, a_args, a_gleader));
    }

    static eterm<Alloc> decode_rpc(const eterm<Alloc>& a_msg) {
        static const eterm<Alloc> s_pattern = eterm<Alloc>::format("{rex, ~v}", var(T));

        if (a_msg.type() != TUPLE)
            return eterm<Alloc>();
        varbind<Alloc> binding;
        return a_msg.match(s_pattern, &binding) ? binding[T] : eterm<Alloc>();
    }

    bool operator() (const eterm<Alloc>& a_pat,
                     const varbind<Alloc>& a_binding,
                     long  a_opaque)
    {
        // TODO: What do we do on the incoming RPC call? Need to define
        // afacility to add RPC callable functions
        return false;
    }

    void start() {
        static const marshal::eterm_pattern_matcher<Alloc>& s_matcher =
        {
            marshal::eterm_pattern_action<Alloc>(
                this->get_allocator(),
                std::bind(&basic_otp_node<Alloc,Mutex>::rpc_call, &m_node,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                0,
                "{'$gen_call', {~w, ~w}, {call, ~w, ~w, ~w, ~w}}",
                                 P,  R,          M,  F,  A,  G),

            marshal::eterm_pattern_action<Alloc>(
                this->get_allocator(),
                std::bind(&basic_otp_node<Alloc,Mutex>::rpc_call, &m_node,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                1,
                "{'$gen_cast', {cast, ~w, ~w, ~w, ~w}}",
                                       M,  F,  A,  G),

            marshal::eterm_pattern_action<Alloc>(
                this->get_allocator(),
                std::bind(&basic_otp_node<Alloc,Mutex>::rpc_call, &m_node,
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                2,
                "{_, {~w, ~w}, _Cmd}}",
                       P,  R)
        };

        m_self->async_match(s_matcher, *this, std::chrono::milliseconds(-1), -1);
    }
};

template <typename Alloc, typename Mutex>
const var basic_otp_node<Alloc, Mutex>::rpc_server::A = var("A", LIST);
template <typename Alloc, typename Mutex>
const var basic_otp_node<Alloc, Mutex>::rpc_server::F = var("F", ATOM);
template <typename Alloc, typename Mutex>
const var basic_otp_node<Alloc, Mutex>::rpc_server::G = var("G");
template <typename Alloc, typename Mutex>
const var basic_otp_node<Alloc, Mutex>::rpc_server::M = var("M", ATOM);
template <typename Alloc, typename Mutex>
const var basic_otp_node<Alloc, Mutex>::rpc_server::P = var("P", PID);
template <typename Alloc, typename Mutex>
const var basic_otp_node<Alloc, Mutex>::rpc_server::R = var("R", REF);
template <typename Alloc, typename Mutex>
const var basic_otp_node<Alloc, Mutex>::rpc_server::T = var("T");


} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_BASIC_RPC_SERVER_HPP_
