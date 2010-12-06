//----------------------------------------------------------------------------
/// \file  connect.hpp
//----------------------------------------------------------------------------
/// \brief Header file for including connectivity part of eixx.
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-20
//----------------------------------------------------------------------------
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
#ifndef _EIXX_CONNECT_HPP_
#define _EIXX_CONNECT_HPP_

#include <eixx/connect/transport_msg.hpp>
#include <eixx/connect/basic_otp_mailbox.hpp>
#include <eixx/connect/basic_otp_connection.hpp>
#include <eixx/connect/basic_otp_node.hpp>

namespace EIXX_NAMESPACE {

typedef connect::transport_msg<allocator_t>                                 transport_msg;
typedef connect::basic_otp_connection<allocator_t, detail::recursive_mutex> otp_connection;
typedef connect::basic_otp_mailbox<allocator_t,    detail::recursive_mutex> otp_mailbox;
typedef connect::basic_otp_node<allocator_t,       detail::recursive_mutex> otp_node;

} // namespace EIXX_NAMESPACE

#endif
