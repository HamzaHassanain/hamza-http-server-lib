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
     */
    struct http_data_under_handling
    {
        std::string socket_key; // key
        handling_type type;
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
