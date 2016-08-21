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

#include <boost/test/unit_test.hpp>
#include "test_alloc.hpp"
#include <eixx/eixx.hpp>

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_mailbox )
{
    boost::asio::io_service io;
    otp_node node(io, "a");
    const atom am_io_server("io_server");
    const atom am_main("main");

    //std::cerr << "mailbox count " << node.registry().count() << std::endl;
    {
        otp_mailbox::pointer a(node.create_mailbox(am_io_server));
        otp_mailbox::pointer b(node.create_mailbox(am_main));
        //BOOST_REQUIRE_NE(*a.get(), *b.get());

        otp_mailbox* ag = node.get_mailbox(atom("io_server"));
        otp_mailbox* bg = node.get_mailbox(atom("main"));

        {
            BOOST_REQUIRE_NE(ag, static_cast<otp_mailbox*>(NULL));
            BOOST_REQUIRE_NE(bg, static_cast<otp_mailbox*>(NULL));

            BOOST_REQUIRE_EQUAL(*a, *ag);
            BOOST_REQUIRE_EQUAL(*b, *bg);
        }

        //node.registry().erase(a.get());
        //node.registry().erase(b.get());
    }
    //std::cerr << "mailbox count " << node.registry().count() << std::endl;
}
