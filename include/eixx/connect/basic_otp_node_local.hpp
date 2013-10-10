//----------------------------------------------------------------------------
/// \file  basic_otp_node_local.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing local node functionality.
///        The purpose of this class is to provide the user with ability
///        to manage inter-node connectivity manually.  This functionality
///        has not been implemented.
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

#ifndef _EIXX_OTP_NODE_LOCAL_HPP_
#define _EIXX_OTP_NODE_LOCAL_HPP_

#include <string>
#include <eixx/marshal/atom.hpp>
#include <eixx/eterm_exception.hpp>

namespace EIXX_NAMESPACE {
namespace connect {

using marshal::atom;

/**
 * Contains information of a node: Host name, alive name,
 * node name (host+alive) and cookie.
 */
class basic_otp_node_local {
public:
    basic_otp_node_local() {}
    basic_otp_node_local(const std::string& a_nodename, const std::string& a_cookie = "")
        throw (std::runtime_error, err_bad_argument);

    virtual ~basic_otp_node_local() {}

    /// Change the nodename of current node.
    void set_nodename(const std::string& a_nodename, const std::string& a_cookie = "")
        throw (std::runtime_error, err_bad_argument);

    /// Get node name in the form <tt>node@host</tt>.
    atom                nodename()  const { return m_nodename; }

    /// Get node name in the form <tt>node@host.name.com</tt>.
    const std::string&  longname()  const { return m_longname; }

    /// Get name of the node without hostname
    const std::string&  alivename() const { return m_alivename; }

    /// Get host name
    const std::string&  hostname()  const { return m_hostname; }

    /// Get cookie
    atom                cookie()    const { return m_cookie; }

    /// Set the cookie
    void cookie(const std::string& a_cookie) { m_cookie = a_cookie; }

    /// Returns true if a_nodename is the same as current node.
    bool is_same_node(const atom& a_node) { return m_nodename == a_node; }

protected:
    atom        m_nodename;
    std::string m_longname;
    std::string m_alivename;
    std::string m_hostname;
    atom        m_cookie;

    static atom        s_default_cookie; // Default cookie
    static std::string s_localhost;      // localhost name
};

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_OTP_NODE_LOCAL_HPP_
