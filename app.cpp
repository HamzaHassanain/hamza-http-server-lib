#include "http-lib.hpp"

#include <functional>
#include <iostream>

int main()
{
    if (!hh_socket::initialize_socket_library())
    {
        std::cerr << "Failed to initialize socket library." << std::endl;
        return 1;
    }

    auto server = std::make_shared<hh_http::http_server>(8080);

    using req = hh_http::http_request;
    using res = hh_http::http_response;

    auto handler_function = []([[maybe_unused]] std::shared_ptr<req> request, std::shared_ptr<res> response)
    {
        try
        {
            std::string your_headers = request->get_method() + " " + request->get_uri() + " " + request->get_version() + "\n";

            if (request->get_method() == "POST")
            {
                std::cout << "POST ACC with " << request->get_body().size() << " bytes of data" << std::endl;
            }
            response->set_status(200, "OK");
            response->add_header("Content-Type", "text/plain");
            response->add_header("Connection", "close");

            response->set_body(your_headers);
            response->send();
            response->end();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling request: " << e.what() << std::endl;
            response->set_status(500, "Internal Server Error");
            response->set_body("An error occurred while processing your request.");
            response->end();
        }
    };
    auto request_callback = [handler_function](req &request, res &response)
    {
        auto req_ptr = std::make_shared<req>(std::move(request));
        auto res_ptr = std::make_shared<res>(std::move(response));
        handler_function(req_ptr, res_ptr);
    };

    server->set_request_callback(request_callback);
    server->listen();
    hh_socket::cleanup_socket_library();

    return 0;
}