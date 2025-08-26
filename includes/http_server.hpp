#pragma once

#include "../libs/socket-lib/socket-lib.hpp"

#include "http_message_handler.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "http_consts.hpp"

#include <string>

namespace hh_http
{
    /**
     * @brief High-level HTTP/1.1 server built on top of TCP server infrastructure.
     *
     * This class provides a complete HTTP server implementation that handles HTTP
     * request parsing, response generation, and connection management. It implements
     * the tcp_server interface
     *
     * The server uses a 2 architectures
     *
     * 1-   callback-driven architecture where application logic
     *      is implemented through user-provided callback functions. This allows
     *      for clean separation between HTTP protocol handling and business logic.
     *
     * 2-   Extend the http_server and override virtual methods to customize behavior.
     *
     * @note Currently implements HTTP/1.1 with "Connection: close" semantics
     * @note Supports GET, POST, and other HTTP methods through generic parsing
     * @note Thread-safe through underlying tcp_server implementation
     * @note Move-only design prevents accidental copying of server resources
     */
    class http_server : public hh_socket::epoll_server
    {
    private:
        /// The HTTP message handler instance, that parses the message, makes sure we have a valid HTTP request
        http_message_handler handler;

        /// Timeout for client connections
        int timeout_milliseconds;

        /// Shared pointer to the server socket
        std::shared_ptr<hh_socket::socket> server_socket;

        /// Callback for handling HTTP requests and generating responses
        std::function<void(http_request &, http_response &)> request_callback;

        /// Callback for handling server and network errors
        std::function<void(const std::exception &)> error_callback;

        /// Callback triggered when new client connects
        std::function<void(std::shared_ptr<hh_socket::connection>)> client_connected_callback;

        /// Callback triggered when client disconnects
        std::function<void(std::shared_ptr<hh_socket::connection>)> client_disconnected_callback;

        /// Callback triggered when server starts listening successfully
        std::function<void()> listen_success_callback;

        /// Callback triggered when server stops
        std::function<void()> server_shutdown_callback;

        /// Callback triggered during server idle periods (select timeout)
        std::function<void()> waiting_for_activity_callback;

        /// Callback triggered when HTTP headers are received
        std::function<void(std::shared_ptr<hh_socket::connection>, const std::multimap<std::string, std::string> &,
                           const std::string &, const std::string &, const std::string &, const std::string &)>
            headers_received_callback;

    protected:
        /**
         * @brief Parse HTTP request and invoke user callback.
         * @param conn Client connection that sent the request
         * @param message Raw HTTP request data
         * @throws std::runtime_error if request callback is not set
         * @throws std::runtime_error for Content-Length validation errors
         * @note Automatically closes connection on empty messages
         * @note Handles HTTP/1.1 request parsing including headers and body
         * @note calles on_request_received() for further processing
         */
        void on_message_received(std::shared_ptr<hh_socket::connection> conn, const hh_socket::data_buffer &message) override;

        /**
         * @brief Handle server startup completion.
         * @note Calls user-provided listen success callback if set
         */
        virtual void on_listen_success() override;

        /**
         * @brief Handle server shutdown completion.
         * @note Calls user-provided server stopped callback if set
         */
        virtual void on_shutdown_success() override;

        /**
         * @brief Handle server exceptions and errors.
         * @param e Exception that occurred
         * @note Forwards to user-provided error callback if set
         */
        virtual void on_exception_occurred(const std::exception &e) override;

        /**
         * @brief Handle client disconnection events.
         * @param conn Client connection that was opened
         * @note Calls user-provided client connection opened callback if set
         */
        virtual void on_connection_opened(std::shared_ptr<hh_socket::connection> conn) override;

        /**
         * @brief Handle new client connection events.
         * @param conn the connection that will be closed
         * @note Calls user-provided client connection closed callback if set
         */
        virtual void on_connection_closed(std::shared_ptr<hh_socket::connection> conn) override;

        /**
         * @brief Handle server idle periods
         * @note Calls user-provided waiting callback if set
         * @note Useful for periodic maintenance tasks
         */
        virtual void on_waiting_for_activity() override;

