#pragma once

#include <string>

namespace hamza_http
{

    // HTTP Version Constants
    constexpr const char *HTTP_VERSION_1_0 = "HTTP/1.0";
    constexpr const char *HTTP_VERSION_1_1 = "HTTP/1.1";

    // HTTP Status Codes (commonly used)
    constexpr int HTTP_OK = 200;
    constexpr int HTTP_CREATED = 201;
    constexpr int HTTP_NO_CONTENT = 204;
    constexpr int HTTP_BAD_REQUEST = 400;
    constexpr int HTTP_UNAUTHORIZED = 401;
    constexpr int HTTP_FORBIDDEN = 403;
    constexpr int HTTP_NOT_FOUND = 404;
    constexpr int HTTP_INTERNAL_SERVER_ERROR = 500;

    // HTTP Methods
    constexpr const char *HTTP_GET = "GET";
    constexpr const char *HTTP_POST = "POST";
    constexpr const char *HTTP_PUT = "PUT";
    constexpr const char *HTTP_DELETE = "DELETE";
    constexpr const char *HTTP_HEAD = "HEAD";
    constexpr const char *HTTP_OPTIONS = "OPTIONS";
    constexpr const char *HTTP_PATCH = "PATCH";

    // HTTP Headers (commonly used)
    constexpr const char *HEADER_CONTENT_TYPE = "Content-Type";
    constexpr const char *HEADER_CONTENT_LENGTH = "Content-Length";
    constexpr const char *HEADER_CONNECTION = "Connection";
    constexpr const char *HEADER_HOST = "Host";
    constexpr const char *HEADER_USER_AGENT = "User-Agent";
    constexpr const char *HEADER_ACCEPT = "Accept";
    constexpr const char *HEADER_AUTHORIZATION = "Authorization";
    constexpr

    // HTTP Line Endings
    constexpr const char *CRLF = "\r\n";
    constexpr const char *DOUBLE_CRLF = "\r\n\r\n";
}