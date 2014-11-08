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

#include <eixx/marshal/am.hpp>
#include <boost/concept_check.hpp>

namespace EIXX_NAMESPACE {

    const atom am_badrpc            = atom("badrpc");
    const atom am_call              = atom("call");
    const atom am_cast              = atom("cast");
    const atom am_erlang            = atom("erlang");
    const atom am_error             = atom("error");
    const atom am_false             = atom("false");
    const atom am_format            = atom("format");
    const atom am_gen_cast          = atom("$gen_cast");
    const atom am_io_lib            = atom("io_lib");
    const atom am_latin1            = atom("latin1");
    const atom am_noconnection      = atom("noconnection");
    const atom am_noproc            = atom("noproc");
    const atom am_normal            = atom("normal");
    const atom am_ok                = atom("ok");
    const atom am_request           = atom("request");
    const atom am_rex               = atom("rex");
    const atom am_rpc               = atom("rpc");
    const atom am_true              = atom("true");
    const atom am_undefined         = atom("undefined");
    const atom am_unsupported       = atom("unsupported");
    const atom am_user              = atom("user");

} // namespace EIXX_NAMESPACE

