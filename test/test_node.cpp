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

#include <eixx/util/async_wait_timeout.hpp>
#include <boost/test/unit_test.hpp>
#include "test_alloc.hpp"
#include <eixx/eixx.hpp>

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_node )
{
    std::string l_hostname("localhost"), l_resolved;
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
