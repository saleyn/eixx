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
#include "test_alloc.hpp"
#include <eixx/eixx.hpp>

using namespace eixx;

BOOST_AUTO_TEST_CASE( test_mailbox )
{
    boost::asio::io_service io;
    otp_node node(io, "a");
    atom am_io_server("io_server");
    atom am_main("main");

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
