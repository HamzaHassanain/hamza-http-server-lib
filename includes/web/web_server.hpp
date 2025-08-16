#pragma once

#include <bits/stdc++.h>
#include <http_server.hpp>
#include <http_objects.hpp>
#include <thread>
#include <iostream>

#include <web/web_types.hpp>
#include <web/web_methods.hpp>
#include <web/web_request.hpp>
#include <web/web_response.hpp>
#include <web/web_router.hpp>
#include <web/web_exceptions.hpp>
#include <web/web_utilities.hpp>

namespace hamza::web
{
    template <typename RequestType = web_request, typename ResponseType = web_response>
    class web_server
    {
        using ret = std::function<void(std::shared_ptr<hamza::general_socket_exception>)>;
        using param = std::function<void(std::shared_ptr<hamza::web::web_general_exception>)>;

        ret custom_wrap(param &callback)
        {
            return [callback = std::move(callback)](std::shared_ptr<hamza::general_socket_exception> e) -> void
            {
                if (auto web_exc = std::dynamic_pointer_cast<hamza::web::web_general_exception>(e))
                {
                    callback(web_exc);
                }
                else
                {
                    callback(std::make_shared<hamza::web::web_general_exception>(e->what()));
                }
            };
        }

        http::http_server server;
        std::shared_ptr<web_router<RequestType, ResponseType>> router;
        std::string host;
        uint16_t port;
        std::vector<std::string> static_directories;

        http_request_callback_t request_callback;
        web_listen_success_callback_t listen_success_callback;
        web_error_callback_t error_callback;

        void set_default_callbacks()
        {
            server.set_request_callback(request_callback);
            server.set_listen_success_callback(listen_success_callback);
            server.set_error_callback(custom_wrap(error_callback));
        }

        void server_static(std::shared_ptr<RequestType> req, std::shared_ptr<ResponseType> res)
        {
            try
            {
                std::string uri = req->get_uri();
                std::string sanitized_path = sanitize_path(uri);
                std::string file_path;
                std::cout << "Looking for static file: " << sanitized_path << std::endl;
                for (const auto &dir : static_directories)
                {
                    file_path = dir + sanitized_path;
                    if (std::ifstream(file_path))
                    {
                        break;
                    }
                }

                if (file_path.empty() || !std::ifstream(file_path))
                {
                    res->set_status(404, "Not Found");
                    res->text("404 Not Found");
                    res->end();
                    return;
                }

                std::ifstream file(file_path);
                std::stringstream buffer;
                buffer << file.rdbuf();
                res->set_body(buffer.str());
                res->set_content_type(get_mime_type_from_extension(get_file_extension_from_uri(uri)));
            }
            catch (const std::exception &e)
            {
                res->set_status(500, "Internal Server Error");
                res->text("500 Internal Server Error: " + std::string(e.what()));
            }
        }

    public:
        web_server(const std::string &host, uint16_t port)
            : server(host, port), host(host), port(port)
        {
            request_callback = [this](hamza::http::http_request &request, hamza::http::http_response &response) -> void
            {
                RequestType web_req(std::move(request));
                ResponseType web_res(std::move(response));

                auto web_res_ptr = std::make_shared<ResponseType>(std::move(web_res));
                auto web_req_ptr = std::make_shared<RequestType>(std::move(web_req));

                try
                {
                    std::thread([this, web_req_ptr, web_res_ptr]() mutable
                                {
                                    std::cout << web_req_ptr->get_method() << " " << web_req_ptr->get_uri() << std::endl;
                                    if (is_uri_static(web_req_ptr->get_uri()))
                                        server_static(web_req_ptr, web_res_ptr);
                                    else
                                        router->handle_request(web_req_ptr, web_res_ptr);
                                    web_res_ptr->end(); })
                        .detach();
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error handling request: " << e.what() << std::endl;
                    web_res_ptr->set_status(500, "Internal Server Error");
                    web_res_ptr->text("500 Internal Server Error");
                    web_res_ptr->end();
                }
            };

            listen_success_callback = [this]() -> void
            {
                std::cout << "Server is listening at " << this->host << ":" << this->port << std::endl;
            };

            error_callback = [](std::shared_ptr<web_general_exception> e) -> void
            {
                std::cerr << "Error occurred: " << e->get_status_code() << ": " << e->get_status_message() << std::endl;
                std::cerr << "Error occurred: " << e->type() << std::endl;
                std::cerr << "Error occurred: " << e->what() << std::endl;
            };

            set_default_callbacks();
        }

        void register_router(std::shared_ptr<web_router<RequestType, ResponseType>> router)
        {
            this->router = router;
        }

        void register_static(const std::string &directory)
        {
            static_directories.push_back(directory);
        }

        void listen(web_listen_success_callback_t callback = nullptr,
                    web_error_callback_t error_callback = nullptr)
        {
            if (callback)
                server.set_listen_success_callback(callback);
            if (error_callback)
                server.set_error_callback(custom_wrap(error_callback));

            server.run();
        }
    };
}