        /**
         * @brief Handle HTTP request processing.
         * @param request Parsed HTTP request object
         * @param response HTTP response object to populate
         * @note Calls user-provided request callback if set
         */
        virtual void on_request_received(http_request &request, http_response &response);

        /**
         * @brief Handle HTTP headers received from the client.
         * @note this function is called when HTTP headers are received, it can be used to process headers before the body is received
         * @param conn Client connection that sent the headers
         * @param headers Parsed HTTP headers
         * @param method HTTP method (GET, POST, etc.)
         * @param uri Requested URI
         * @param version HTTP version (e.g., "HTTP/1.1")
         * @param body Request body (if any)
         */
        virtual void on_headers_received(std::shared_ptr<hh_socket::connection> conn,
                                         const std::multimap<std::string, std::string> &headers,
                                         const std::string &method,
                                         const std::string &uri,
                                         const std::string &version,
                                         const std::string &body)
        {
            if (headers_received_callback)
            {
                headers_received_callback(conn, headers, method, uri, version, body);
            }
        };

    public:
        /**
         * @brief Construct HTTP server bound to specified socket address.
         * @param addr Socket address (IP and port) to bind server to
         * @param timeout_milliseconds Timeout duration in milliseconds for epoll calls
         * @throws socket_exception for socket creation, binding, or listening errors
         * @note Inherits all TCP server functionality and error handling
         */
        explicit http_server(const hh_socket::socket_address &addr, int timeout_milliseconds = epoll_config::TIMEOUT_MILLISECONDS);

        /**
         * @brief Construct HTTP server with IP address and port.
         * @param ip IP address string (e.g., "127.0.0.1", "0.0.0.0")
         * @param port Port number (1-65535)
         * @throws socket_exception for invalid IP addresses or socket errors
         * @note Convenience constructor that creates socket_address internally
         * @note Defaults to IPv4 address family
         */
        explicit http_server(int port, const std::string &ip = "0.0.0.0", int timeout_milliseconds = epoll_config::TIMEOUT_MILLISECONDS)
            : http_server(hh_socket::socket_address(hh_socket::port(port), hh_socket::ip_address(ip), hh_socket::family(hh_socket::IPV4)), timeout_milliseconds) {}

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
        void set_error_callback(std::function<void(const std::exception &)> callback);

        /**
         * @brief Set callback for new client connections.
         * @param callback Function to call when clients connect
         * @note Optional callback - can be nullptr
         * @note Useful for connection logging, rate limiting, or authentication
         */
        void set_client_connected_callback(std::function<void(std::shared_ptr<hh_socket::connection>)> callback);

        /**
         * @brief Set callback for client disconnections.
         * @param callback Function to call when clients disconnect
         * @note Optional callback - can be nullptr
         * @note Useful for cleanup, session management, or analytics
         */
        void set_client_disconnected_callback(std::function<void(std::shared_ptr<hh_socket::connection>)> callback);

        /**
         * @brief Set callback for server idle periods.
         * @param callback Function to call during select() timeouts
         * @note Optional callback - can be nullptr
         * @note Called approximately every second (based on select timeout)
         * @note Useful for periodic maintenance, statistics, or health checks
         */
        void set_waiting_for_activity_callback(std::function<void()> callback);

        /**
         * @brief Set the headers received callback object
         *
         * @param callback that is able to recive:
         *      std::shared_ptr<hh_socket::connection> conn, const std::multimap<std::string, std::string> & headers,
         *      const std::string & method, const std::string & uri, const std::string & version, const std::string & body
         */
        void set_headers_received_callback(std::function<void(std::shared_ptr<hh_socket::connection>, const std::multimap<std::string, std::string> &,
                                                              const std::string &, const std::string &, const std::string &, const std::string &)>
                                               callback)
        {
            headers_received_callback = (callback);
        }

        /**
         * @brief Start listening for incoming HTTP requests.
         * @note just calls the epoll_server::listen() method.
         */
        virtual void listen()
        {
            epoll_server::listen(timeout_milliseconds);
        }
    };
}