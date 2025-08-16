#pragma once

#include <string>

namespace hamza_http
{

    namespace headers
    {
        const std::string CONTENT_TYPE = "Content-Type";
        const std::string CONTENT_LENGTH = "Content-Length";
        const std::string CONNECTION = "Connection";
        const std::string ACCEPT = "Accept";
        const std::string USER_AGENT = "User-Agent";
        const std::string HOST = "Host";
        const std::string REFERER = "Referer";
        const std::string COOKIE = "Cookie";
        const std::string AUTHORIZATION = "Authorization";
        const std::string IF_MODIFIED_SINCE = "If-Modified-Since";
        const std::string IF_NONE_MATCH = "If-None-Match";
        const std::string EXPECT = "Expect";

    }
    namespace versions
    {
        const std::string HTTP_1_1 = "HTTP/1.1";
        const std::string HTTP_2_0 = "HTTP/2.0";
    }
}