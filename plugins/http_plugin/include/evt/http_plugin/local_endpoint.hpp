#pragma once

#include <sstream>
#include <string>

#include <websocketpp/transport/base/endpoint.hpp>
#include <websocketpp/transport/asio/connection.hpp>
#include <websocketpp/transport/asio/security/none.hpp>

#include <websocketpp/uri.hpp>
#include <websocketpp/logger/levels.hpp>

#include <websocketpp/common/functional.hpp>

namespace websocketpp { namespace transport { namespace asio {

namespace local_socket {

/// The signature of the socket init handler for this socket policy
typedef lib::function<void(connection_hdl, lib::asio::local::stream_protocol::socket&)>
    socket_init_handler;

class connection : public lib::enable_shared_from_this<connection> {
public:
    /// Type of this connection socket component
    typedef connection type;
    /// Type of a shared pointer to this connection socket component
    typedef lib::shared_ptr<type> ptr;

    /// Type of a pointer to the Asio io_service being used
    typedef lib::asio::io_service* io_service_ptr;
    /// Type of a pointer to the Asio io_service strand being used
    typedef lib::shared_ptr<lib::asio::io_service::strand> strand_ptr;
    /// Type of the ASIO socket being used
    typedef lib::asio::local::stream_protocol::socket socket_type;
    /// Type of a shared pointer to the socket being used.
    typedef lib::shared_ptr<socket_type> socket_ptr;

    explicit connection()
        : m_state(UNINITIALIZED) {
    }

    ptr
    get_shared() {
        return shared_from_this();
    }

    bool
    is_secure() const {
        return false;
    }

    void
    set_socket_init_handler(socket_init_handler h) {
        m_socket_init_handler = h;
    }

    lib::asio::local::stream_protocol::socket&
    get_socket() {
        return *m_socket;
    }

    lib::asio::local::stream_protocol::socket&
    get_next_layer() {
        return *m_socket;
    }

    lib::asio::local::stream_protocol::socket&
    get_raw_socket() {
        return *m_socket;
    }

    std::string
    get_remote_endpoint(lib::error_code& ec) const {
        return "UNIX Socket Endpoint";
    }

protected:
    lib::error_code
    init_asio(io_service_ptr service, strand_ptr, bool) {
        if(m_state != UNINITIALIZED) {
            return socket::make_error_code(socket::error::invalid_state);
        }

        m_socket = lib::make_shared<lib::asio::local::stream_protocol::socket>(
            lib::ref(*service));

        if(m_socket_init_handler) {
            m_socket_init_handler(m_hdl, *m_socket);
        }

        m_state = READY;

        return lib::error_code();
    }

    void
    set_uri(uri_ptr) {}

    void
    pre_init(init_handler callback) {
        if(m_state != READY) {
            callback(socket::make_error_code(socket::error::invalid_state));
            return;
        }

        m_state = READING;

        callback(lib::error_code());
    }

    void
    post_init(init_handler callback) {
        callback(lib::error_code());
    }

    void
    set_handle(connection_hdl hdl) {
        m_hdl = hdl;
    }

    lib::asio::error_code
    cancel_socket() {
        lib::asio::error_code ec;
        m_socket->cancel(ec);
        return ec;
    }

    void
    async_shutdown(socket::shutdown_handler h) {
        lib::asio::error_code ec;
        m_socket->shutdown(lib::asio::ip::tcp::socket::shutdown_both, ec);
        h(ec);
    }

    lib::error_code
    get_ec() const {
        return lib::error_code();
    }

public:
    template <typename ErrorCodeType>
    static lib::error_code
    translate_ec(ErrorCodeType) {
        return make_error_code(transport::error::pass_through);
    }

    static lib::error_code
    translate_ec(lib::error_code ec) {
        return ec;
    }

private:
    enum state {
        UNINITIALIZED = 0,
        READY         = 1,
        READING       = 2
    };

    socket_ptr m_socket;
    state      m_state;

