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

#ifndef _EIXX_OTP_NODE_LOCAL_HPP_
#define _EIXX_OTP_NODE_LOCAL_HPP_

#include <string>
#include <eixx/marshal/atom.hpp>
#include <eixx/eterm_exception.hpp>

namespace eixx {
namespace connect {

using marshal::atom;

/**
 * Contains information of a node: Host name, alive name,
 * node name (host+alive) and cookie.
 */
class basic_otp_node_local {
public:
    basic_otp_node_local() {}
    basic_otp_node_local(const std::string& a_nodename, const std::string& a_cookie = "");

    virtual ~basic_otp_node_local() {}

    /// Change the nodename of current node.
    void set_nodename(const std::string& a_nodename, const std::string& a_cookie = "");

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
} // namespace eixx

#endif // _EIXX_OTP_NODE_LOCAL_HPP_
