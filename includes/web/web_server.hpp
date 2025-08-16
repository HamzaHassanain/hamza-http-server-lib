#pragma once

#include <bits/stdc++.h>
#include <http_server.hpp>
#include <http_objects.hpp>

#include <web/web_types.hpp>
#include <web/web_methods.hpp>
#include <web/web_request.hpp>
#include <web/web_response.hpp>
#include <web/web_router.hpp>
#include <web/web_exceptions.hpp>
namespace hamza::web
{

    class web_server
    {
        using ret = std::function<void(std::shared_ptr<hamza::general_socket_exception>)>;
        using param = std::function<void(std::shared_ptr<hamza::web::web_general_exception>)>;

        ret custom_wrap(param &callback);

        http::http_server server;
        web_router router;
        std::string host;
        uint16_t port;
        std::vector<std::string> static_directories;

        http_request_callback_t request_callback;
        web_listen_success_callback_t listen_success_callback;
        web_error_callback_t error_callback;

        void set_default_callbacks();
        void server_static(std::shared_ptr<web_request> req, std::shared_ptr<web_response> res);

    public:
        web_server(const std::string &host, uint16_t port);
        void register_static(const std::string &directory);
        void register_router(web_router &&router);
        void listen(web_listen_success_callback_t callback = nullptr,
                    web_error_callback_t error_callback = nullptr);
    };
}