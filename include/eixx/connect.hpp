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
#ifndef _EIXX_CONNECT_HPP_
#define _EIXX_CONNECT_HPP_

#include <eixx/connect/transport_msg.hpp>
#include <eixx/connect/basic_otp_mailbox.hpp>
#include <eixx/connect/basic_otp_connection.hpp>
#include <eixx/connect/basic_otp_node.hpp>

namespace eixx {

typedef connect::transport_msg<allocator_t>                                 transport_msg;
typedef connect::basic_otp_connection<allocator_t, detail::recursive_mutex> otp_connection;
typedef connect::basic_otp_mailbox<allocator_t,    detail::recursive_mutex> otp_mailbox;
typedef connect::basic_otp_node<allocator_t,       detail::recursive_mutex> otp_node;

} // namespace eixx

#endif
