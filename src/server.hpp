//
// server.hpp
// ~~~~~~~~~~~~~~
//
// Unified channel class implementing TCP/UDP server types.
//
// Copyright (c) 2010 Serge Aleynikov <serge@aleynikov.org>
// Created: 2010-04-11
//

#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <boost/asio.hpp>
#include <string>
#include <boost/noncopyable.hpp>
#include "channel.hpp"

namespace perc {

//------------------------------------------------------------------------------
// channel_manager class
//------------------------------------------------------------------------------

/// Manages open channels so that they may be cleanly stopped when the server
/// needs to shut down.
template <class Connection>
class channel_manager : private boost::noncopyable
{
public:
    typedef std::shared_ptr<Connection> channel_ptr;

    /// Add the specified channel to the manager and start it.
    void start(channel_ptr c, bool a_connected = false) {
        m_channels.insert(c);
        c->start(a_connected);
    }

    /// Stop the specified channel.
    void stop(channel_ptr c) {
        m_channels.erase(c);
        c->stop();
        c.reset();
    }

    /// Stop all channels.
    void stop_all() {
        std::for_each(m_channels.begin(), m_channels.end(),
            std::bind(&Connection::stop, _1));
        m_channels.clear();
    }

private:
    /// The managed channels.
    std::set<channel_ptr> m_channels;
};


// Forward declaration
template<class Handler> class tcp_server;
template<class Handler> class uds_server;

//------------------------------------------------------------------------------
// server class
//------------------------------------------------------------------------------

/// The top-level class of the HTTP server.
template<class Handler>
class server : private boost::noncopyable
{
public:
    typedef Handler handler_type;
    typedef std::shared_ptr<server<Handler> > pointer;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    server(boost::asio::io_service& io, handler_type& h)
        : m_io_service(io)
        , m_request_handler(h)
    {}

    virtual ~server() {}

    static pointer create(
            boost::asio::io_service& a_svc,
            handler_type& a_h,
            const boost::property_tree::ptree& pt) {
        std::string url = pt.get<std::string>("address", "");
        size_t n = url.find(':');
        std::string stype = url.substr(0, n);
        if (stype.empty())
            THROW_RUNTIME_ERROR("Missing 'address' configuration option");
        typename channel<handler_type>::channel_type type =
            channel<handler_type>::parse_channel_type(stype);
        pointer p = create(a_svc, a_h, type);
        p->init(pt);
        return p;
    }
    static pointer create(
            boost::asio::io_service& a_svc,
            handler_type& a_h,
            typename channel<handler_type>::channel_type a_type) {
        switch (a_type) {
            case channel<handler_type>::TCP:
                return pointer(new tcp_server<handler_type>(a_svc, a_h));
            case channel<handler_type>::UDS:
                return pointer(new uds_server<handler_type>(a_svc, a_h));
            default:
                THROW_RUNTIME_ERROR("Unknown server type: " << a_type);
        }
    }

    virtual void init(const boost::property_tree::ptree& pt)
            throw(std::runtime_error) = 0;

    /// Start the listening process.
    virtual void start() {}

    /// Run the server's io_service loop.
    virtual void run() {
        // The io_service::run() call will block until all asynchronous operations
        // have finished. While the server is running, there is always at least one
        // asynchronous operation outstanding: the asynchronous accept call waiting
        // for new incoming channels.
        m_io_service.run();
    }

    /// Stop the server.
    virtual void stop() {
        // Post a call to the stop function so that server::stop() is safe to call
        // from any thread.
        m_io_service.post(std::bind(&server::handle_stop, this));
    }

    // Event handlers
    boost::function<void (channel<Handler>*)>    on_connect;
    boost::function<void (channel<Handler>*,
              const boost::system::error_code&)>    on_disconnect;
    boost::function<void (channel<Handler>*,
              const std::string&)>                  on_client_error;
    boost::function<void (server<Handler>*,
              const std::string&)>                  on_server_error;

protected:
    /// Handle a request to stop the server.
    virtual void handle_stop() {
        m_channels.stop_all();
    }

    bool handle_accept_internal(const boost::system::error_code& e,
                                typename channel<Handler>::pointer con) {
        if (e) {
            if (!this->on_server_error)
                THROW_RUNTIME_ERROR("Error in server::handle_accept: " << e.message());
            this->on_server_error(static_cast<server<handler_type>*>(this), e.message());
            return false;
        }
        Handler& h = con->handler();
        con->on_disconnect = this->on_disconnect;
        con->on_error      = this->on_client_error;
        if (this->on_connect)
            this->on_connect(con.get());
        this->m_channels.start(con, true);
        return true;
    }

    /// The io_service used to perform asynchronous operations.
    boost::asio::io_service&  m_io_service;
    /// The handler for all incoming requests.
    Handler&                  m_request_handler;

    /// The channel manager which owns all live channels.
    channel_manager< channel<Handler> > m_channels;
};


/// The TCP server.
template<class Handler>
class tcp_server : public server< Handler >
{
public:
    typedef server< Handler > base_t;

    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    tcp_server(boost::asio::io_service& io, Handler& h)
        : base_t(io, h)
        , m_acceptor(this->m_io_service)
        , m_new_channel(
            tcp_channel<Handler>::create(this->m_io_service, h, channel<Handler>::TCP)
          )
    {
    }