    connection_hdl      m_hdl;
    socket_init_handler m_socket_init_handler;
};

class endpoint {
public:
    /// The type of this endpoint socket component
    typedef endpoint type;

    /// The type of the corresponding connection socket component
    typedef connection socket_con_type;
    /// The type of a shared pointer to the corresponding connection socket
    /// component.
    typedef socket_con_type::ptr socket_con_ptr;

    explicit endpoint() {}

    bool
    is_secure() const {
        return false;
    }

    void
    set_socket_init_handler(socket_init_handler h) {
        m_socket_init_handler = h;
    }

protected:
    lib::error_code
    init(socket_con_ptr scon) {
        scon->set_socket_init_handler(m_socket_init_handler);
        return lib::error_code();
    }

private:
    socket_init_handler m_socket_init_handler;
};

}  // namespace local_socket

template <typename config>
class local_endpoint : public config::socket_type {
public:
    /// Type of this endpoint transport component
    typedef local_endpoint<config> type;

    /// Type of the concurrency policy
    typedef typename config::concurrency_type concurrency_type;
    /// Type of the socket policy
    typedef typename config::socket_type socket_type;
    /// Type of the error logging policy
    typedef typename config::elog_type elog_type;
    /// Type of the access logging policy
    typedef typename config::alog_type alog_type;

    /// Type of the socket connection component
    typedef typename socket_type::socket_con_type socket_con_type;
    /// Type of a shared pointer to the socket connection component
    typedef typename socket_con_type::ptr socket_con_ptr;

    /// Type of the connection transport component associated with this
    /// endpoint transport component
    typedef asio::connection<config> transport_con_type;
    /// Type of a shared pointer to the connection transport component
    /// associated with this endpoint transport component
    typedef typename transport_con_type::ptr transport_con_ptr;

    /// Type of a pointer to the ASIO io_service being used
    typedef lib::asio::io_service* io_service_ptr;
    /// Type of a shared pointer to the acceptor being used
    typedef lib::shared_ptr<lib::asio::local::stream_protocol::acceptor> acceptor_ptr;
    /// Type of timer handle
    typedef lib::shared_ptr<lib::asio::steady_timer> timer_ptr;
    /// Type of a shared pointer to an io_service work object
    typedef lib::shared_ptr<lib::asio::io_service::work> work_ptr;

    // generate and manage our own io_service
    explicit local_endpoint()
        : m_io_service(NULL)
        , m_external_io_service(false)
        , m_listen_backlog(lib::asio::socket_base::max_connections)
        , m_state(UNINITIALIZED) {
        //std::cout << "transport::asio::local_endpoint constructor" << std::endl;
    }

    ~local_endpoint() {
        if(m_acceptor && m_state == LISTENING) {
            ::unlink(m_acceptor->local_endpoint().path().c_str());
        }

        // clean up our io_service if we were initialized with an internal one.

        // Explicitly destroy local objects
        m_acceptor.reset();
        m_work.reset();
        if(m_state != UNINITIALIZED && !m_external_io_service) {
            delete m_io_service;
        }
    }

    /// transport::asio objects are moveable but not copyable or assignable.
    /// The following code sets this situation up based on whether or not we
    /// have C++11 support or not
#ifdef _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_
    local_endpoint(const local_endpoint& src) = delete;
    local_endpoint& operator=(const local_endpoint& rhs) = delete;
#else
private:
    local_endpoint(const local_endpoint& src);
    local_endpoint& operator=(const local_endpoint& rhs);

public:
#endif  // _WEBSOCKETPP_DEFAULT_DELETE_FUNCTIONS_

#ifdef _WEBSOCKETPP_MOVE_SEMANTICS_
    local_endpoint(local_endpoint&& src)
        : config::socket_type(std::move(src))
        , m_io_service(src.m_io_service)
        , m_external_io_service(src.m_external_io_service)
        , m_acceptor(src.m_acceptor)
        , m_listen_backlog(lib::asio::socket_base::max_connections)
        , m_elog(src.m_elog)
        , m_alog(src.m_alog)
        , m_state(src.m_state) {
        src.m_io_service          = NULL;
        src.m_external_io_service = false;
        src.m_acceptor            = NULL;
        src.m_state               = UNINITIALIZED;
    }
#endif  // _WEBSOCKETPP_MOVE_SEMANTICS_

