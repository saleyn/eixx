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
        union u {
            struct s {
                uint16_t id      : 15;
                uint16_t serial  : 13;
                uint8_t  creation:  2;
            } __attribute__((__packed__)) s;
            uint32_t i;

            BOOST_STATIC_ASSERT(sizeof(s) == 4);

            u(int a_id, uint8_t a_cre) {
                i = (a_id & 0x0FFFffff) | ((a_cre & 0x3) << 28);
            }

            u(uint16_t a_id, uint16_t a_ser, uint8_t a_cre) {
                s.id = a_id & 0x7fff; s.serial = a_ser & 0x1fff; s.creation = a_cre & 0x03;
            }
        } u;
        atom node;

        pid_blob(const atom& a_node, int a_id, int8_t a_cre)
            : u(a_id, a_cre), node(a_node)
        {}
    };

    BOOST_STATIC_ASSERT(sizeof(pid_blob) == sizeof(uint64_t));

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
    void init(const atom& node, int id, uint8_t creation, const Alloc& alloc)
    {
        m_blob = new blob<pid_blob, Alloc>(1, alloc);
        new (m_blob->data()) pid_blob(node, id, creation);
        #ifdef EIXX_DEBUG
        std::cerr << "Initialized pid " << *this
                  << " [addr=" << this << ", blob=" << m_blob << ']' << std::endl;
        #endif
    }

    /// @throw err_decode_exception
    /// @throw err_bad_argument
    void decode(const char* buf, int& idx, size_t size, const Alloc& a_alloc);

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
    epid(const char* node, int id, int serial, int creation, const Alloc& a_alloc = Alloc()) 
        : epid(atom(node), id, serial, creation, a_alloc)
    {}

    epid(const atom& node, int id, int serial, int creation, const Alloc& a_alloc = Alloc())
        : epid(node, (id & 0x7fff) | ((serial & 0x1fff) << 15), creation, a_alloc)
    {}

    epid(const atom& node, int id, int creation, const Alloc& a_alloc = Alloc())
    {
        detail::check_node_length(node.size());
        init(node, id, creation, a_alloc);
    }

    /// Decode the pid from a binary buffer.
    epid(const char* buf, int& idx, size_t size, const Alloc& a_alloc = Alloc()) {
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
    int id() const { return m_blob ? m_blob->data()->u.s.id : 0; }

    /**
     * Get the serial number from the PID.
     * @return the serial number from the PID.
     **/
    int serial() const { return m_blob ? m_blob->data()->u.s.serial : 0; }

    /**
     * Get the creation number from the PID.
     * @return the creation number from the PID.
     **/
    int creation() const { return m_blob ? m_blob->data()->u.s.creation : 0; }

    uint32_t id_internal() const { return m_blob ? m_blob->data()->u.i & 0x3FFFffff : 0; }

    bool operator== (const epid<Alloc>& rhs) const {
        return id_internal() == rhs.id_internal() && node() == rhs.node();
    }
    bool operator!= (const epid<Alloc>& rhs) const { return !(*this == rhs); }

    /** less operator, needed for maps */
    bool operator< (const epid<Alloc>& t2) const {
        int n = node().compare(t2.node());
        if (n < 0)                              return true;
        if (n > 0)                              return false;
        if (id_internal() < t2.id_internal())   return true;
        if (id_internal() > t2.id_internal())   return false;
        return false;
    }
    size_t encode_size() const { return 13 + node().size(); }

    void encode(char* buf, int& idx, size_t size) const;

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding=NULL) const {
        return out << "#Pid<" << node() 
                   << '.' << id() << '.' << serial() << '.' << creation() << ">";
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

