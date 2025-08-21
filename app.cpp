#include <bits/stdc++.h>
#include <http_server.hpp>
#include <tcp_server.hpp>
#include <functional>
#include <iostream>
int main()
{

    if (!hamza::initialize_socket_library())
    {
        std::cerr << "Failed to initialize socket library." << std::endl;
        return 1;
    }

    auto server = std::make_shared<hamza_http::http_server>("127.0.0.1", 12345, 0, 50000);
    int ReqCnt = 0;
    using reqT = hamza_http::http_request;
    using resT = hamza_http::http_response;
    using req = hamza_http::http_request;
    using res = hamza_http::http_response;

    std::mutex mtx;
    auto log_cnt = [&ReqCnt, &mtx]()
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Request count: " << ReqCnt << std::endl;
        ReqCnt++;
    };

    auto handler_function = [&log_cnt]([[maybe_unused]] std::shared_ptr<reqT> request, std::shared_ptr<resT> response)
    {
        // Handle the request and prepare the response
        try
        {
            std::string your_headers;
            // for (const auto &[key, value] : request->get_headers())
            // {
            //     your_headers += key + ": " + value + "\n";
            // }
            std::this_thread::sleep_for(std::chrono::seconds(1));
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
        std::thread(handler_function, req_ptr, res_ptr).detach();
    };
    auto server_stopped_callback = [&ReqCnt]()
    {
        std::cout << "Server stopped. Total requests handled: " << ReqCnt << std::endl;
    };

    auto server_error_callback = [](std::shared_ptr<hamza::socket_exception> e)
    {
        std::cerr << "Server error occurred: " << e->what() << std::endl;
    };

    auto listen_success_callback = []()
    {
        std::cout << "Server is listening ...." << std::endl;
    };

    auto on_client_connect = [](const std::shared_ptr<hamza::socket> &client_socket)
    {
        // std::cout << "Client connected: " << client_socket->get_remote_address() << std::endl;
    };

    auto on_client_disconnect = [](const std::shared_ptr<hamza::socket> &client_socket)
    {
        // std::cout << "Client disconnected: " << client_socket->get_remote_address() << std::endl;
    };

    server->set_request_callback(request_callback);
    server->set_server_stopped_callback(server_stopped_callback);
    server->set_error_callback(server_error_callback);
    server->set_listen_success_callback(listen_success_callback);
    server->set_client_connected_callback(on_client_connect);
    server->set_client_disconnected_callback(on_client_disconnect);
    server->listen();
    hamza::cleanup_socket_library();

    return 0;
}