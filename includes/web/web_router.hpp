
#include <string>
#include <web/web_types.hpp>
#include <web/web_route.hpp>
#include <web/web_server.hpp>
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
            for (const auto &route : routes)
            {
                if (route.match(request->get_path(), request->get_method()))
                {
                    auto &handlers = route.handlers;
                    for (const auto &handler : handlers)
                    {
                        if (handler(request, response) == EXIT)
                        {
                            response->end();
                            return;
                        }
                    }
                    return;
                }
            }

            default_handler(request, response);
            response->end();
            return;
        }

    public:
        friend class web_server;
        web_router() = default;


        void register_route

        void set_default_handler(const web_request_handler_t &handler)
        {
            default_handler = std::move(handler);
        }
    };
}