    void init(const boost::property_tree::ptree& pt) throw(std::runtime_error) {
        if (m_acceptor.is_open())
            m_acceptor.close();

        std::string url = pt.get<std::string>("address", "tcp://0.0.0.0:0");
        std::string proto, addr, port, path;
        if (!parse_url(url, proto, addr, port, path))
            THROW_RUNTIME_ERROR("Invalid URL address: " << url);
        if (proto != "tcp")
            THROW_RUNTIME_ERROR("Expected 'tcp' protocol type!");
        if (port.empty() || port == "0")
            THROW_RUNTIME_ERROR("tcp_server invalid 'port' configuration: " << port);

        // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
        boost::asio::ip::tcp::resolver resolver(this->m_io_service);
        boost::asio::ip::tcp::resolver::query query(addr, port);
        boost::asio::ip::tcp::endpoint end;
        m_endpoint = *resolver.resolve(query);

        if (m_endpoint == end)
            THROW_RUNTIME_ERROR("Error resolving address");
    }

    void start() {
        m_acceptor.open(m_endpoint.protocol());
        m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor.bind(m_endpoint);
        m_acceptor.listen();
        m_acceptor.async_accept(
            static_cast<tcp_channel<Handler>&>(*m_new_channel).socket(),
            std::bind(&tcp_server<Handler>::handle_accept,
                        this, boost::asio::placeholders::error));
    }

private:
    /// Acceptor used to listen for incoming channels.
    boost::asio::ip::tcp::acceptor  m_acceptor;
    boost::asio::ip::tcp::endpoint  m_endpoint;
    /// The next channel to be accepted.
    typename tcp_channel<Handler>::pointer m_new_channel;

    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e) {
        if (handle_accept_internal(e, m_new_channel)) {
            m_new_channel =
                tcp_channel<Handler>::create(this->m_io_service,
                        m_new_channel->handler(), channel<Handler>::TCP);
            m_acceptor.async_accept(
                static_cast<tcp_channel<Handler>&>(*m_new_channel).socket(),
                std::bind(&tcp_server<Handler>::handle_accept,
                            this, boost::asio::placeholders::error));
        }
    }

    /// Handle a request to stop the server.
    virtual void handle_stop() {
        // The server is stopped by cancelling all outstanding asynchronous
        // operations. Once all operations have finished the io_service::run() call
        // will exit.
        m_acceptor.close();
        base_t::handle_stop();
    }
};

/// The Unix Domain Socket server.
template<class Handler>
class uds_server : public server< Handler >
{
public:
    typedef server< Handler > base_t;

    /// Construct the server to listen on the specified UDS filename, and
    /// serve up files from the given directory.
    uds_server(boost::asio::io_service& io, Handler& h)
        : base_t(io, h)
        , m_acceptor(this->m_io_service)
        , m_new_channel(
            uds_channel<Handler>::create(this->m_io_service, h, channel<Handler>::UDS)
          )
    {
    }

    ~uds_server() {
        if (!m_endpoint.path().empty())
            ::unlink(m_endpoint.path().c_str());
    }

    void init(const boost::property_tree::ptree& pt) throw(std::runtime_error) {
        if (m_acceptor.is_open())
            m_acceptor.close();

        std::string url = pt.get<std::string>("address", "");
        std::string proto, addr, port, path;
        if (url.empty() || !parse_url(url, proto, addr, port, path))
            THROW_RUNTIME_ERROR("Invalid URL address: '" << url << "'");
        if (proto != "uds")
            THROW_RUNTIME_ERROR("Expected 'uds' protocol type!");
        if (path.empty())
            THROW_RUNTIME_ERROR("uds_server empty 'address' configuration");
        m_endpoint.path(path);
    }

    void start() {
        m_acceptor.open(m_endpoint.protocol());
        m_acceptor.set_option(
            boost::asio::local::stream_protocol::acceptor::reuse_address(true));
        m_acceptor.bind(m_endpoint);
        m_acceptor.listen();
        // Open the acceptor
        m_acceptor.async_accept(
            static_cast<uds_channel<Handler>&>(*m_new_channel).socket(),
            std::bind(&uds_server<Handler>::handle_accept,
                        this, boost::asio::placeholders::error));
    }

private:
    /// Acceptor used to listen for incoming channels on Unix Domain Socket.
    boost::asio::local::stream_protocol::acceptor  m_acceptor;
    boost::asio::local::stream_protocol::endpoint  m_endpoint;
    /// The next channel to be accepted.
    typename uds_channel<Handler>::pointer m_new_channel;

    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e) {
        if (handle_accept_internal(e, m_new_channel)) {
            m_new_channel =
                uds_channel<Handler>::create(this->m_io_service,
                        m_new_channel->handler(), channel<Handler>::UDS);
            m_acceptor.async_accept(
                static_cast<uds_channel<Handler>&>(*m_new_channel).socket(),
                std::bind(&uds_server<Handler>::handle_accept,
                            this, boost::asio::placeholders::error));
        }
    }

    /// Handle a request to stop the server.
    virtual void handle_stop() {
        // The server is stopped by cancelling all outstanding asynchronous
        // operations. Once all operations have finished the io_service::run() call
        // will exit.
        m_acceptor.close();
        base_t::handle_stop();
    }
};

} // namespace perc

#endif // _SERVER_HPP_
