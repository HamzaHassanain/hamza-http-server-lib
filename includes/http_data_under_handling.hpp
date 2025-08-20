
#pragma once

#include <string>
#include <map>

namespace hamza_http
{
    enum class handling_type
    {
        CONTENT_LENGTH, ///< Content-Length based handling
        CHUNKED         ///< Chunked Transfer-Encoding based handling
    };
    /**
     * @brief Struct representing the result of handling an HTTP message.
     *  Small POD that stores parsing state for requests that arrive across
     * multiple TCP segments. Stored in http_message_handler::under_handling_data
     * and used to accumulate headers/body until the request is complete.
     *
     * @note:
     *  - socket_key: identifies client (remote address string)
     *  - type: parsing strategy (CONTENT_LENGTH or CHUNKED)
     *  - content_length: expected body size for CONTENT_LENGTH mode
     */
    struct http_data_under_handling
    {
        std::string socket_key; // key
        handling_type type;     ///< to know if we handle CONTENT_LENGTH or CHUNKED
        std::size_t content_length;
        std::string method;                              ///< HTTP method (e.g., GET, POST)
        std::string uri;                                 ///< Request URI
        std::string version;                             ///< HTTP version (e.g., "HTTP/1.1")
        std::multimap<std::string, std::string> headers; ///< Request headers
        std::string body;                                ///< Request body
        http_data_under_handling() = default;
        http_data_under_handling(const std::string &socket_key, handling_type type) : socket_key(socket_key), type(type) {}

        bool operator<(const http_data_under_handling &other) const
        {
            return socket_key < other.socket_key;
        }
    };
}
