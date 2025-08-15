#pragma once

#include <bits/stdc++.h>
#include <http_server.hpp>
#include <http_objects.hpp>

namespace hamza::web
{
    using request_callback_t = std::function<void(hamza::http::http_request &, hamza::http::http_response &)>;
    using listen_success_callback_t = std::function<void()>;
    using error_callback_t = std::function<void(std::unique_ptr<hamza::general_socket_exception>)>;

    class web_server
    {
        hamza::http::http_server server;
        std::string host;
        uint16_t port;

        request_callback_t request_callback = [](hamza::http::http_request &request, hamza::http::http_response &response) -> void
        {
            response.set_status(200, "OK");
            response.add_header("Content-Type", "text/html");

            std::ifstream file("html/index.cml");
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string index = buffer.str();

            std::cout << "Serving index.cml: " << index << std::endl;

            response.set_body(index);
            response.end();
        };

        listen_success_callback_t listen_success_callback = [this]() -> void
        {
            std::cout << "Server is listening at " << host << ":" << port << std::endl;
        };

        error_callback_t error_callback = [this](std::unique_ptr<hamza::general_socket_exception> e) -> void
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
        web_server(const std::string &host, uint16_t port, const request_callback_t &callback)
            : web_server(host, port)
        {
            server.set_request_callback(callback);
        }

        void listen(const listen_success_callback_t &callback = nullptr,
                    const error_callback_t &error_callback = nullptr)
        {
            if (callback)
                server.set_listen_success_callback(callback);
            if (error_callback)
                server.set_error_callback(error_callback);

            server.run();
        }
    };
}