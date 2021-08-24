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

#ifndef _IMPL_REF_HPP_
#define _IMPL_REF_HPP_

#include <boost/static_assert.hpp>
#include <boost/assert.hpp>
#include <boost/iterator/iterator_concepts.hpp>
#include <eixx/marshal/atom.hpp>
#include <eixx/eterm_exception.hpp>

namespace eixx {
namespace marshal {
namespace detail {
} // namespace detail

/**
 * Representation of erlang Pids.
 * A ref has 5 parameters, nodeName, three ids, and creation number
 */
template <class Alloc>
class ref {
    enum { COUNT = 5 /* max of 5 when the DFLAG_V4_NC has been set */ };

    struct ref_blob {
        atom     node;
        uint16_t len;
        uint32_t ids[COUNT];
        uint32_t creation;

        ref_blob(const atom& a_node, const uint32_t* a_ids, uint16_t n, uint32_t a_cre)
            : node(a_node)
            , len(n)
            , creation(a_cre)
        {
            assert(n >= 3 && n <= COUNT);

            int i = 0;
            for (auto p = a_ids, e = a_ids+std::min<size_t>(COUNT,n); p != e; ++p)
                ids[i++] = *p;

            while(i < COUNT)
                ids[i++] = 0;
        }
        template <int N>
        ref_blob(const atom& node, uint32_t (&ids)[N], uint32_t creation)
            : ref_blob(node, ids, N, creation)
        {}

        ref_blob(const atom& a_node, std::initializer_list<uint32_t> a_ids, uint32_t a_cre)
            : ref_blob(node, &*a_ids.begin(), a_ids.size(), creation)
        {}
    };

    blob<ref_blob, Alloc>* m_blob;

    // Must only be called from constructor!
    void init(const atom& a_node, const uint32_t* a_ids, uint16_t n, uint32_t a_cre,
              const Alloc& alloc)
    {
        detail::check_node_length(a_node.size());

        m_blob = new blob<ref_blob, Alloc>(1, alloc);
        new (m_blob->data()) ref_blob(a_node, a_ids, n, a_cre);
    }

    void release() {
        if (m_blob)
            m_blob->release();
    }

    uint32_t id0() const { return m_blob->data()->ids[0]; }
    uint64_t id1() const { return m_blob->data()->ids[1]; }

public:
    inline static const uint32_t* s_ref_ids() {
       static const uint32_t s_ref_ids[] = {0, 0, 0, 0, 0};
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
    ref(const atom& node, const uint32_t* a_ids, size_t n, uint32_t creation,
        const Alloc& a_alloc = Alloc())
    {
        init(node, a_ids, n, creation, a_alloc);
    }

    template <int N>
    ref(const atom& node, uint32_t (&ids)[N], uint32_t creation,
        const Alloc& a_alloc = Alloc())
        : ref(node, ids, N, creation, a_alloc)
    {
        BOOST_STATIC_ASSERT(N >= 3 && N <= 5);
    }

    ref(const atom& node, uint32_t id0, uint32_t id1, uint32_t id2, uint32_t creation,
        const Alloc& a_alloc = Alloc())
        : ref(node, {id0, id1, id2}, creation, a_alloc)
    {}

    // For internal use
    ref(const atom& node, std::initializer_list<uint32_t> a_ids, uint8_t creation,
        const Alloc& a_alloc = Alloc())
        : ref(node, &*a_ids.begin(), a_ids.size(), creation, a_alloc)
    {}

    /**
     * Construct the object by decoding it from a binary
     * encoded buffer.
     * @param buf is the buffer containing Erlang external binary format.
     * @param idx is the current offset in the buf buffer.
     * @param size is the size of the \a buf buffer.
     * @param a_alloc is the allocator to use.
     */
    ref(const char* buf, uintptr_t& idx, size_t size, const Alloc& a_alloc = Alloc());

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
        BOOST_ASSERT(index < len());
        return ids()[index];
    }

    /**
     * Get the id array from the REF.
     * @return the id array number from the REF.
     */
    const uint32_t* ids() const { return m_blob ? m_blob->data()->ids : s_ref_ids(); }

    /**
     * Get the id array from the REF.
     * @return the id array number from the REF.
     */
    uint16_t len() const { return m_blob ? m_blob->data()->len : 0; }

    /**
     * Get the creation number from the REF.
     * @return the creation number from the REF.
     */
    uint32_t creation() const { return m_blob ? m_blob->data()->creation : 0; }

    bool operator==(const ref<Alloc>& t) const {
        return ::memcmp(m_blob->data(), t.m_blob->data(), sizeof(ref_blob)) == 0;
    }

    /// Less operator, needed for maps
    bool operator<(const ref<Alloc>& rhs) const {
        if (!rhs.m_blob)        return m_blob;
        if (!m_blob)            return true;
        int n = node().compare(rhs.node());
        if (n != 0)             return n < 0;
        auto e = std::min(len(), rhs.len());
        for (uint32_t i=0; i < e; ++i) {
            auto i1 = id(i);
            auto i2 = rhs.id(i);
            if (i1 > i2) return false;
            if (i1 < i2) return true;
        }
        if (len()      < rhs.len())       return true;
        if (len()      > rhs.len())       return false;
        if (creation() < rhs.creation())  return true;
        if (creation() > rhs.creation())  return false;
        return false;
    }

    size_t encode_size() const
    { return 1+2+(3+node().size()) + len()*4 + (creation() > 0x03 ? 4 : 1); }

    void encode(char* buf, uintptr_t& idx, size_t size) const;

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* =nullptr) const {
        return out << *this;
    }
};

} //namespace marshal
} //namespace eixx

namespace std {
    /**
     * Get the string representation of the REF. Erlang REFs are printed
     * as \verbatim #Ref<node.id> \endverbatim
     **/
    template <class Alloc>
    ostream& operator<< (ostream& out, const eixx::marshal::ref<Alloc>& a) {
        out << "#Ref<" << a.node();
        for (int i=0, e=a.len(); i != e; ++i)
            out << '.' << a.id(i);
        if (a.creation() > 0)
            out << ',' << a.creation();
        return out << '>';
    }

} // namespace std

#include <eixx/marshal/ref.hxx>

#endif // _IMPL_REF_HPP_

