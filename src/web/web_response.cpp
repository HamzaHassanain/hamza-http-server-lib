#include <web/web_response.hpp>

namespace hamza::web
{
    web_response::web_response(http::http_response &&response)
        : response(std::move(response))
    {
        response.set_status(200, "OK");
    }

    void web_response::set_status(int status_code, const std::string &status_message)
    {
        response.set_status(status_code, status_message);
    }

    void web_response::json(const std::string &json_data)
    {
        response.add_header(http::headers::CONTENT_TYPE, "application/json");
        response.set_body(json_data);
    }

    void web_response::html(const std::string &html_data)
    {
        response.add_header(http::headers::CONTENT_TYPE, "text/html");
        response.set_body(html_data);
    }

    void web_response::text(const std::string &text_data)
    {
        response.add_header(http::headers::CONTENT_TYPE, "text/plain");
        response.set_body(text_data);
    }


    void web_response::add_header(const std::string &key, const std::string &value)
    {
        response.add_header(key, value);
    }

    void web_response::add_trailer(const std::string &key, const std::string &value)
    {
        response.add_trailer(key, value);
    }

    void web_response::add_cookie(const std::string &name, const std::string &cookie, const std::string &attributes)
    {
        std::string value = cookie;
        if (!attributes.empty())
        {
            value += "; " + attributes;
        }
        response.add_header("Set-Cookie", name + "=" + value);
    }

    void web_response::set_content_type(const std::string &content_type)
    {
        response.add_header(http::headers::CONTENT_TYPE, content_type);
    }

    void web_response::set_body(const std::string &body)
    {
        response.set_body(body);
    }

    void web_response::end()
    {
        if (did_end)
            return;
        did_end = true;
        response.end();
    }
}