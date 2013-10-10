/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) Library.

Copyright (C) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#include <fstream>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <eixx/connect/basic_otp_node_local.hpp>
#include <ei.h>

namespace EIXX_NAMESPACE {
namespace connect {

namespace {
    atom get_default_cookie() {
        const char* home = getenv("HOME") ? getenv("HOME") : "";
        if (home) {
            std::stringstream s;
            s   << home
                #if BOOST_VERSION >= 104600
                << boost::filesystem::path("/").make_preferred().native()
                #else
                << boost::filesystem2::slash<std::string>::value
                #endif
                << ".erlang.cookie";
            std::ifstream file(s.str().c_str());
            if (! file.fail()) {
                std::string cookie;
                file >> cookie;
                size_t n = cookie.find('\n');
                if (n != std::string::npos) cookie.erase(n);
                if (cookie.size() > EI_MAX_COOKIE_SIZE)
                    throw err_bad_argument("Cookie size too long", cookie.size());
                return atom(cookie);
            }
        }
        return atom();
    }

} // namespace

atom        basic_otp_node_local::s_default_cookie = get_default_cookie();
std::string basic_otp_node_local::s_localhost      = boost::asio::ip::host_name();

basic_otp_node_local::basic_otp_node_local(
    const std::string& a_nodename, const std::string& a_cookie)
    throw (std::runtime_error, err_bad_argument)
{
    set_nodename(a_nodename, a_cookie);
}

void basic_otp_node_local::set_nodename(
    const std::string& a_nodename, const std::string& a_cookie)
    throw (std::runtime_error, err_bad_argument)
{
    if (m_cookie.size() > EI_MAX_COOKIE_SIZE)
        throw err_bad_argument("Cookie size too long", m_cookie.size());

    m_cookie = a_cookie.empty() ? s_default_cookie : atom(a_cookie);

    std::string::size_type pos = a_nodename.find('@');

    if (pos == std::string::npos) {
        m_alivename = a_nodename;
        m_hostname  = s_localhost;
    } else {
        m_alivename = a_nodename.substr(0, pos);
        m_hostname  = a_nodename.substr(pos+1, a_nodename.size());
    }

    std::string short_hostname;
    pos = m_hostname.find('.');

    std::stringstream str;
    if (pos != std::string::npos) {
        str << m_alivename << '@' << m_hostname.substr(0, pos);
        m_longname = m_alivename+'@'+m_hostname;
    } else {
        str << m_alivename << '@' << m_hostname;
        try {
            boost::asio::io_service svc;
            boost::asio::ip::tcp::resolver resolver(svc);
            boost::asio::ip::tcp::resolver::query query(m_hostname, "");
            boost::asio::ip::tcp::resolver::iterator it;

            for (it = resolver.resolve(query); it != boost::asio::ip::tcp::resolver::iterator(); ++it) {
                std::string l_host_name = it->host_name();
                if (l_host_name.find('.') != std::string::npos) {
                    m_hostname = l_host_name;
                    break;
                }
            }
            m_longname = m_alivename+'@'+m_hostname;
        } catch (std::exception& e) {
            throw err_bad_argument(std::string("Can't resolve ") + m_hostname + ": " + e.what());
        }
    }

    m_nodename = marshal::make_node_name(str.str());
}

} // namespace connect
} // namespace EIXX_NAMESPACE
