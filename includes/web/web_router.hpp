#pragma once

#include <string>
#include <vector>
#include <web/web_types.hpp>
#include <web/web_route.hpp>

namespace hamza::web
{
    class web_server;
    class web_request;
    class web_response;
    class web_general_exception;

    class web_router
    {
        std::vector<web_route> routes;
        web_request_handler_t default_handler;

        void handle_request(std::shared_ptr<web_request> request, std::shared_ptr<web_response> response);

    public:
        friend class web_server;

        web_router();
        web_router(const web_router &) = delete;
        web_router &operator=(const web_router &) = delete;
        web_router(web_router &&) = default;
        web_router &operator=(web_router &&) = default;

        void register_route(web_route &&route);
        void set_default_handler(const web_request_handler_t &handler);
    };
}