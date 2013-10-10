//----------------------------------------------------------------------------
/// \file transport_otp_connection.ipp
//----------------------------------------------------------------------------
// Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>
// Created: 2010-09-11
//----------------------------------------------------------------------------
/*
***** BEGIN LICENSE BLOCK *****

This file is part of the eixx (Erlang C++ Interface) library.

Copyright (c) 2010 Serge Aleynikov <saleyn@gmail.com>

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

#ifndef _EIXX_TRANSPORT_OTP_CONNECTION_IPP_
#define _EIXX_TRANSPORT_OTP_CONNECTION_IPP_

#include <eixx/connect/transport_otp_connection_tcp.hpp>
#include <eixx/connect/transport_otp_connection_uds.hpp>
#include <eixx/marshal/endian.hpp>
#include <eixx/marshal/trace.hpp>
#include <eixx/util/string_util.hpp>
#include <eixx/marshal/string.hpp>
//#include <misc/eiext.h>                 // see erl_interface/src

namespace EIXX_NAMESPACE {
namespace connect {

using EIXX_NAMESPACE::marshal::trace;

template <class Handler, class Alloc>
const size_t connection<Handler, Alloc>::s_header_size = 4;

template <class Handler, class Alloc>
const char   connection<Handler, Alloc>::s_header_magic = 132;

template <class Handler, class Alloc>
const eterm<Alloc> connection<Handler, Alloc>::s_null_cookie;


template <class Handler, class Alloc>
typename connection<Handler, Alloc>::pointer
connection<Handler, Alloc>::create(
    boost::asio::io_service& a_svc,
    Handler*            a_h,
    atom                a_this_node,
    atom                a_node,
    atom                a_cookie,
    const Alloc&        a_alloc)
{
    BOOST_ASSERT(a_this_node.to_string().find('@') != std::string::npos);

    std::string addr(a_node.to_string());

    connection_type con_type = parse_connection_type(addr);

    switch (con_type) {
        case TCP:
            if (addr.find('@') == std::string::npos)
                THROW_RUNTIME_ERROR("Invalid node name " << a_node);
            break;
        case UDS:
            if (addr.find_last_of('/') == std::string::npos)
                THROW_RUNTIME_ERROR("Invalid node name " << a_node);
            break;
        default:
            THROW_RUNTIME_ERROR("Invalid node transport type: " << a_node);
    }

    pointer p;
    switch (con_type) {
        case TCP:  p.reset(new tcp_connection<Handler, Alloc>(a_svc, a_h, a_alloc));  break;
        case UDS:  p.reset(new uds_connection<Handler, Alloc>(a_svc, a_h, a_alloc));  break;
        default:   THROW_RUNTIME_ERROR("Not implemented! (proto=" << con_type << ')');
    }

    p->connect(a_this_node, a_node, a_cookie);
    return p;
}

template <class Handler, class Alloc>
connection_type
connection<Handler, Alloc>::parse_connection_type(std::string& s) throw(std::runtime_error)
{
    size_t pos = s.find("://", 0);
    if (pos == std::string::npos)
        return TCP;
    std::string a = s.substr(0, pos);
    const char* types[] = {"UNDEFINED", "tcp", "uds"};
    for (size_t i=1; i < sizeof(types)/sizeof(char*); ++i)
        if (boost::iequals(a, types[i])) {
            s = s.substr(pos+1);
            return static_cast<connection_type>(i);
        }
    THROW_RUNTIME_ERROR("Unknown connection type: " << s);
}

// NOTE: This is somewhat ugly because addition of new connection types
// requires modification of async_read() and async_write() functions by
// adding case statements.  However, there doesn't seem to be a way of
// handling it more generically because the socket type is statically
// defined in ASIO and boost::asio::async_read()/async_write() funcations
// are template specialized for each socket type. Consequently we
// resolve invocation of the right async function by using a switch statement.

template <class Handler, class Alloc>
template <class MutableBuffers, class CompletionCondition, class ReadHandler>
void connection<Handler, Alloc>::async_read(
    const MutableBuffers& b, const CompletionCondition& c, ReadHandler h)
{
    switch (m_type) {
        case TCP: {
            boost::asio::ip::tcp::socket& s =
                reinterpret_cast<tcp_connection<Handler, Alloc>*>(this)->socket();
            boost::asio::async_read(s, b, c, h);
            break;
        }
        case UDS: {
            boost::asio::local::stream_protocol::socket& s =
                reinterpret_cast<uds_connection<Handler, Alloc>*>(this)->socket();
            boost::asio::async_read(s, b, c, h);
            break;
        }
        default:
            THROW_RUNTIME_ERROR("async_read: Not implemented! (type=" << m_type << ')');
    }
}

template <class Handler, class Alloc>
template <class MutableBuffers, class CompletionCondition, class ReadHandler>
void connection<Handler, Alloc>::async_write(
    const MutableBuffers& b, const CompletionCondition& c, ReadHandler h)
{
    switch (m_type) {
        case TCP: {
            boost::asio::ip::tcp::socket& s =
                reinterpret_cast<tcp_connection<Handler, Alloc>*>(this)->socket();
            boost::asio::async_write(s, b, c, h);
            break;
        }
        case UDS: {
            boost::asio::local::stream_protocol::socket& s =
                reinterpret_cast<uds_connection<Handler, Alloc>*>(this)->socket();
            boost::asio::async_write(s, b, c, h);
            break;
        }
        default:
            THROW_RUNTIME_ERROR("async_write: Not implemented! (type=" << m_type << ')');
    }
}

template <class Handler, class Alloc>
void connection<Handler, Alloc>::
handle_write(const boost::system::error_code& err)
{
    if (m_connection_aborted) {
        if (unlikely(verbose() >= VERBOSE_TRACE))
            m_handler->report_status(REPORT_INFO,
                "Connection aborted - exiting connection::handle_write");
        return;
    }

    if (unlikely(err)) {
        if (verbose() >= VERBOSE_TRACE) {
            std::stringstream s;
            s << "connection::handle_write(" << err.value() << ')';
            m_handler->report_status(REPORT_INFO, s.str());
        }
        // We use operation_aborted as a user-initiated connection reset,
        // therefore check to substitute the error since bytes_transferred == 0
        // means a connection loss.
        boost::system::error_code e = err == boost::asio::error::operation_aborted
            ? boost::asio::error::not_connected : err;
        stop(e);
        return;
    }
    auto& q = m_out_msg_queue[writing_queue()];
    for (auto it  = q.begin(), end = q.end(); it != end; ++it) {
        const char* p = boost::asio::buffer_cast<const char*>(*it);
        // Don't forget to adjust for the header magic byte.
        BOOST_ASSERT(*(p - 1) == s_header_magic);
        m_allocator.deallocate(
            const_cast<char*>(p-1), boost::asio::buffer_size(*it)+1);
    }
    m_out_msg_queue[writing_queue()].clear();
    m_is_writing = false;
    do_write_internal();
}

template <class Handler, class Alloc>
void connection<Handler, Alloc>::
handle_read(const boost::system::error_code& err, size_t bytes_transferred)
{
    if (unlikely(verbose() >= VERBOSE_WIRE)) {
        std::stringstream s;
        s << "connection::handle_read(transferred="
          << bytes_transferred << ", got_header="
          << (m_got_header ? "true" : "false")
          << ", rd_buf.size=" << m_rd_buf.capacity()
          << ", rd_ptr=" << (m_rd_ptr - &m_rd_buf[0])
          << ", rd_end=" << (m_rd_end - &m_rd_buf[0])
          << ", rd_capacity=" << rd_capacity()
          << ", pkt_sz=" << m_packet_size << " (ec="
          << err.value() << ')';
        m_handler->report_status(REPORT_INFO, s.str());
    }

    if (unlikely(m_connection_aborted)) {
        if (verbose() >= VERBOSE_WIRE) {
            m_handler->report_status(REPORT_INFO,
                "Connection aborted - exiting connection::handle_read");
        }
        return;
    } else if (unlikely(err)) {
        // We use operation_aborted as a user-initiated connection reset,
        // therefore check to substitute the error since bytes_transferred == 0
        // means a connection loss.
        boost::system::error_code e =
            err == boost::asio::error::operation_aborted && bytes_transferred == 0
            ? boost::asio::error::not_connected : err;
        stop(e);
        return;
    }

    m_rd_end += bytes_transferred;

    if (!m_got_header) {
        size_t len = rd_length();

        m_got_header = len >= s_header_size;

        if (m_got_header) {
            // Make sure that the buffer size is large enouch to store
            // next message.
            m_packet_size = cast_be<uint32_t>(m_rd_ptr);
            if (m_packet_size > m_rd_buf.capacity()-s_header_size) {
                size_t begin_offset = m_rd_ptr - &m_rd_buf[0];
                m_rd_buf.reserve(begin_offset + m_packet_size + s_header_size);
                m_rd_ptr = &m_rd_buf[0] + begin_offset;
                m_rd_end = m_rd_ptr + len;
            }
        }
    }

    long need_bytes = m_packet_size + s_header_size - rd_length();

    /*
    if (unlikely(verbose() >= VERBOSE_WIRE))
        std::cout << "  pkt_size=" << m_packet_size << ", need=" << need_bytes
                  << ", rd_ptr=" << (m_rd_ptr - &m_rd_buf[0])
                  << ", rd_end=" << (m_rd_end - &m_rd_buf[0])
                  << ", length=" << rd_length()
                  << ", rd_buf.size=" << m_rd_buf.capacity()
                  << ", got_header=" << (m_got_header ? "true" : "false")
                  << ", " << to_binary_string(m_rd_ptr, std::min(rd_length(), 15lu)) << "..."
                  << std::endl;
    */

    // Process all messages in the buffer
    while (m_got_header && need_bytes <= 0) {
        m_rd_ptr += s_header_size;
        m_in_msg_count++;

        try {
            /*
            if (unlikely(verbose() >= VERBOSE_WIRE)) {
                std::cout << " MsgCnt=" << m_in_msg_count
                          << ", pkt_size=" << m_packet_size << ", need=" << need_bytes
                          << ", rd_buf.size=" << m_rd_buf.capacity()
                          << ", rd_ptr=" << (m_rd_ptr - &m_rd_buf[0])
                          << ", rd_end=" << (m_rd_end - &m_rd_buf[0])
                          << ", len=" << rd_length()
                          << ", rd_capacity=" << rd_capacity()
                          << std::endl;
                to_binary_string(
                    std::cout << "client <- server: ", m_rd_ptr, m_packet_size) << std::endl;
            }
            */

            // Decode the packet into a message and dispatch it.
            process_message(m_rd_ptr, m_packet_size);

        } catch (std::exception& e) {
            ON_ERROR_CALLBACK(this,
                "Error processing packet from server: " << e.what() << std::endl << "  ";
                to_binary_string(m_rd_ptr, m_packet_size));
        }
        m_rd_ptr     += m_packet_size;
        m_got_header  = rd_length() >= (long)s_header_size;
        if (m_got_header) {
            m_packet_size = cast_be<uint32_t>(m_rd_ptr);
            need_bytes    = m_packet_size + s_header_size - rd_length();
        }
    }
    bool crunched = false;

    if (m_rd_ptr == m_rd_end) {
        m_rd_ptr = &m_rd_buf[0];
        m_rd_end = m_rd_ptr;
        m_packet_size = s_header_size;
        need_bytes    = m_packet_size;
    } else if ((m_rd_ptr - (&m_rd_buf[0] + s_header_size)) > 0) {
        // Crunch the buffer by copying leftover bytes to the beginning of the buffer.
        const size_t len = m_rd_end - m_rd_ptr;
        char* begin = &m_rd_buf[0];
        if (likely((size_t)(m_rd_ptr - begin) < len))
            memcpy(begin, m_rd_ptr, len);
        else
            memmove(begin, m_rd_ptr, len);
        m_rd_ptr = begin;
        m_rd_end = begin + len;
        crunched = true;
    }

    if (unlikely(verbose() >= VERBOSE_WIRE)) {
        std::stringstream s;
        s << "Scheduling connection::async_read(offset="
          << (m_rd_end-&m_rd_buf[0])
          << ", capacity=" << rd_capacity() << ", pkt_size="
          << m_packet_size << ", need=" << need_bytes
          << ", got_header=" << (m_got_header ? "true" : "false")
          << ", crunched=" << (crunched ? "true" : "false")
          << ", aborted=" << (m_connection_aborted ? "true" : "false") << ')';
        m_handler->report_status(REPORT_INFO, s.str());
    }

    boost::asio::mutable_buffers_1 buffers(m_rd_end, rd_capacity());
    async_read(
        buffers, boost::asio::transfer_at_least(need_bytes),
        std::bind(&connection<Handler, Alloc>::handle_read, this->shared_from_this(),
            std::placeholders::_1,
            std::placeholders::_2));
}

