//----------------------------------------------------------------------------
/// \file  ref.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an ref object of Erlang external 
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

#ifndef _IMPL_REF_HPP_
#define _IMPL_REF_HPP_

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <eixx/marshal/atom.hpp>
#include <eixx/eterm_exception.hpp>

namespace EIXX_NAMESPACE {
namespace marshal {
namespace detail {
} // namespace detail

/**
 * Representation of erlang Pids.
 * A ref has 5 parameters, nodeName, three ids, and creation number
 */
template <class Alloc>
class ref {
    enum { COUNT = 3 };

    struct ref_blob {
        atom node;
        union {
            uint32_t ids[COUNT+1];
            struct {
                uint32_t id0;
                uint64_t id1;
                uint32_t creation;
            } __attribute__((__packed__)) s;
        } u;

        ref_blob(const atom& a_node, uint32_t a_id0, uint64_t a_id1, uint8_t a_cre)
            : node(a_node)
        {
            u.s.id0      = a_id0 & 0x3ffff;
            u.s.id1      = a_id1;
            u.s.creation = a_cre & 0x3;
        }
    };

    blob<ref_blob, Alloc>* m_blob;

    // Must only be called from constructor!
    void init(const atom& a_node, uint32_t a_id0, uint64_t a_id1, uint8_t a_cre,
              const Alloc& alloc) throw(err_bad_argument)
    {
        m_blob = new blob<ref_blob, Alloc>(1, alloc);
        new (m_blob->data()) ref_blob(a_node, a_id0, a_id1, a_cre);
    }

    void release() {
        if (m_blob)
            m_blob->release();
    }

    uint32_t id0() const { return m_blob->data()->u.s.id0; }
    uint64_t id1() const { return m_blob->data()->u.s.id1; }

public:
    inline static const uint32_t* s_ref_ids() {
       static const uint32_t s_ref_ids[] = {0, 0, 0};
       return s_ref_ids;
    }

    static const ref<Alloc> null;

    ref() : m_blob(nullptr) {}

    /**
     * Create an Erlang ref from its components.
     * If node string size is greater than MAX_NODE_LENGTH or = 0,
     * the ErlAtom object is created but invalid.
     * @param node the nodename.
     * @param ids an array of arbitrary numbers. Only the low order 18
     * bits of the first number will be used.
     * @param creation yet another arbitrary number. Only the low order
     * 2 bits will be used.
     * @throw err_bad_argument if node is empty or greater than MAX_NODE_LENGTH
     */
    template <int N>
    ref(const char* node, uint32_t (&ids)[N], unsigned int creation,
        const Alloc& a_alloc = Alloc()) throw(err_bad_argument)
        : ref(atom(node), ids[0], ids[1], ids[2], creation, a_alloc)
    {}

    template <int N>
    ref(const atom& node, uint32_t (&ids)[N], unsigned int creation,
        const Alloc& a_alloc = Alloc()) throw(err_bad_argument)
        : ref(node, ids[0], ids[1], ids[2], creation, a_alloc)
    {
        BOOST_STATIC_ASSERT(N == 3);
    }

    ref(const atom& node, uint32_t id0, uint32_t id1, uint32_t id2, unsigned int creation,
        const Alloc& a_alloc = Alloc()) throw(err_bad_argument)
        : ref(node, id0, id1 | ((uint64_t)id2 << 32), creation, a_alloc)
    {}

    // For internal use
    ref(const atom& node, uint32_t id0, uint64_t id1, uint8_t creation,
        const Alloc& a_alloc = Alloc()) throw(err_bad_argument)
    {
        detail::check_node_length(node.size());
        init(node, id0, id1, creation, a_alloc);
    }

    /**
     * Construct the object by decoding it from a binary
     * encoded buffer.
     * @param buf is the buffer containing Erlang external binary format.
     * @param idx is the current offset in the buf buffer.
     * @param size is the size of the \a buf buffer.
     * @param a_alloc is the allocator to use.
     */
    ref(const char* buf, int& idx, size_t size, const Alloc& a_alloc = Alloc())
        throw(err_decode_exception);

    ref(const ref& rhs) : m_blob(rhs.m_blob) { if (m_blob) m_blob->inc_rc(); }
    ref(ref&& rhs) : m_blob(rhs.m_blob) { rhs.m_blob = nullptr; }

    ~ref() { release(); }

    ref& operator= (const ref& rhs) {
        if (this != &rhs) {
            release(); m_blob = rhs.m_blob;
            if (m_blob) m_blob->inc_rc();
        }
        return *this;
    }

    ref& operator= (ref&& rhs) {
        if (this != &rhs) {
            release(); m_blob = rhs.m_blob; rhs.m_blob = nullptr;
        }
        return *this;
    }

    /**
     * Get the node name from the REF.
     * @return the node name from the REF.
     */
    atom node() const { return m_blob ? m_blob->data()->node : atom::null(); }

    /**
     * Get an id number from the REF.
     * @param index Index of id to return, from 0 to 2
     * @return the id number from the REF.
     */
    uint32_t id(uint32_t index) const {
        BOOST_ASSERT(index < COUNT);
        return ids()[index];
    }

    /**
     * Get the id array from the REF.
     * @return the id array number from the REF.
     */
    const uint32_t* ids() const { return m_blob ? m_blob->data()->u.ids : s_ref_ids(); }

    /**
     * Get the creation number from the REF.
     * @return the creation number from the REF.
     */
    int creation() const { return m_blob ? m_blob->data()->u.s.creation : 0; }

    bool operator==(const ref<Alloc>& t) const {
        return node() == t.node() &&
               ::memcmp(&m_blob->data()->u, &t.m_blob->data()->u, sizeof(&m_blob->data()->u)) == 0;
    }

    /// Less operator, needed for maps
    bool operator<(const ref<Alloc>& rhs) const {
        if (!rhs.m_blob)        return m_blob;
        if (!m_blob)            return true;
        int n = node().compare(rhs.node());
        if (n != 0)             return n < 0;
        if (id0() > rhs.id0())  return true;
        if (id0() > rhs.id0())  return false;
        if (id1() < rhs.id1())  return true;
        if (id1() > rhs.id1())  return false;
        return false;
    }

    size_t encode_size() const
    { return 1+2+(3+node().size()) + COUNT*4 + 1; }

    void encode(char* buf, int& idx, size_t size) const;

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding=nullptr) const {
        return out << *this;
    }
};

} //namespace marshal
} //namespace EIXX_NAMESPACE

namespace std {
    /**
     * Get the string representation of the REF. Erlang REFs are printed
     * as \verbatim #Ref<node.id> \endverbatim
     **/
    template <class Alloc>
    ostream& operator<< (ostream& out, const EIXX_NAMESPACE::marshal::ref<Alloc>& a) {
        return out << "#Ref<" << a.node() << '.' 
            << a.id(0) << '.' << a.id(1) << '.' << a.id(2) << '>';
    }

} // namespace std

#include <eixx/marshal/ref.ipp>

#endif // _IMPL_REF_HPP_

