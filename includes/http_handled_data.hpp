#pragma once
#include <string>
#include <map>
namespace hamza_http
{
    /**
     * @brief Struct representing the result of handling an HTTP message.
     */
    struct http_handled_data
    {
        bool completed;                             ///< Indicates if the request was fully processed
        std::string method;                         ///< HTTP method (e.g., GET, POST)
        std::string uri;                            ///< Request URI
        std::string version;                        ///< HTTP version (e.g., "HTTP/1.1")
        std::multimap<std::string, std::string> headers; ///< Request headers
        std::string body;                           ///< Request body

        http_handled_data(bool completed = false, const std::string &method = "",
                          const std::string &uri = "", const std::string &version = "",
                          const std::multimap<std::string, std::string> &headers = {},
                          const std::string &body = "")
            : completed(completed), method(method), uri(uri), version(version), headers(headers), body(body) {}
    };
}