    bool
    is_secure() const {
        return socket_type::is_secure();
    }

    void
    init_asio(io_service_ptr ptr, lib::error_code& ec) {
        if(m_state != UNINITIALIZED) {
            m_elog->write(log::elevel::library,
                          "asio::init_asio called from the wrong state");
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::invalid_state);
            return;
        }

        m_alog->write(log::alevel::devel, "asio::init_asio");

        m_io_service          = ptr;
        m_external_io_service = true;
        m_acceptor            = lib::make_shared<lib::asio::local::stream_protocol::acceptor>(lib::ref(*m_io_service));

        m_state = READY;
        ec      = lib::error_code();
    }

    void
    init_asio(io_service_ptr ptr) {
        lib::error_code ec;
        init_asio(ptr, ec);
        if(ec) {
            throw exception(ec);
        }
    }

    void
    init_asio(lib::error_code& ec) {
        // Use a smart pointer until the call is successful and ownership has
        // successfully been taken. Use unique_ptr when available.
        // TODO: remove the use of auto_ptr when C++98/03 support is no longer
        //       necessary.
#ifdef _WEBSOCKETPP_CPP11_MEMORY_
        lib::unique_ptr<lib::asio::io_service> service(new lib::asio::io_service());
#else
        lib::auto_ptr<lib::asio::io_service> service(new lib::asio::io_service());
#endif
        init_asio(service.get(), ec);
        if(!ec)
            service.release();  // Call was successful, transfer ownership
        m_external_io_service = false;
    }

    void
    init_asio() {
        // Use a smart pointer until the call is successful and ownership has
        // successfully been taken. Use unique_ptr when available.
        // TODO: remove the use of auto_ptr when C++98/03 support is no longer
        //       necessary.
#ifdef _WEBSOCKETPP_CPP11_MEMORY_
        lib::unique_ptr<lib::asio::io_service> service(new lib::asio::io_service());
#else
        lib::auto_ptr<lib::asio::io_service> service(new lib::asio::io_service());
#endif
        init_asio(service.get());
        // If control got this far without an exception, then ownership has successfully been taken
        service.release();
        m_external_io_service = false;
    }

    void
    set_listen_backlog(int backlog) {
        m_listen_backlog = backlog;
    }

    lib::asio::io_service&
    get_io_service() {
        return *m_io_service;
    }

    lib::asio::local::stream_protocol::endpoint
    get_local_endpoint(lib::asio::error_code& ec) {
        if(m_acceptor) {
            return m_acceptor->local_endpoint(ec);
        }
        else {
            ec = lib::asio::error::make_error_code(lib::asio::error::bad_descriptor);
            return lib::asio::local::stream_protocol::endpoint();
        }
    }

    void
    listen(lib::asio::local::stream_protocol::endpoint const& ep, lib::error_code& ec) {
        if(m_state != READY) {
            m_elog->write(log::elevel::library,
                          "asio::listen called from the wrong state");
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::invalid_state);
            return;
        }

        m_alog->write(log::alevel::devel, "asio::listen");

        lib::asio::error_code bec;

        {
            boost::system::error_code                 test_ec;
            lib::asio::local::stream_protocol::socket test_socket(get_io_service());
            test_socket.connect(ep, test_ec);

            //looks like a service is already running on that socket, probably another keosd, don't touch it
            if(test_ec == boost::system::errc::success) {
                bec = boost::system::errc::make_error_code(boost::system::errc::address_in_use);
            }
            //socket exists but no one home, go ahead and remove it and continue on
            else if(test_ec == boost::system::errc::connection_refused) {
                ::unlink(ep.path().c_str());
            }
            else if(test_ec != boost::system::errc::no_such_file_or_directory) {
                bec = test_ec;
            }
        }

