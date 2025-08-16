#pragma once

#include <string>
#include <vector>
#include <memory>
#include <web/web_types.hpp>
#include <web/web_exceptions.hpp>
#include <web/web_methods.hpp>
#include <iostream>

namespace hamza::web
{
    template <typename RequestType, typename ResponseType>
    class web_route; // Forward declaration

    template <typename RequestType, typename ResponseType>
    class web_server; // Forward declaration

    template <typename RequestType = web_request, typename ResponseType = web_response>
    class web_router
    {
        std::vector<std::shared_ptr<web_route<RequestType, ResponseType>>> routes;
        web_request_handler_t<RequestType, ResponseType> default_handler;

    public:
        friend class web_server<RequestType, ResponseType>;

        web_router()
        {
            default_handler = [](std::shared_ptr<RequestType> req, std::shared_ptr<ResponseType> res) -> int
            {
                res->set_status(404, "Not Found");
                res->text("404 Not Found");
                res->end();
                return EXIT;
            };
        }

        web_router(const web_router &) = delete;
        web_router &operator=(const web_router &) = delete;
        web_router(web_router &&) = default;
        web_router &operator=(web_router &&) = default;

        void handle_request(std::shared_ptr<RequestType> request, std::shared_ptr<ResponseType> response)
        {
            try
            {
                for (const auto &route : routes)
                {
                    if (route->match(request->get_path(), request->get_method()))
                    {
                        auto &handlers = route->handlers;
                        for (const auto &handler : handlers)
                        {
                            auto resp = handler(request, response);
                            if (resp == EXIT)
                            {
                                response->end();
                                return;
                            }
                            if (resp == ERROR)
                            {
                                throw web_general_exception("Handler returned an error");
                            }
                        }
                    }
                }
                default_handler(request, response);
                response->end();
                return;
            }
            catch (const web_general_exception &e)
            {
                std::cerr << "Error handling request: " << e.what() << std::endl;
                response->set_status(e.get_status_code(), e.get_status_message());
                response->text(e.what());
                response->end();
                return;
            }
        }

        void register_route(std::shared_ptr<web_route<RequestType, ResponseType>> route)
        {
            if (route->get_path().empty())
            {
                throw std::invalid_argument("Route path cannot be empty");
            }
            routes.push_back(route);
        }

        void set_default_handler(const web_request_handler_t<RequestType, ResponseType> &handler)
        {
            default_handler = std::move(handler);
        }
    };
}

#include <web/web_route.hpp> // Include after the template declaration