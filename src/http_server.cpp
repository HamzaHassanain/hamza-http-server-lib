
#include <iostream>
#include <sstream>
#include <chrono>

#include "../includes/http_server.hpp"

namespace hh_http
{
    /**
     * Construct HTTP server using base TCP server infrastructure.
     * Delegates socket creation, binding, and listening to parent class.
     * HTTP-specific functionality is added through callback overrides.
     */
    http_server::http_server(const hh_socket::socket_address &addr, int timeout_milliseconds) : hh_socket::epoll_server(epoll_config::MAX_FILE_DESCRIPTORS)
    {
        this->timeout_milliseconds = timeout_milliseconds;
        this->server_socket = hh_socket::make_listener_socket(addr.get_port().get(),
                                                              addr.get_ip_address().get(),
                                                              epoll_config::BACKLOG_SIZE);
        if (!this->server_socket)
            throw std::runtime_error("Failed to create listener socket");
        this->register_listener_socket(this->server_socket);
    }

    /**
     * Parse complete HTTP request and invoke user-defined request handler.
     * Implements HTTP/1.1 request parsing including method, URI, headers, and body.
     * Creates request/response objects and provides connection management functions.
     */

    void http_server::on_message_received(std::shared_ptr<hh_socket::connection> conn, const hh_socket::data_buffer &message)
    {
        auto [completed, method, uri, version, headers, body] = handler.handle(conn, message);

        if (static_cast<int>(headers.size()) >= 0)
            on_headers_received(conn, headers, method, uri, version, body);

        if (!completed)
            return;

        auto close_connection_for_objects = [this, conn]()
        {
            this->close_connection(conn);
        };
        auto send_message_for_request = [this, conn](const std::string &message)
        {
            this->send_message(conn, hh_socket::data_buffer(message));
        };
        // Create HTTP request object with parsed data
        http_request request(method, uri, version, headers, body, close_connection_for_objects);

        // Create HTTP response object with default HTTP/1.1 version
        http_response response("HTTP/1.1", {}, close_connection_for_objects, send_message_for_request);

        // Invoke user-defined request handler with parsed request and response objects
        // User callback populates response and optionally closes connection
        this->on_request_received(request, response);
    }

    void http_server::on_request_received(http_request &request, http_response &response)
    {
        if (request_callback)
        {
            request_callback(request, response);
        }
        else
        {
            throw std::runtime_error("No request handler registered");
        }
    }

    /**
     * Handle server startup completion event.
     * Calls user-provided callback to notify application that server is listening.
     * Useful for logging startup messages or performing initialization tasks.
     */
    void http_server::on_listen_success()
    {
        if (listen_success_callback)
            listen_success_callback();
    }

    /**
     * Handle server shutdown completion event.
     * Calls user-provided callback to notify application that server has stopped.
     * Useful for cleanup tasks, final logging, or resource deallocation.
     */
    void http_server::on_shutdown_success()
    {
        if (server_shutdown_callback)
            server_shutdown_callback();
    }

    /**
     * Handle server exceptions and forward to user error handler.
     * Provides centralized error handling for all server and network errors.
     * User callback can implement logging, recovery, or graceful degradation.
     */
    void http_server::on_exception_occurred(const std::exception &e)
    {
        if (error_callback)
            error_callback(e);
    }

    /**
     * Handle client disconnection events.
     */
    void http_server::on_connection_closed(std::shared_ptr<hh_socket::connection> conn)
    {
        if (client_disconnected_callback)
            client_disconnected_callback(conn);
    }

    /**
     * Handle new client connection events.
     */
    void http_server::on_connection_opened(std::shared_ptr<hh_socket::connection> conn)
    {
        if (client_connected_callback)
            client_connected_callback(conn);
    }

    /**
     * Handle server idle periods (select timeout events).
     */
    void http_server::on_waiting_for_activity()
    {
        if (waiting_for_activity_callback)
            waiting_for_activity_callback();
    }

    /**
     * Set callback function for handling HTTP requests.
     * This is the main application logic entry point.
     * Callback receives parsed request and empty response objects.
     */
    void http_server::set_request_callback(std::function<void(http_request &, http_response &)> callback)
    {
        request_callback = callback;
    }

    /**
     * Set callback function for server startup notification.
     * Called once when server successfully starts listening.
     */
    void http_server::set_listen_success_callback(std::function<void()> callback)
    {
        listen_success_callback = callback;
    }

    /**
     * Set callback function for server shutdown notification.
     * Called once when server stops listening and exits main loop.
     */
    void http_server::set_server_stopped_callback(std::function<void()> callback)
    {
        server_shutdown_callback = callback;
    }

    /**
     * Set callback function for error handling.
     * Receives all server and network exceptions for centralized error management.
     */
    void http_server::set_error_callback(std::function<void(const std::exception &)> callback)
    {
        error_callback = callback;
    }

    /**
     * Set callback function for new client connections.
     */
    void http_server::set_client_connected_callback(std::function<void(std::shared_ptr<hh_socket::connection>)> callback)
    {
        client_connected_callback = callback;
    }

    /**
     * Set callback function for client disconnections.
     */
    void http_server::set_client_disconnected_callback(std::function<void(std::shared_ptr<hh_socket::connection>)> callback)
    {
        client_disconnected_callback = callback;
    }

    /**
     * Set callback function for server idle periods.
     */
    void http_server::set_waiting_for_activity_callback(std::function<void()> callback)
    {
        // BUG: Should be waiting_for_activity_callback = callback;
        waiting_for_activity_callback = callback;
    }
}