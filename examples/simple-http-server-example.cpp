#include <bits/stdc++.h>
#include <http_server.hpp>
#include <tcp_server.hpp>
int main()
{
    auto server = std::make_shared<hamza_http::http_server>("127.0.0.1", 12345);
    int ReqCnt = 0;
    using reqT = hamza_http::http_request;
    using resT = hamza_http::http_response;
    using req = hamza_http::http_request;
    using res = hamza_http::http_response;

    auto handler_function = [&ReqCnt]([[maybe_unused]] std::shared_ptr<reqT> request, std::shared_ptr<resT> response)
    {
        // Handle the request and prepare the response
        try
        {

            std::string your_headers;
            for (const auto &[key, value] : request->get_headers())
            {
                your_headers += key + ": " + value + "\n";
            }

            response->set_status(200, "OK");
            response->add_header("Content-Type", "text/plain");
            response->add_header("Connection", "close");

            response->set_body(your_headers);
            ReqCnt++;
            response->send();
            response->end();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling request: " << e.what() << std::endl;
            response->set_status(500, "Internal Server Error");
            response->set_body("An error occurred while processing your request.");
            response->send();
            response->end();
        }
    };
    auto request_callback = [handler_function](req &request, res &response)
    {
        std::thread(handler_function, request, response).detach();
    };
    auto server_stopped_callback = [&ReqCnt]()
    {
        std::cout << "Server stopped. Total requests handled: " << ReqCnt << std::endl;
    };

    auto server_error_callback = [](std::shared_ptr<hamza::socket_exception> e)
    {
        std::cerr << "Server error occurred: " << e->what() << std::endl;
    };

    auto client_connected_callback = [](std::shared_ptr<hamza::socket> sock_ptr)
    {
        std::cout << "New client connected: " << sock_ptr->get_address() << std::endl;
    };

    auto client_disconnected_callback = [](std::shared_ptr<hamza::socket> sock_ptr)
    {
        std::cout << "Client disconnected: " << sock_ptr->get_address() << std::endl;
    };

    auto listen_success_callback = []()
    {
        std::cout << "Server is listening ...." << std::endl;
    };

    auto waiting_for_activity_callback = []()
    {
        std::cout << "Waiting for activity..." << std::endl;
    };

    server->set_request_callback(request_callback);
    server->set_server_stopped_callback(server_stopped_callback);
    server->set_error_callback(server_error_callback);
    server->set_client_connected_callback(client_connected_callback);
    server->set_client_disconnected_callback(client_disconnected_callback);
    server->set_listen_success_callback(listen_success_callback);
    server->set_waiting_for_activity_callback(waiting_for_activity_callback);

    server->listen();

    return 0;
}