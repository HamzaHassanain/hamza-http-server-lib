#pragma once

#include <string>
#include <vector>
#include <http_objects.hpp>
#include <http_headers.hpp>

namespace hamza::web
{
    template <typename RequestType, typename ResponseType>
    class web_server;

    class web_response
    {
        http::http_response response;
        bool did_end = 0;

        web_response(http::http_response &&response) : response(std::move(response))
        {
            response.set_status(200, "OK");
        }

    public:
        template <typename RequestType, typename ResponseType>
        friend class web_server;

        void set_status(int status_code, const std::string &status_message)
        {
            response.set_status(status_code, status_message);
        }

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

        void add_header(const std::string &key, const std::string &value)
        {
            response.add_header(key, value);
        }

        void add_trailer(const std::string &key, const std::string &value)
        {
            response.add_trailer(key, value);
        }

        void add_cookie(const std::string &name, const std::string &cookie, const std::string &attributes = "")
        {
            std::string value = cookie;
            if (!attributes.empty())
            {
                value += "; " + attributes;
            }
            response.add_header("Set-Cookie", name + "=" + value);
        }

        void set_content_type(const std::string &content_type)
        {
            response.add_header(http::headers::CONTENT_TYPE, content_type);
        }

        void set_body(const std::string &body)
        {
            response.set_body(body);
        }

        void end()
        {
            if (did_end)
                return;
            did_end = true;
            response.end();
        }
    };
}