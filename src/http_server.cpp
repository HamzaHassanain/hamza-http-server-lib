#include <http_server.hpp>
#include <iostream>
#include <sstream>
#include <chrono>

namespace hamza_http
{
    /**
     * Construct HTTP server using base TCP server infrastructure.
     * Delegates socket creation, binding, and listening to parent class.
     * HTTP-specific functionality is added through callback overrides.
     */
    http_server::http_server(const hamza::socket_address &addr) : hamza::tcp_server(addr) {}

    /**
     * Parse complete HTTP request and invoke user-defined request handler.
     * Implements HTTP/1.1 request parsing including method, URI, headers, and body.
     * Creates request/response objects and provides connection management functions.
     * Automatically closes connection for empty messages (client disconnect).
     */

    void http_server::on_message_received(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message)
    {
        // std::cout << "client: " << sock_ptr->get_remote_address().to_string() << std::endl;
        auto [completed, method, uri, version, headers, body] = handler.handle(sock_ptr, message);

        if (!completed)
            return;

        auto close_connection_for_objects = [this](std::shared_ptr<hamza::socket> client_socket)
        {
            this->close_connection(client_socket);
        };

        // Create HTTP request object with parsed data
        http_request request(method, uri, version, headers, body, sock_ptr);

        // Create HTTP response object with default HTTP/1.1 version
        http_response response("HTTP/1.1", {}, sock_ptr);
        // Provide connection closure capability to both objects
        request.close_connection = close_connection_for_objects;
        response.close_connection = close_connection_for_objects;

        // Invoke user-defined request handler with parsed request and response objects
        // User callback populates response and optionally closes connection
        std::cout << sock_ptr->get_remote_address().to_string() << " " << body.size() << std::endl;
        this->on_request_received(request, response);
    }

    void http_server::on_request_received(http_request &request, http_response &response)
    {
        if (request_callback)
        {
            request_callback(request, response);
        }
    }

    /**
     * Handle server startup completion event.
     * Calls user-provided callback to notify application that server is listening.
     * Useful for logging startup messages or performing initialization tasks.
     */
    void http_server::on_server_listen()
    {
        if (listen_success_callback)
            listen_success_callback();
    }

    /**
     * Handle server shutdown completion event.
     * Calls user-provided callback to notify application that server has stopped.
     * Useful for cleanup tasks, final logging, or resource deallocation.
     */
    void http_server::on_server_stopped()
    {
        if (server_stopped_callback)
            server_stopped_callback();
    }

    /**
     * Handle server exceptions and forward to user error handler.
     * Provides centralized error handling for all server and network errors.
     * User callback can implement logging, recovery, or graceful degradation.
     */
    void http_server::on_exception(std::shared_ptr<hamza::socket_exception> e)
    {
        if (error_callback)
            error_callback(e);
    }

    /**
     * Handle client disconnection events.
     */
    void http_server::on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr)
    {
        if (client_disconnected_callback)
            client_disconnected_callback(sock_ptr);
    }

    /**
     * Handle new client connection events.
     */
    void http_server::on_client_connected(std::shared_ptr<hamza::socket> sock_ptr)
    {
        if (client_connected_callback)
            client_connected_callback(sock_ptr);
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
        server_stopped_callback = callback;
    }

    /**
     * Set callback function for error handling.
     * Receives all server and network exceptions for centralized error management.
     */
    void http_server::set_error_callback(std::function<void(std::shared_ptr<hamza::socket_exception>)> callback)
    {
        error_callback = callback;
    }

    /**
     * Set callback function for new client connections.
     */
    void http_server::set_client_connected_callback(std::function<void(std::shared_ptr<hamza::socket>)> callback)
    {
        client_connected_callback = callback;
    }

    /**
     * Set callback function for client disconnections.
     */
    void http_server::set_client_disconnected_callback(std::function<void(std::shared_ptr<hamza::socket>)> callback)
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