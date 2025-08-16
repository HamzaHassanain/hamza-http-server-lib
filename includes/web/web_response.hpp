#pragma once

#include <string>
#include <vector>
#include <http_objects.hpp>
#include <http_headers.hpp>

namespace hamza::web
{
    class web_server;

    class web_response
    {
        http::http_response response;
        bool did_end = 0;

        web_response(http::http_response &&response);

    public:
        friend class web_server;

        void set_status(int status_code, const std::string &status_message);
        void json(const std::string &json_data);
        void html(const std::string &html_data);
        void text(const std::string &text_data);
        void add_header(const std::string &key, const std::string &value);
        void add_trailer(const std::string &key, const std::string &value);
        void add_cookie(const std::string &name, const std::string &cookie, const std::string &attributes = "");
        void set_content_type(const std::string &content_type);
        void set_body(const std::string &body);
        void end();
    };
}