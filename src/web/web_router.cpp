#include <web/web_router.hpp>
#include <web/web_request.hpp>
#include <web/web_response.hpp>
#include <web/web_exceptions.hpp>
#include <web/web_methods.hpp>
#include <iostream>

namespace hamza::web
{


    web_router::web_router()
    {
        default_handler = [](std::shared_ptr<web_request> req, std::shared_ptr<web_response> res) -> int
        {
            res->set_status(404, "Not Found");
            res->text("404 Not Found");
            res->end();
            return EXIT;
        };
    }

    void web_router::handle_request(std::shared_ptr<web_request> request, std::shared_ptr<web_response> response)
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

    void web_router::register_route(web_route &&route)
    {
        if (route.get_path().empty())
        {
            throw std::invalid_argument("Route path cannot be empty");
        }
        routes.push_back(std::move(route));
    }

  

    void web_router::set_default_handler(const web_request_handler_t &handler)
    {
        default_handler = std::move(handler);
    }
}