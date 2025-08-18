

#include <bits/stdc++.h>

#include <http_server.hpp>
#include <http_request.hpp>
#include <http_response.hpp>

using namespace hamza_http;

class web : public http_server

{

public:
    web() : http_server("0.0.0.0", 8080) {}

protected:
    void on_request_received(http_request &request, http_response &response) override
    {
        if (request.get_method() == "GET" && request.get_uri() == "/")
        {
            response.set_status(200, "OK");
            response.set_body("<html><body><h1>Welcome to Hamza HTTP Server!</h1></body></html>");
        }
        else
        {
            response.set_status(404, "Not Found");
            response.set_body("<html><body><h1>404 Not Found</h1></body></html>");
        }

        response.add_header("Content-Type", "text/html");
        response.add_header("Connection", "close");
        response.send();
        response.end();
    }

    void on_server_listen() override
    {
        std::cout << "Server is listening on " << get_local_address() << std::endl;
    }
    void on_server_stopped() override
    {
        std::cout << "Server is stopping..." << std::endl;
    }
    void on_client_connected(std::shared_ptr<hamza::socket> sock_ptr) override
    {
        std::cout << "Client connected: " << sock_ptr->get_address().to_string() << std::endl;
    }
    void on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr) override
    {
        std::cout << "Client disconnected: " << sock_ptr->get_address().to_string() << std::endl;
    }
    void on_waiting_for_activity()
    {
        std::cout << "." << std::flush;
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    void on_exception(std::shared_ptr<hamza::socket_exception> e) override
    {
        std::cerr << e->what() << std::endl;
    }
};

int main()
{
    if (!hamza::initialize_socket_library())
    {
        std::cerr << "Failed to initialize socket library." << std::endl;
        return 1;
    }
    web server;
    server.listen();

    hamza::cleanup_socket_library();

    return 0;
}