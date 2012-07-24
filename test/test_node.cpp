//----------------------------------------------------------------------------
/// \file  test_mailbox.cpp
//----------------------------------------------------------------------------
/// \brief Test cases for basic_otp_mailbox.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-23
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the EPI (Erlang Plus Interface) Library.

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

#include <boost/test/unit_test.hpp>
#include <eixx/alloc_std.hpp>
#include <eixx/eixx.hpp>

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_node )
{
    std::string l_hostname("gbox-car-00"), l_resolved;
    boost::asio::io_service svc;
    boost::asio::ip::tcp::resolver resolver(svc);
    boost::asio::ip::tcp::resolver::query query(l_hostname, "");
    boost::asio::ip::tcp::resolver::iterator it;
    bool l_same = false;
    for (it = resolver.resolve(query); it != boost::asio::ip::tcp::resolver::iterator(); ++it) {
        std::string l_host_name = it->host_name();
        if (l_host_name.find('.') != std::string::npos) {
            l_resolved = l_host_name;
            break;
        }
        if (!l_same && l_host_name == it->host_name())
            l_same = true;
    }

    BOOST_REQUIRE(l_same);
    BOOST_REQUIRE_EQUAL(std::string(), l_resolved);
}
