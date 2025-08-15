#pragma once
#include <string>
#include <vector>
#include <http_objects.hpp>
#include <web_server.hpp>
#include <http_headers.hpp>

namespace hamza::web
{
    class web_request
    {
        http::http_request request;

        std::string trim(const std::string &str) const
        {
            size_t first = str.find_first_not_of(" \t\n\r");
            size_t last = str.find_last_not_of(" \t\n\r");
            return (first == std::string::npos || last == std::string::npos) ? "" : str.substr(first, last - first + 1);
        }

    public:
        web_request(http::http_request &&req)
            : request(std::move(req))
        {
        }

        std::string get_method() const
        {
            return request.get_method();
        }

        std::string get_uri() const
        {
            return request.get_uri();
        }
        std::vector<std::pair<std::string, std::string>> get_query_parameter(const std::string &name) const
        {
            std::vector<std::pair<std::string, std::string>> result;
            // assuming URI is in the format "/path?key=value&key2=value2"
            size_t pos = request.get_uri().find('?');
            if (pos == std::string::npos)
                return result;

            std::string query = request.get_uri().substr(pos + 1);
            size_t start = 0;
            while ((start = query.find(name + '=', start)) != std::string::npos)
            {
                start += name.length() + 1;
                size_t end = query.find('&', start);
                result.emplace_back(name, query.substr(start, end - start));
                start = end + 1;
            }
            return result;
        }
        std::string get_path() const
        {
            size_t pos = request.get_uri().find('?');
            if (pos == std::string::npos)
                return request.get_uri();
            return request.get_uri().substr(0, pos);
        }

        std::string get_version() const
        {
            return request.get_version();
        }

        std::string get_header(const std::string &name) const
        {
            return request.get_header(name);
        }

        std::vector<std::pair<std::string, std::string>> get_headers() const
        {
            return request.get_headers();
        }

        std::string get_body() const
        {
            return request.get_body();
        }

        std::string get_content_type() const
        {
            return request.get_header(http::headers::CONTENT_TYPE);
        }
        std::vector<std::string> get_cookies() const
        {
            // parse cookies
            std::string cookie_header = request.get_header(http::headers::COOKIE);
            std::vector<std::string> cookies;

            if (!cookie_header.empty())
            {
                size_t start = 0;
                size_t end = 0;
                while ((end = cookie_header.find(';', start)) != std::string::npos)
                {
                    cookies.push_back(trim(cookie_header.substr(start, end - start)));
                    start = end + 1;
                }
                cookies.push_back(trim(cookie_header.substr(start)));
            }

            return cookies;
        }

        std::string get_authorization() const
        {
            return request.get_header(http::headers::AUTHORIZATION);
        }
    };
}