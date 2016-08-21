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
#pragma once

#include <eixx/marshal/atom.hpp>

namespace eixx {

    using marshal::atom;

    // Constant global atom values

    const atom am_ANY_ = atom("_");
    extern const atom am_badarg;
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

} // namespace eixx