        if(!bec) {
            m_acceptor->open(ep.protocol(), bec);
        }
        if(!bec) {
            m_acceptor->bind(ep, bec);
        }
        if(!bec) {
            m_acceptor->listen(m_listen_backlog, bec);
        }
        if(bec) {
            if(m_acceptor->is_open()) {
                m_acceptor->close();
            }
            log_err(log::elevel::info, "asio listen", bec);
            ec = bec;
        }
        else {
            m_state = LISTENING;
            ec      = lib::error_code();
        }
    }

    void
    listen(lib::asio::local::stream_protocol::endpoint const& ep) {
        lib::error_code ec;
        listen(ep, ec);
        if(ec) {
            throw exception(ec);
        }
    }

    void
    stop_listening(lib::error_code& ec) {
        if(m_state != LISTENING) {
            m_elog->write(log::elevel::library,
                          "asio::listen called from the wrong state");
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::invalid_state);
            return;
        }

        ::unlink(m_acceptor->local_endpoint().path().c_str());
        m_acceptor->close();
        m_state = READY;
        ec      = lib::error_code();
    }

    void
    stop_listening() {
        lib::error_code ec;
        stop_listening(ec);
        if(ec) {
            throw exception(ec);
        }
    }

    bool
    is_listening() const {
        return (m_state == LISTENING);
    }

    std::size_t
    run() {
        return m_io_service->run();
    }

    std::size_t
    run_one() {
        return m_io_service->run_one();
    }

    void
    stop() {
        m_io_service->stop();
    }

    std::size_t
    poll() {
        return m_io_service->poll();
    }

    std::size_t
    poll_one() {
        return m_io_service->poll_one();
    }

    void
    reset() {
        m_io_service->reset();
    }

    bool
    stopped() const {
        return m_io_service->stopped();
    }

    void
    start_perpetual() {
        m_work = lib::make_shared<lib::asio::io_service::work>(
            lib::ref(*m_io_service));
    }

    void
    stop_perpetual() {
        m_work.reset();
    }

    timer_ptr
    set_timer(long duration, timer_handler callback) {
        timer_ptr new_timer = lib::make_shared<lib::asio::steady_timer>(
            *m_io_service,
            lib::asio::milliseconds(duration));

        new_timer->async_wait(
            lib::bind(
                &type::handle_timer,
                this,
                new_timer,
                callback,
                lib::placeholders::_1));

        return new_timer;
    }

    void
    handle_timer(timer_ptr, timer_handler callback, lib::asio::error_code const& ec) {
        if(ec) {
            if(ec == lib::asio::error::operation_aborted) {
                callback(make_error_code(transport::error::operation_aborted));
            }
            else {
                m_elog->write(log::elevel::info,
                              "asio handle_timer error: " + ec.message());
                log_err(log::elevel::info, "asio handle_timer", ec);
                callback(socket_con_type::translate_ec(ec));
            }
        }
        else {
            callback(lib::error_code());
        }
    }

    void
    async_accept(transport_con_ptr tcon, accept_handler callback, lib::error_code& ec) {
        if(m_state != LISTENING || !m_acceptor) {
            using websocketpp::error::make_error_code;
            ec = make_error_code(websocketpp::error::async_accept_not_listening);
            return;
        }

        m_alog->write(log::alevel::devel, "asio::async_accept");

        if(config::enable_multithreading) {
            m_acceptor->async_accept(
                tcon->get_raw_socket(),
                tcon->get_strand()->wrap(lib::bind(
                    &type::handle_accept,
                    this,
                    callback,
                    lib::placeholders::_1)));
        }
        else {
            m_acceptor->async_accept(
                tcon->get_raw_socket(),
                lib::bind(
                    &type::handle_accept,
                    this,
                    callback,
                    lib::placeholders::_1));
        }
    }

