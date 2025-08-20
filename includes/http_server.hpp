#pragma once

#include <socket_address.hpp>
#include <http_message_handler.hpp>
#include <socket.hpp>
#include <tcp_server.hpp>
#include <http_request.hpp>
#include <http_response.hpp>
#include <string>

namespace hamza_http
{
    /**
     * @brief High-level HTTP/1.1 server built on top of TCP server infrastructure.
     *
     * This class provides a complete HTTP server implementation that handles HTTP
     * request parsing, response generation, and connection management. It extends
     * the tcp_server class
     *
     * The server uses a callback-driven architecture where application logic
     * is implemented through user-provided callback functions. This allows
     * for clean separation between HTTP protocol handling and business logic.
     *
     * @note Currently implements HTTP/1.1 with "Connection: close" semantics
     * @note Supports GET, POST, and other HTTP methods through generic parsing
     * @note Thread-safe through underlying tcp_server implementation
     * @note Move-only design prevents accidental copying of server resources
     */
    class http_server : public hamza::tcp_server
    {
    private:
        http_message_handler handler;

        /// Callback for handling HTTP requests and generating responses
        std::function<void(http_request &, http_response &)> request_callback;

        /// Callback for handling server and network errors
        std::function<void(std::shared_ptr<hamza::socket_exception>)> error_callback;

        /// Callback triggered when new client connects
        std::function<void(std::shared_ptr<hamza::socket>)> client_connected_callback;

        /// Callback triggered when client disconnects
        std::function<void(std::shared_ptr<hamza::socket>)> client_disconnected_callback;

        /// Callback triggered when server starts listening successfully
        std::function<void()> listen_success_callback;

        /// Callback triggered when server stops
        std::function<void()> server_stopped_callback;

        /// Callback triggered during server idle periods (select timeout)
        std::function<void()> waiting_for_activity_callback;

    protected:
        /**
         * @brief Parse HTTP request and invoke user callback.
         * @param sock_ptr Client socket that sent the request
         * @param message Raw HTTP request data
         * @throws std::runtime_error if request callback is not set
         * @throws std::runtime_error for Content-Length validation errors
         * @note Automatically closes connection on empty messages
         * @note Handles HTTP/1.1 request parsing including headers and body
         */
        void on_message_received(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message) override;

        /**
         * @brief Handle HTTP request processing.
         * @param request Parsed HTTP request object
         * @param response HTTP response object to populate
         */
        virtual void on_request_received(http_request &request, http_response &response);

        /**
         * @brief Handle server startup completion.
         * @note Calls user-provided listen success callback if set
         */
        virtual void on_server_listen() override;

        /**
         * @brief Handle server shutdown completion.
         * @note Calls user-provided server stopped callback if set
         */
        virtual void on_server_stopped() override;

        /**
         * @brief Handle server exceptions and errors.
         * @param e Socket exception that occurred
         * @note Forwards to user-provided error callback if set
         */
        virtual void on_exception(std::shared_ptr<hamza::socket_exception> e) override;

        /**
         * @brief Handle client disconnection events.
         * @param sock_ptr Client socket that disconnected
         * @note Calls user-provided client disconnect callback if set
         */
        virtual void on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr) override;

        /**
         * @brief Handle new client connection events.
         * @param sock_ptr Newly connected client socket
         * @note Calls user-provided client connected callback if set
         */
        virtual void on_client_connected(std::shared_ptr<hamza::socket> sock_ptr) override;

        /**
         * @brief Handle server idle periods (select timeout).
         * @note Calls user-provided waiting callback if set
         * @note Useful for periodic maintenance tasks
         */
        virtual void on_waiting_for_activity() override;

    public:
        /**
         * @brief Construct HTTP server bound to specified socket address.
         * @param addr Socket address (IP and port) to bind server to
         * @param timeout_seconds Timeout duration in seconds for select() calls
         * @param timeout_microseconds Timeout duration in microseconds for select() calls
         * @throws socket_exception for socket creation, binding, or listening errors
         * @note Inherits all TCP server functionality and error handling
         */
        explicit http_server(const hamza::socket_address &addr, int timeout_seconds = 1, int timeout_microseconds = 0);

        /**
         * @brief Construct HTTP server with IP address and port.
         * @param ip IP address string (e.g., "127.0.0.1", "0.0.0.0")
         * @param port Port number (1-65535)
         * @throws socket_exception for invalid IP addresses or socket errors
         * @note Convenience constructor that creates socket_address internally
         * @note Defaults to IPv4 address family
         */
        explicit http_server(const std::string &ip, int port, int timeout_seconds = 1, int timeout_microseconds = 0)
            : http_server(hamza::socket_address(hamza::ip_address(ip), hamza::port(port), hamza::family(hamza::IPV4)), timeout_seconds, timeout_microseconds) {}

        // Copy and move operations - DELETED for resource safety
        http_server(const http_server &) = delete;
        http_server &operator=(const http_server &) = delete;
        http_server(http_server &&) = delete;
        http_server &operator=(http_server &&) = delete;

        /**
         * @brief Set callback for handling HTTP requests.
         * @param callback Function to call for each HTTP request
         * @note Callback receives parsed http_request and http_response objects
         * @note Must be set before calling listen() - server will throw if not set
         * @note Callback should populate response object and optionally close connection
         */
        void set_request_callback(std::function<void(http_request &, http_response &)> callback);

        /**
         * @brief Set callback for server startup notification.
         * @param callback Function to call when server starts listening
         * @note Optional callback - can be nullptr
         * @note Useful for logging server startup or initialization tasks
         */
        void set_listen_success_callback(std::function<void()> callback);

        /**
         * @brief Set callback for server shutdown notification.
         * @param callback Function to call when server stops
         * @note Optional callback - can be nullptr
         * @note Useful for cleanup tasks or shutdown logging
         */
        void set_server_stopped_callback(std::function<void()> callback);

        /**
         * @brief Set callback for error handling.
         * @param callback Function to call when errors occur
         * @note Optional callback - can be nullptr
         * @note Receives socket_exception objects with error details
         * @note Should handle logging, recovery, or graceful degradation
         */
        void set_error_callback(std::function<void(std::shared_ptr<hamza::socket_exception>)> callback);

        /**
         * @brief Set callback for new client connections.
         * @param callback Function to call when clients connect
         * @note Optional callback - can be nullptr
         * @note Useful for connection logging, rate limiting, or authentication
         */
        void set_client_connected_callback(std::function<void(std::shared_ptr<hamza::socket>)> callback);

        /**
         * @brief Set callback for client disconnections.
         * @param callback Function to call when clients disconnect
         * @note Optional callback - can be nullptr
         * @note Useful for cleanup, session management, or analytics
         */
        void set_client_disconnected_callback(std::function<void(std::shared_ptr<hamza::socket>)> callback);

        /**
         * @brief Set callback for server idle periods.
         * @param callback Function to call during select() timeouts
         * @note Optional callback - can be nullptr
         * @note Called approximately every second (based on select timeout)
         * @note Useful for periodic maintenance, statistics, or health checks
         */
        void set_waiting_for_activity_callback(std::function<void()> callback);
    };
}