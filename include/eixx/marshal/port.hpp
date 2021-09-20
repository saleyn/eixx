//----------------------------------------------------------------------------
/// \file  port.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an port object of Erlang external 
///        term format.
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

#ifndef _IMPL_PORT_HPP_
#define _IMPL_PORT_HPP_

#include <boost/static_assert.hpp>
#include <eixx/eterm_exception.hpp>
#include <eixx/marshal/atom.hpp>
#include <eixx/marshal/config.hpp>

namespace eixx {
namespace marshal {

/**
 * Representation of erlang Pids.
 * A port has 3 parameters, node name, id, and creation number
 *
 */
template <class Alloc>
class port {
    struct port_blob {
        uint32_t creation;
        uint64_t id;
        atom     node;

        port_blob(const atom& a_node, uint64_t a_id, uint32_t a_cre)
            : creation(a_cre), id(a_id), node(a_node)
        {}
    };

    blob<port_blob, Alloc>* m_blob;

    void release() {
        if (m_blob)
            m_blob->release();
    }

    // Must only be called from constructor!
    void init(const atom& node, uint64_t id, uint32_t creation, 
              const Alloc& alloc)
    {
        m_blob = new blob<port_blob, Alloc>(1, alloc);
        new (m_blob->data()) port_blob(node, id, creation);
    }

public:
    static const port<Alloc> null;

    /// When true - include 'Creation' in printing to string/stream
    static bool display_creation() { return config::display_creation(); }

    port() : m_blob(nullptr) {}

    /**
     * Create an Erlang port from its components.
     * If node string size is greater than MAX_NODE_LENGTH or = 0,
     * the constructor throws an exception.
     * @param node the nodename.
     * @param id an arbitrary number. Only the low order 18
     * bits will be used.
     * @param creation yet another arbitrary number. Only the low order
     * 2 bits will be used.
     * @throw err_bad_argument if node is empty or greater than MAX_NODE_LENGTH
     **/
    port(const char* node, const uint64_t id, const uint32_t creation, const Alloc& a_alloc = Alloc())
    {
        size_t n = strlen(node);
        detail::check_node_length(n);
        atom l_node(node, n);
        init(l_node, id, creation, a_alloc);
    }

    port(const atom& node, const uint64_t id, const uint32_t creation, const Alloc& a_alloc = Alloc())
    {
        detail::check_node_length(node.size());
        init(node, id, creation, a_alloc);
    }

    /**
     * Construct the object by decoding it from a binary
     * encoded buffer and using custom allocator.
     * @param buf is the buffer containing Erlang external binary format.
     * @param idx is the current offset in the buf buffer.
     * @param size is the size of the \a buf buffer.
     * @param a_alloc is the allocator to use.
     */
    port(const char *buf, uintptr_t& idx, size_t size, const Alloc& a_alloc = Alloc());

    port(const port& rhs) : m_blob(rhs.m_blob) { if (m_blob) m_blob->inc_rc(); }
    port(port&& rhs)      : m_blob(rhs.m_blob) { rhs.m_blob = nullptr; }

    ~port() { release(); }

    port& operator= (const port& rhs) {
        if (this != &rhs) {
            release(); m_blob = rhs.m_blob;
            if (m_blob) m_blob->inc_rc();
        }
        return *this;
    }

    port& operator= (port&& rhs) {
        if (this != &rhs) {
            release(); m_blob = rhs.m_blob; rhs.m_blob = nullptr;
        }
        return *this;
    }

    /**
     * Get the node name from the PORT.
     * @return the node name from the PORT.
     **/
    atom node() const { return m_blob ? m_blob->data()->node : atom::null(); }

    /**
     * Get the id number from the PORT.
     * @return the id number from the PORT.
     **/
    uint64_t id() const { return m_blob ? m_blob->data()->id : 0; }

    /**
     * Get the creation number from the PORT.
     * @return the creation number from the PORT.
     **/
    uint32_t creation() const { return m_blob ? m_blob->data()->creation : 0; }

    bool operator== (const port<Alloc>& t) const {
        return id() == t.id() && node() == t.node() && creation() == t.creation();
    }

    /// Less operator, needed for maps
    bool operator<(const port<Alloc>& rhs) const {
        int n = node().compare(rhs.node());
        if (n > 0)                       return true;
        if (n < 0)                       return false;
        if (id()   < rhs.id())           return true;
        if (id()   > rhs.id())           return false;
        if (creation() < rhs.creation()) return true;
        if (creation() > rhs.creation()) return false;
        return false;
    }

    size_t encode_size() const {
        return id() > 0x0fffffff ? 16 :
            #if defined(ERL_V4_PORT_EXT) || defined(ERL_NEW_PORT_EXT)
                12
            #else
                9
            #endif
            + node().size();
    }

    void encode(char* buf, uintptr_t& idx, size_t size) const;

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* =NULL) const {
        return out << *this;
    }
};

} // namespace marshal
} // namespace eixx

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, const eixx::marshal::port<Alloc>& a) {
        out << "#Port<" << a.node() << "." << a.id();
        if (a.creation() > 0 && a.display_creation())
            out << ',' << a.creation();
        return out << '>';
    }

} // namespace std

#include <eixx/marshal/port.hxx>

#endif // _IMPL_PORT_HPP_