    void
    async_accept(transport_con_ptr tcon, accept_handler callback) {
        lib::error_code ec;
        async_accept(tcon, callback, ec);
        if(ec) {
            throw exception(ec);
        }
    }

protected:
    void
    init_logging(const lib::shared_ptr<alog_type>& a, const lib::shared_ptr<elog_type>& e) {
        m_alog = a;
        m_elog = e;
    }

    void
    handle_accept(accept_handler callback, lib::asio::error_code const& asio_ec) {
        lib::error_code ret_ec;

        m_alog->write(log::alevel::devel, "asio::handle_accept");

        if(asio_ec) {
            if(asio_ec == lib::asio::errc::operation_canceled) {
                ret_ec = make_error_code(websocketpp::error::operation_canceled);
            }
            else {
                log_err(log::elevel::info, "asio handle_accept", asio_ec);
                ret_ec = socket_con_type::translate_ec(asio_ec);
            }
        }

        callback(ret_ec);
    }

    void
    handle_connect_timeout(transport_con_ptr tcon, timer_ptr,
                           connect_handler callback, lib::error_code const& ec) {
        lib::error_code ret_ec;

        if(ec) {
            if(ec == transport::error::operation_aborted) {
                m_alog->write(log::alevel::devel,
                              "asio handle_connect_timeout timer cancelled");
                return;
            }

            log_err(log::elevel::devel, "asio handle_connect_timeout", ec);
            ret_ec = ec;
        }
        else {
            ret_ec = make_error_code(transport::error::timeout);
        }

        m_alog->write(log::alevel::devel, "TCP connect timed out");
        tcon->cancel_socket_checked();
        callback(ret_ec);
    }

    void
    handle_connect(transport_con_ptr tcon, timer_ptr con_timer,
                   connect_handler callback, lib::asio::error_code const& ec) {
        if(ec == lib::asio::error::operation_aborted || lib::asio::is_neg(con_timer->expires_from_now())) {
            m_alog->write(log::alevel::devel, "async_connect cancelled");
            return;
        }

        con_timer->cancel();

        if(ec) {
            log_err(log::elevel::info, "asio async_connect", ec);
            callback(socket_con_type::translate_ec(ec));
            return;
        }

        if(m_alog->static_test(log::alevel::devel)) {
            m_alog->write(log::alevel::devel,
                          "Async connect to " + tcon->get_remote_endpoint() + " successful.");
        }

        callback(lib::error_code());
    }

    lib::error_code
    init(transport_con_ptr tcon) {
        m_alog->write(log::alevel::devel, "transport::asio::init");

        // Initialize the connection socket component
        socket_type::init(lib::static_pointer_cast<socket_con_type,
                                                   transport_con_type>(tcon));

        lib::error_code ec;

        ec = tcon->init_asio(m_io_service);
        if(ec) {
            return ec;
        }

        return lib::error_code();
    }

private:
    /// Convenience method for logging the code and message for an error_code
    template <typename error_type>
    void
    log_err(log::level l, char const* msg, error_type const& ec) {
        std::stringstream s;
        s << msg << " error: " << ec << " (" << ec.message() << ")";
        m_elog->write(l, s.str());
    }

    /// Helper for cleaning up in the listen method after an error
    template <typename error_type>
    lib::error_code
    clean_up_listen_after_error(error_type const& ec) {
        if(m_acceptor->is_open()) {
            m_acceptor->close();
        }
        log_err(log::elevel::info, "asio listen", ec);
        return socket_con_type::translate_ec(ec);
    }

    enum state {
        UNINITIALIZED = 0,
        READY         = 1,
        LISTENING     = 2
    };

    // Network Resources
    io_service_ptr m_io_service;
    bool           m_external_io_service;
    acceptor_ptr   m_acceptor;
    work_ptr       m_work;

    // Network constants
    int m_listen_backlog;

    lib::shared_ptr<elog_type> m_elog;
    lib::shared_ptr<alog_type> m_alog;

    // Transport state
    state m_state;
};

}}}  // namespace websocketpp::transport::asio