/// Decode distributed Erlang message.  The message must be fully
/// stored in \a mbuf.
/// Note: TICK message is represented by msg type = 0, in this case \a a_cntrl_msg
/// and \a a_msg are invalid.
/// @return message type
template <class Handler, class Alloc>
int connection<Handler, Alloc>::
transport_msg_decode(const char *mbuf, int len, transport_msg<Alloc>& a_tm)
    throw(err_decode_exception)
{
    const char* s = mbuf;
    int version;
    int index = 0;

    if (unlikely(len == 0)) // This is TICK message
        return ERL_TICK;

    /* now decode header */
    /* pass-through, version, control tuple header, control message type */
    if (unlikely(get8(s) != ERL_PASS_THROUGH)) {
        int n = len < 65 ? len : 64;
        std::string s = std::string("Missing pass-throgh flag in message")
                      + to_binary_string(mbuf, n);
        throw err_decode_exception(s, len);
    }

    if (unlikely(ei_decode_version(s,&index,&version) || version != ERL_VERSION_MAGIC))
        throw err_decode_exception("Invalid control message magic number", version);

    tuple<Alloc> cntrl(s, index, len, m_allocator);

    int msgtype = cntrl[0].to_long();

    if (unlikely(msgtype <= ERL_TICK) || unlikely(msgtype > ERL_MONITOR_P_EXIT))
        throw err_decode_exception("Invalid message type", msgtype);

    static const uint32_t types_with_payload = 1 << ERL_SEND
                                             | 1 << ERL_REG_SEND
                                             | 1 << ERL_SEND_TT
                                             | 1 << ERL_REG_SEND_TT;
    if (likely((1 << msgtype) & types_with_payload)) {
        if (unlikely(ei_decode_version(s,&index,&version)) || unlikely((version != ERL_VERSION_MAGIC)))
            throw err_decode_exception("Invalid message magic number", version);

        eterm<Alloc> msg(s, index, len, m_allocator);
        a_tm.set(msgtype, cntrl, &msg);
    } else {
        a_tm.set(msgtype, cntrl);
    }

    return msgtype;
}

