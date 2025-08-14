#include <bits/stdc++.h>
#include <socket_address.hpp>
#include <http_server.hpp>
#include <http_objects.hpp>

auto request_callback = [](hamza::http::http_request &request, hamza::http::http_response &response) -> void
{
    response.set_status(200, "OK");
    response.add_header("Content-Type", "text/html");

    std::ifstream file("html/index.html");
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string index = buffer.str();

    response.set_body(index);
    response.end();
};

int main()
{
    try
    {
        hamza::socket_address server_address(hamza::ip_address("127.0.0.1"), hamza::port(12349), hamza::family(hamza::IPV4));
        hamza::http::http_server server(server_address);

        server.set_request_callback(request_callback);

        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
