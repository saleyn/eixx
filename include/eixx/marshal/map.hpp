//----------------------------------------------------------------------------
/// \file  map.hpp
//----------------------------------------------------------------------------
/// \brief A class implementing an map object of Erlang external term format.
//----------------------------------------------------------------------------
// Copyright (c) 2020 Serge Aleynikov <saleyn@gmail.com>
// Created: 2020-06-10
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
#ifndef _IMPL_MAP_HPP_
#define _IMPL_MAP_HPP_

#include <ostream>
#include <initializer_list>
#include <map>
#include <boost/assert.hpp>
#include <eixx/marshal/alloc_base.hpp>
#include <eixx/marshal/varbind.hpp>
#include <eixx/marshal/visit.hpp>
#include <eixx/marshal/visit_encode_size.hpp>
#include <ei.h>

namespace eixx {
namespace marshal {

template <typename Alloc> class eterm;

template <typename Alloc>
class map {
public:
    using MapAlloc = typename Alloc::template rebind<std::pair<const eterm<Alloc>, eterm<Alloc>>>::other;
    using MapT     = std::map<eterm<Alloc>, eterm<Alloc>, std::less<eterm<Alloc>>, MapAlloc>;
protected:
    blob<MapT, Alloc>* m_blob;

    void release() { release(m_blob); m_blob = nullptr; }

    void release(blob<MapT, Alloc>* p) {
        if (p && p->release(false)) {
            p->data()->~MapT();
            p->free();
        }
    }

    void initialize(const Alloc& alloc = Alloc()) {
        m_blob = new blob<MapT, Alloc>(1, alloc);
        auto* m = m_blob->data();
        new  (m)  MapT();
    }
public:
    using const_iterator = typename MapT::const_iterator;
    using iterator       = typename MapT::iterator;

    static const map<Alloc>& null() { static map<Alloc> s = map<Alloc>(Alloc()); return s; }

    explicit map(std::nullptr_t) : m_blob(nullptr) {}

    map(const Alloc& a = Alloc()) {
        initialize(a);
    }

    map(const map<Alloc>& s) : m_blob(s.m_blob) {
        if (m_blob) m_blob->inc_rc();
    }

    map(map<Alloc>&& s) : m_blob(s.m_blob) {
        s.m_blob = nullptr;
    }

    map(std::initializer_list<std::pair<eterm<Alloc>,eterm<Alloc>>> items, const Alloc& alloc = Alloc()) {
        initialize(alloc);
        auto* m = m_blob->data();
        for  (auto& pair : items)
            m->insert(pair);
    }

    map(const char* buf, int& idx, size_t size, const Alloc& a_alloc = Alloc()) {
        int arity;
        if (ei_decode_map_header(buf, &idx, &arity) < 0)
            err_decode_exception("Error decoding tuple header", idx);
        initialize(a_alloc);
        auto* m = m_blob->data();
        for (int i=0; i < arity; i++) {
            auto key = eterm<Alloc>(buf, idx, size, a_alloc);
            auto val = eterm<Alloc>(buf, idx, size, a_alloc);
            m->emplace(std::make_pair(key, val));
        }
        BOOST_ASSERT((size_t)idx <= size);
    }

    ~map() {
        release();
    }

    map<Alloc>& operator= (const map<Alloc>& s) {
        if (this != &s) {
            release();
            m_blob = s.m_blob;
            if (m_blob) m_blob->inc_rc();
        }
        return *this;
    }

    map<Alloc>& operator= (map<Alloc>&& s) {
        if (this != &s) {
            release();
            m_blob = s.m_blob;
            s.m_blob = nullptr;
        }
        return *this;
    }

    const eterm<Alloc>& operator[](const eterm<Alloc>& key) const {
        static const eterm<Alloc> s_undefined = am_undefined;
        if (!m_blob) return s_undefined;
        auto    it =  m_blob->data()->find(key);
        return (it == m_blob->data()->end()) ? s_undefined : it->second;
    }

    void        insert(const eterm<Alloc>& key, const eterm<Alloc>& val) {
        if (!m_blob) initialize();
        m_blob->data()->insert(std::make_pair(key, val));
    }

    void        erase(const eterm<Alloc>& key) {
        if (!m_blob) return;
        m_blob->data()->erase(key);
    }

    const_iterator begin()  const { return m_blob ? m_blob->data()->cbegin() : null().begin(); }
    const_iterator end()    const { return m_blob ? m_blob->data()->cend()   : null().end();   }

    size_t      size()   const { return m_blob ? m_blob->data()->size() : 0; }
    bool        empty()  const { return !m_blob || m_blob->data()->empty();  }

    void        clear()        { if (m_blob) m_blob->data().clear(); }

    // Use only for debugging
    int         use_count() const { return m_blob ? m_blob->use_count() : -1000000; }

    bool operator== (const map<Alloc>& rhs) const {
        if (size() != rhs.size()) return false;
        auto it1  = begin();
        auto it2  = rhs.begin();
        auto end1 = end();
        for (; it1 != end1; ++it1, ++it2)
            if (!(*it1 == *it2))
                return false;
        return true;
    }

    bool operator<  (const map<Alloc>& rhs) const {
        if (size() < rhs.size()) return true;
        if (size() > rhs.size()) return false;
        BOOST_ASSERT(size() == rhs.size());
        auto it1  = begin();
        auto it2  = rhs.begin();
        auto end1 = end();
        for (; it1 != end1; ++it1, ++it2) {
            // 1. Is key1 < key2?
            if (it1->first < it2->first)
                return true;
            if (it2->first < it1->first)
                return false;
            // 1. Is value1 < value2?
            if (it1->second < it2->second)
                return true;
            if (it2->second < it1->second)
                return false;
        }
        return size() == 0;
    }

    /** Size of buffer needed to hold the encoded map. */
    size_t encode_size() const {
        BOOST_ASSERT(m_blob);
        size_t result = 5;
        for (const_iterator it = begin(), iend = end(); it != iend; ++it) {
            result += visit_eterm_encode_size_calc<Alloc>().apply_visitor(it->first);  // key
            result += visit_eterm_encode_size_calc<Alloc>().apply_visitor(it->second); // value
        }
        return result;
    }

    void encode(char* buf, int& idx, size_t size) const {
        BOOST_ASSERT(m_blob);
        ei_encode_map_header(buf, &idx, this->size());
        for(auto it = begin(), iend=end(); it != iend; ++it) {
            visit_eterm_encoder key_visitor(buf, idx, size);
            key_visitor.apply_visitor(it->first);
            visit_eterm_encoder val_visitor(buf, idx, size);
            val_visitor.apply_visitor(it->second);
        }
        BOOST_ASSERT((size_t)idx <= size);
    }

    std::ostream& dump(std::ostream& out, const varbind<Alloc>* binding=NULL) const {
        out << "#{";
        for(auto it = begin(), first=begin(), iend=end(); it != iend; ++it) {
            out << (it != first ? "," : "");
            visit_eterm_stringify<Alloc> key_visitor(out, binding);
            key_visitor.apply_visitor(it->first);
            out << " => ";
            visit_eterm_stringify<Alloc> val_visitor(out, binding);
            val_visitor.apply_visitor(it->second);
        }
        return out << '}';
    }
};

} // namespace marshal
} // namespace eixx

namespace std {

    template <typename Alloc>
    ostream& operator<< (ostream& out, const eixx::marshal::map<Alloc>& m) { return m.dump(out); }

}

//#include <eixx/marshal/map.hxx>

#endif // _IMPL_MAP_HPP_