template <class Handler, class Alloc>
void connection<Handler, Alloc>::
process_message(const char* a_buf, size_t a_size)
{
    transport_msg<Alloc> tm;
    int msgtype = transport_msg_decode(a_buf, a_size, tm);

    switch (msgtype) {
        case ERL_TICK: {
            // Reply with TOCK packet
            char* data = allocate(s_header_size);
            bzero(data, s_header_size);
            boost::asio::const_buffer b(data, s_header_size);
            do_write(b);
            break;
        }
        /*
        case ERL_SEND:
        case ERL_REG_SEND:
        case ERL_LINK:
        case ERL_UNLINK:
        case ERL_GROUP_LEADER:
        case ERL_EXIT:
        case ERL_EXIT2:
        case ERL_NODE_LINK:
        case ERL_MONITOR_P:
        case ERL_DEMONITOR_P:
        case ERL_MONITOR_P_EXIT:
            ...
            break;
        */
        default:
            if (unlikely(verbose() >= VERBOSE_MESSAGE)) {
                if (unlikely(verbose() >= VERBOSE_WIRE)) {
                    std::stringstream s;
                    s << "Got transport msg - (cntrl): " << tm.cntrl();
                    m_handler->report_status(REPORT_INFO, s.str());
                }
                if (tm.has_msg()) {
                    std::stringstream s;
                    s << "Got transport msg - (msg):   " << tm.msg();
                    m_handler->report_status(REPORT_INFO, s.str());
                }
            }
            m_handler->on_message(this, tm);
    }
}

