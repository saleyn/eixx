//----------------------------------------------------------------------------
/// \file  atom.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an atom - enumerated string stored 
///        stored in non-garbage collected hash table of fixed size.
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
#ifndef _EIXX_AM_HPP_
#define _EIXX_AM_HPP_

#include <eixx/marshal/atom.hpp>

namespace EIXX_NAMESPACE {

    using marshal::atom;

    // Constant global atom values

	const atom am_ANY_              = atom("_");
	extern const atom am_badrpc;
	extern const atom am_call;
	extern const atom am_cast;
	extern const atom am_erlang;
	extern const atom am_error;
	extern const atom am_false;
	extern const atom am_format;
	extern const atom am_gen_cast;
	extern const atom am_io_lib;
	extern const atom am_latin1;
	extern const atom am_noconnection;
	extern const atom am_noproc;
	extern const atom am_normal;
	extern const atom am_ok;
	extern const atom am_request;
	extern const atom am_rex;
	extern const atom am_rpc;
	extern const atom am_true;
	extern const atom am_undefined;
	extern const atom am_unsupported;
	extern const atom am_user;

} // namespace EIXX_NAMESPACE

#endif // _EIXX_AM_HPP_
