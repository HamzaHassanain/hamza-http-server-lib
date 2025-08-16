#pragma once
#include <string>
#include <vector>
#include <http_objects.hpp>
#include <http_headers.hpp>

namespace hamza::web
{
    class web_general_exception;

    class web_request
    {
        http::http_request request;

        std::string trim(const std::string &str) const;

    public:
        web_request(http::http_request &&req);

        std::vector<std::pair<std::string, std::string>> get_path_params() const;
        std::string get_method() const;
        std::string get_path() const;
        std::string get_uri() const;
        std::vector<std::pair<std::string, std::string>> get_query_parameter(const std::string &name) const;
        std::string get_version() const;
        std::vector<std::string> get_header(const std::string &name) const;
        std::vector<std::pair<std::string, std::string>> get_headers() const;
        std::string get_body() const;
        std::vector<std::string> get_content_type() const;
        std::vector<std::string> get_cookies() const;
        std::vector<std::string> get_authorization() const;
        
    };
}