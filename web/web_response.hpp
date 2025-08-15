#pragma once

#include <string>
#include <vector>
#include <http_objects.hpp>
#include <web_server.hpp>
#include <http_headers.hpp>
#include <web_types.hpp>

namespace hamza::web
{
    class web_response
    {
        http::http_response response;

        web_response(http::http_response &&response)
            : response(std::move(response))
        {
            response.set_status(200, "OK");
        }

    public:
        friend class web_server;

        void json(const std::string &json_data)
        {
            response.add_header(http::headers::CONTENT_TYPE, "application/json");
            response.set_body(json_data);
        }
        void html(const std::string &html_data)
        {
            response.add_header(http::headers::CONTENT_TYPE, "text/html");
            response.set_body(html_data);
        }
        void text(const std::string &text_data)
        {
            response.add_header(http::headers::CONTENT_TYPE, "text/plain");
            response.set_body(text_data);
        }

        void end()
        {
            response.end();
        }
    };
};