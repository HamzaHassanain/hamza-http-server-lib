#include <http_server.hpp>
#include <iostream>
#include <sstream>

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
        // Handle client disconnect (empty message indicates connection closed)
        if (message.empty())
        {
            close_connection(sock_ptr);
            return;
        }

        // Ensure request callback is configured before processing requests
        if (!request_callback)
        {
            throw std::runtime_error("Request callback is not set.");
            exit(1); // Fatal error - server cannot function without request handler
            return;
        }

        // Convert raw message to string stream for line-by-line parsing
        std::istringstream request_stream(message.to_string());
        std::ostringstream body_stream;

        // HTTP request components to be parsed
        std::string method, uri, version, body;
        std::multimap<std::string, std::string> headers; // Allows duplicate header names
        std::string line;

        // Parse HTTP request line: "METHOD URI VERSION"
        // Example: "GET /index.html HTTP/1.1"
        if (std::getline(request_stream, line) && !line.empty())
        {
            // Remove carriage return from line ending (CRLF -> LF)
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            // Split request line into method, URI, and version
            std::istringstream request_line(line);
            request_line >> method >> uri >> version;
        }

        // Parse HTTP headers until empty line is encountered
        while (std::getline(request_stream, line))
        {
            // Remove carriage return from each header line
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            // Empty line indicates end of headers and start of body
            if (line.empty())
            {
                break;
            }

            // Parse header in format "Name: Value"
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string header_name = line.substr(0, colon_pos);
                std::string header_value = line.substr(colon_pos + 1);

                // Trim leading whitespace from header value
                size_t start = header_value.find_first_not_of(" \t");
                if (start != std::string::npos)
                {
                    header_value = header_value.substr(start);
                }

                // Trim trailing whitespace from header value
                size_t end = header_value.find_last_not_of(" \t");
                if (end != std::string::npos)
                {
                    header_value = header_value.substr(0, end + 1);
                }

                // Store header (multimap allows duplicate header names)
                headers.emplace(header_name, header_value);
            }
        }

        // Handle Content-Length for requests with body (POST, PUT, etc.)
        int content_length = 0;
        auto content_length_it = headers.find("Content-Length");
        if (content_length_it != headers.end())
        {
            content_length = std::stoi(content_length_it->second);
        }

        // Read the request body (remaining content after headers)
        body_stream << request_stream.rdbuf();
        body = body_stream.str();

        // Validate Content-Length matches actual body size
        // Prevents incomplete requests or buffer overflow attacks
        if (content_length < static_cast<int>(body.size()))
        {
            throw std::runtime_error("Content-Length mismatch: expected " + std::to_string(content_length) +
                                     " but received " + std::to_string(body.size()));
        }

        // Create lambda for connection closure (captures 'this' pointer)
        // Allows request/response objects to close connection when needed
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

        // Set default connection header (HTTP/1.1 with connection close)
        // Simplifies connection management by closing after each response
        response.add_header("Connection", "close");

        // Invoke user-defined request handler with parsed request and response objects
        // User callback populates response and optionally closes connection
        request_callback(request, response);
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
     * FIXME: Bug - callback name collision with member variable.
     * Should call client_disconnected_callback, not recursive call.
     */
    void http_server::on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // BUG: This creates infinite recursion - should be client_disconnected_callback
        if (client_disconnected_callback)
            client_disconnected_callback(sock_ptr);
    }

    /**
     * Handle new client connection events.
     * FIXME: Bug - callback name collision with member variable.
     * Should call client_connected_callback, not recursive call.
     */
    void http_server::on_new_client_connected(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // BUG: This creates infinite recursion - should be client_connected_callback
        if (client_connected_callback)
            client_connected_callback(sock_ptr);
    }

    /**
     * Handle server idle periods (select timeout events).
     * FIXME: Bug - callback name collision with member variable.
     * Should call waiting_for_activity_callback, not recursive call.
     */
    void http_server::on_waiting_for_activity()
    {
        // BUG: This creates infinite recursion - should be waiting_for_activity_callback
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