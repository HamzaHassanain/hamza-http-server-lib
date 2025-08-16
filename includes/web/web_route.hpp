#pragma once
#include <string>
#include <vector>
#include <web/web_types.hpp>

namespace hamza::web
{
    class web_router;
    class web_general_exception;

    class web_route
    {
        std::string path;
        std::string method;
        std::vector<web_request_handler_t> handlers;

        bool match_path(const std::string &rhs) const;

    public:
        friend class web_router;

        web_route(const std::string &path, const std::string &method, const std::vector<web_request_handler_t> &handlers);

        web_route(const web_route &) = delete;
        web_route &operator=(const web_route &) = delete;
        web_route(web_route &&) = default;
        web_route &operator=(web_route &&) = default;

        std::string get_path() const;
        std::string get_method() const;
        virtual bool match(const std::string &path, const std::string &method) const;
    };
}