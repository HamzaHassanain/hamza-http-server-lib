#pragma once

#include <string>
#include <web/web_types.hpp>
#include <web/web_route.hpp>
#include <web/web_server.hpp>
#include <web/web_exceptions.hpp>
namespace hamza::web
{
    class web_router
    {
        std::vector<web_route> routes;

        web_request_handler_t default_handler = [](std::shared_ptr<web_request> req, std::shared_ptr<web_response> res) -> int
        {
            res->set_status(404, "Not Found");
            res->text("404 Not Found");
            res->end();
            return EXIT;
        };

        void handle_request(std::shared_ptr<web_request> request, std::shared_ptr<web_response> response)
        {

            try
            {
                for (const auto &route : routes)
                {

                    if (route.match(request->get_path(), request->get_method()))
                    {
                        auto &handlers = route.handlers;
                        for (const auto &handler : handlers)
                        {
                            auto resp = handler(request, response);
                            if (resp == EXIT)
                            {
                                response->end();
                                return;
                            }
                        }
                        return;
                    }

                    default_handler(request, response);
                    response->end();
                    return;
                }
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

    public:
        friend class web_server;
        web_router() = default;

        web_router(const web_router &) = delete;
        web_router &operator=(const web_router &) = delete;

        web_router(web_router &&) = default;
        web_router &operator=(web_router &&) = default;

        void register_route(web_route &&route)
        {
            if (route.get_path().empty())
            {
                throw std::invalid_argument("Route path cannot be empty");
            }
            routes.push_back(std::move(route));
        }

        void set_default_handler(const web_request_handler_t &handler)
        {
            default_handler = std::move(handler);
        }
    };
}