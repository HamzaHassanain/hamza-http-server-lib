#pragma once

#include <socket.hpp>
#include <http_consts.hpp>
#include <map>
#include <functional>

namespace hamza_http
{
    /**
     * @brief Represents an HTTP request with move-only semantics.
     *
     * This class encapsulates an HTTP request including method, URI, version,
     * headers, and body. It provides a safe interface for accessing request
     * data while maintaining ownership of the associated client socket.
     *
     * The class implements move-only semantics to ensure unique ownership
     * of the underlying socket resource and prevent accidental duplication
     * of request objects.
     */
    class http_request
    {
    private:
        /// HTTP method (GET, POST, PUT, DELETE, etc.)
        std::string method;

        /// Request URI/path
        std::string uri;

        /// HTTP version (e.g., "HTTP/1.1")
        std::string version;

        /// HTTP headers (multimap allows multiple values per header name)
        std::multimap<std::string, std::string> headers;

        /// Request body content
        std::string body;

        /// Function to close the connection when needed (closes the current client only, it shall know what to close)
        std::function<void()> close_connection;

        /**
         * @brief Private constructor for internal use by http_server.
         * @param method HTTP method
         * @param uri Request URI
         * @param version HTTP version
         * @param headers Request headers
         * @param body Request body
         * @param close_connection Function to close the associated connection
         *
         * This constructor is private and can only be called by the http_server
         * class to ensure proper request object creation and lifecycle management.
         */
        http_request(const std::string &method, const std::string &uri, const std::string &version,
                     const std::multimap<std::string, std::string> &headers,
                     const std::string &body, std::function<void()> close_connection);

    public:
        // Copy operations - DELETED for resource safety
        /**
         * @brief Copy constructor - DELETED.
         *
         * Copy construction is disabled to prevent duplication of socket
         * resources and ensure unique ownership of the client connection.
         */
        http_request(const http_request &) = delete;

        /**
         * @brief Copy assignment - DELETED.
         *
         * Copy assignment is disabled to maintain unique ownership semantics.
         */
        http_request &operator=(const http_request &) = delete;

        /**
         * @brief Move assignment - DELETED.
         *
         * Move assignment is disabled to prevent reassignment after construction.
         * Use move construction instead for transferring ownership.
         */
        http_request &operator=(http_request &&) = delete;

        // Move construction - ENABLED for ownership transfer
        /**
         * @brief Move constructor.
         * @param other Request object to move from
         *
         * Transfers ownership of the request data and socket connection.
         * The source object becomes invalid after the move.
         */
        http_request(http_request &&other);

        /// Allow http_server to access private constructor
        friend class http_server;

        /**
         * @brief Safely destroy the request and close connection.
         * @param Isure Confirmation parameter to prevent accidental calls
         *
         * Explicitly destroys the request object and closes the associated
         * client connection. The boolean parameter serves as a safety check
         * to prevent accidental destruction.
         *
         * @warning This method should not be called if the response also will be needed, as it closes the connection completely.
         */
        void destroy(bool Isure);

        /**
         * @brief Get the HTTP method.
         */
        std::string get_method() const;

        /**
         * @brief Get the request URI.
         */
        std::string get_uri() const;

        /**
         * @brief Get the HTTP version.
         */
        std::string get_version() const;

        /**
         * @brief Get all values for a specific header.
         */
        std::vector<std::string> get_header(const std::string &name) const;

        /**
         * @brief Get all headers as name-value pairs.
         */
        std::vector<std::pair<std::string, std::string>> get_headers() const;

        /**
         * @brief Get the request body.
         */
        std::string get_body() const;

        /// Default destructor
        ~http_request() = default;
    };
}