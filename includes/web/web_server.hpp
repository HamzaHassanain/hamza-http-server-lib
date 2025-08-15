#pragma once

#include <bits/stdc++.h>
#include <http_server.hpp>
#include <http_objects.hpp>
#include <web/web_types.hpp>
#include <web/web_methods.hpp>
#include <web/web_request.hpp>
#include <web/web_response.hpp>
#include <web/web_router.hpp>
namespace hamza::web
{

    class web_server
    {
        http::http_server server;
        web_router router;
        std::string host;
        uint16_t port;

        http_request_callback_t request_callback = [this](hamza::http::http_request &request, hamza::http::http_response &response) -> void
        {
            web_request web_req(std::move(request));
            web_response web_res(std::move(response));

            auto web_res_ptr = std::make_shared<web_response>(std::move(web_res));
            auto web_req_ptr = std::make_shared<web_request>(std::move(web_req));

            auto path = web_req_ptr->get_path();
            auto method = web_req_ptr->get_method();

            web_res_ptr->end();
        };

        web_listen_success_callback_t listen_success_callback = [this]() -> void
        {
            std::cout << "Server is listening at " << host << ":" << port << std::endl;
        };

        web_error_callback_t error_callback = [](std::shared_ptr<hamza::general_socket_exception> e) -> void
        {
            std::cerr << "Error occurred: " << e->type() << std::endl;
            std::cerr << "Error occurred: " << e->what() << std::endl;
        };

        void set_default_callbacks()
        {
            server.set_request_callback(request_callback);
            server.set_listen_success_callback(listen_success_callback);
            server.set_error_callback(error_callback);
        }

    public:
        web_server(const std::string &host, uint16_t port)
            : server(host, port), host(host), port(port)
        {
            set_default_callbacks();
        }

        void 

        void listen(const web_listen_success_callback_t &callback = nullptr,
                    const web_error_callback_t &error_callback = nullptr)
        {
            if (callback)
                server.set_listen_success_callback(callback);
            if (error_callback)
                server.set_error_callback(error_callback);

            server.run();
        }
    };
}