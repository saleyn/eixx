//----------------------------------------------------------------------------
/// \file  pid.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an epid object of Erlang external 
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

#ifndef _IMPL_PID_HPP_
#define _IMPL_PID_HPP_

#include <boost/static_assert.hpp>
#include <boost/concept_check.hpp>
#include <eixx/eterm_exception.hpp>
#include <eixx/marshal/atom.hpp>

namespace eixx {
namespace marshal {

/**
 * Representation of erlang Pids.
 * A pid has 4 parameters, nodeName, id, serial and creation number
 *
 */
template <class Alloc>
class epid {
    struct pid_blob {
        // creation is a special value that allows 
        // distinguishing pid values between successive node restarts.
        uint32_t id;        // only 15 bits may be used and the rest must be 0
        uint32_t serial;    // only 13 bits may be used and the rest must be 0
        uint32_t creation;
        atom     node;

        pid_blob(const atom& a_node, int a_id, int a_cre)
            : id(a_id), serial(0), creation(a_cre), node(a_node)
        {}

        pid_blob(const atom& a_node, int a_id, int serial, int a_cre)
            : id(a_id), serial(serial), creation(a_cre), node(a_node)
        {}
    };

    BOOST_STATIC_ASSERT(sizeof(pid_blob) == sizeof(uint64_t)*2);

    blob<pid_blob, Alloc>* m_blob;

    void release() {
        if (m_blob) {
            #ifdef EIXX_DEBUG
            std::cerr << "Releasing pid " << *this
                      << " [addr=" << this << ", blob=" << m_blob
                      << ", rc=" << m_blob->use_count() << ']' << std::endl;
            #endif
            m_blob->release();
        }
    }

    // Must only be called from constructor!
    void init(const atom& node, int id, int serial, uint32_t creation, const Alloc& alloc)
    {
        m_blob = new blob<pid_blob, Alloc>(1, alloc);
        new (m_blob->data()) pid_blob(node, id, serial, creation);
        #ifdef EIXX_DEBUG
        std::cerr << "Initialized pid " << *this
                  << " [addr=" << this << ", blob=" << m_blob << ']' << std::endl;
        #endif
    }

    /// @throw err_decode_exception
    /// @throw err_bad_argument
    void decode(const char* buf, uintptr_t& idx, size_t size, const Alloc& a_alloc);

public:

    static const epid null;

    epid() : m_blob(nullptr) {}

    /**
     * Create an Erlang pid from its components using provided allocator.
     * @param node the nodename.
     * @param id an arbitrary number. Only the low order 15 bits will
     * be used.
     * @param serial another arbitrary number. Only the low order 13 bits
     * will be used.
     * @param creation yet another arbitrary number. Only the low order
     * 2 bits will be used.
     * @param a_alloc is the allocator to use.
     * @throw err_bad_argument if node is empty or greater than MAX_NODE_LENGTH
     **/
    epid(const char* node, int id, int serial, uint32_t creation, const Alloc& a_alloc = Alloc()) 
        : epid(atom(node), id, serial, creation, a_alloc)
    {}

    epid(const atom& node, int id, uint32_t creation, const Alloc& a_alloc = Alloc())
        : epid(node, id, 0, creation, a_alloc)
    {}

    epid(const atom& node, int id, int serial, uint32_t creation, const Alloc& a_alloc = Alloc())
    {
        detail::check_node_length(node.size());
        init(node, id, serial, creation, a_alloc);
    }

    /// Decode the pid from a binary buffer.
    epid(const char* buf, uintptr_t& idx, size_t size, const Alloc& a_alloc = Alloc()) {
        decode(buf, idx, size, a_alloc);
    }

    epid(const epid& rhs) : m_blob(rhs.m_blob) {
        if (m_blob) {
            m_blob->inc_rc();
            #ifdef EIXX_DEBUG
            std::cerr << "Copied pid " << *this
                    << " [addr=" << this << ", blob=" << m_blob
                    << ", rc=" << m_blob->use_count() << ']' << std::endl;
            #endif
        }
    }

    epid(epid&& rhs) : m_blob(rhs.m_blob) { rhs.m_blob = nullptr; }

    ~epid() { release(); }

    epid& operator= (const epid& rhs) {
        if (this != &rhs) {
            release();
            m_blob = rhs.m_blob;
            if (!m_blob) m_blob->inc_rc();
        }
        return *this;
    }

    epid& operator= (epid&& rhs) {
        if (this != &rhs) {
            release();
            m_blob = rhs.m_blob;
            rhs.m_blob = nullptr;
        }
        return *this;
    }

    /**
     * Get the node name from the PID.
     * @return the node name from the PID.
     **/
    atom node() const { return m_blob ? m_blob->data()->node : atom::null(); }

    /**
     * Get the id number from the PID.
     * @return the id number from the PID.
     **/
    int id() const { return m_blob ? m_blob->data()->id : 0; }

    /**
     * Get the serial number from the PID.
     * @return the serial number from the PID.
     **/
    int serial() const { return m_blob ? m_blob->data()->serial : 0; }

    /**
     * Get the creation number from the PID.
     * @return the creation number from the PID.
     **/
    uint32_t creation() const { return m_blob ? m_blob->data()->creation : 0; }

    uint32_t id_internal() const { return m_blob ? m_blob->data()->id : 0; }

    bool operator== (const epid<Alloc>& rhs) const {
        return ::memcmp(m_blob->data(), rhs.m_blob->data(), sizeof(pid_blob)) == 0;
    }
    bool operator!= (const epid<Alloc>& rhs) const { return !(*this == rhs); }

    /** less operator, needed for maps */
    bool operator< (const epid<Alloc>& t2) const {
        int n = node().compare(t2.node());
        if (n < 0)                              return true;
        if (n > 0)                              return false;
        if (id_internal() < t2.id_internal())   return true;
        if (id_internal() > t2.id_internal())   return false;
        if (serial()      < t2.serial())        return true;
        if (creation()    > t2.creation())      return false;
        if (creation()    < t2.creation())      return true;
        return false;
    }
    size_t encode_size() const { return (creation() > 0x03 ? 16 : 13) + node().size(); }

    void encode(char* buf, uintptr_t& idx, size_t size) const;

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* =NULL) const {
        out << "#Pid<" << node() 
            << '.' << id() << '.' << serial();
        if (creation() > 0)
            out << ',' << creation();
        return out << '>';
    }
};

} // namespace marshal
} // namespace eixx

namespace std {
    template <typename Alloc>
    ostream& operator<< (ostream& out, const eixx::marshal::epid<Alloc>& a) {
        return a.dump(out);
    }

} // namespace std

#include <eixx/marshal/pid.hxx>

#endif // _IMPL_PID_HPP_

