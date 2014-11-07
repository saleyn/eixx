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

#ifndef _IMPL_PORT_HPP_
#define _IMPL_PORT_HPP_

#include <boost/static_assert.hpp>
#include <eixx/eterm_exception.hpp>
#include <eixx/marshal/atom.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {

/**
 * Representation of erlang Pids.
 * A port has 3 parameters, node name, id, and creation number
 *
 */
template <class Alloc>
class port {
    struct port_blob {
        uint8_t creation;
        int     id;
        atom    node;

        port_blob(const atom& a_node, int a_id, uint8_t a_cre)
            : creation(a_cre), id(a_id), node(a_node)
        {}
    };

    blob<port_blob, Alloc>* m_blob;

    void release() {
        if (m_blob)
            m_blob->release();
    }

    // Must only be called from constructor!
    void init(const atom& node, int id, uint8_t creation, 
              const Alloc& alloc) throw(err_bad_argument) 
    {
        m_blob = new blob<port_blob, Alloc>(1, alloc);
        new (m_blob->data()) port_blob(node, id & 0x0fffffff, creation & 0x03);
    }

public:
    static const port<Alloc> null;

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
    port(const char* node, const int id, const int creation, const Alloc& a_alloc = Alloc())
        throw (err_bad_argument)
    {
        size_t n = strlen(node);
        detail::check_node_length(n);
        atom l_node(node, n);
        init(l_node, id, creation, a_alloc);
    }

    port(const atom& node, const int id, const int creation, const Alloc& a_alloc = Alloc())
        throw (err_bad_argument)
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
    port(const char *buf, int& idx, size_t size, const Alloc& a_alloc = Alloc())
        throw (err_decode_exception);

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
    int id() const { return m_blob ? m_blob->data()->id : 0; }

    /**
     * Get the creation number from the PORT.
     * @return the creation number from the PORT.
     **/
    int creation() const { return m_blob ? m_blob->data()->creation : 0; }

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

    size_t encode_size() const { return 9 + node().size(); }

    void encode(char* buf, int& idx, size_t size) const;

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding=NULL) const {
        return out << *this;
    }
};

} // namespace marshal
} // namespace EIXX_NAMESPACE

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::port<Alloc>& a) {
        return out << "#Port<" << a.node() << "." << a.id() << ">";
    }

} // namespace std

#include <eixx/marshal/port.ipp>

#endif // _IMPL_PORT_HPP_