template <class Handler, class Alloc>
void connection<Handler, Alloc>::
send(const transport_msg<Alloc>& a_msg)
{
    if (!check_connected(&a_msg.msg()))
        return;

    eterm<Alloc> l_cntrl(a_msg.cntrl());
    bool   l_has_msg= a_msg.has_msg();
    size_t cntrl_sz = l_cntrl.encode_size(0, true);
    size_t msg_sz   = l_has_msg ? a_msg.msg().encode_size(0, true) : 0;
    size_t len      = cntrl_sz + msg_sz + 1 /*passthrough*/ + 4 /*len*/;
    char*  data     = allocate(len);
    char*  s        = data;
    put32be(s, len-4);
    *s++ = ERL_PASS_THROUGH;
    l_cntrl.encode(s, cntrl_sz, 0, true);
    if (l_has_msg)
        a_msg.msg().encode(s + cntrl_sz, msg_sz, 0, true);

    if (unlikely(verbose() >= VERBOSE_MESSAGE)) {
        std::stringstream s;
        s << "SEND cntrl="
          << l_cntrl.to_string() << (l_has_msg ? ", msg=" : "")
          << (l_has_msg ? a_msg.msg().to_string() : std::string(""));
        m_handler->report_status(REPORT_INFO, s.str());
    }
    //if (unlikely(verbose() >= VERBOSE_WIRE))
    //    std::cout << "SEND " << len << " bytes " << to_binary_string(data, len) << std::endl;

    boost::asio::const_buffer b(data, len);
    m_io_service.post(
        std::bind(&connection<Handler, Alloc>::do_write, this->shared_from_this(), b));
}

} // namespace connect
} // namespace EIXX_NAMESPACE

#endif // _EIXX_TRANSPORT_OTP_CONNECTION_IPP_

