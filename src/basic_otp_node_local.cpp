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

#include <fstream>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/filesystem/path.hpp>
#include <eixx/connect/basic_otp_node_local.hpp>
#include <ei.h>

namespace eixx {
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
} // namespace eixx
