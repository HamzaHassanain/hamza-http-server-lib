
// #include <http_server.hpp>

// int main()
// {
//     hamza_http::http_server server("127.0.0.1", 12345);

//     server.set_request_callback([](hamza_http::http_request &request, hamza_http::http_response &response)
//                                 {
//         // Handle the request and prepare the response
//         std::cout << "Received request: " << request.get_method() << " " << request.get_uri() << std::endl;
//         response.set_status(200 , "OK");
//         response.add_header("Content-Type", "text/plain");
//         response.set_body("Hello, World!");
//         response.end(); });
//     server.run();

//     return 0;
// }

#include <bits/stdc++.h>
#include <socket.hpp>

int main()
{
    if (!hamza::initialize_socket_library())
    {
        std::cerr << "Failed to initialize socket library." << std::endl;
        return 1;
    }

    // Create server address and socket
    hamza::socket_address server_addr(
        hamza::ip_address("127.0.0.1"),
        hamza::port(8080),
        hamza::family(hamza::IPV4));
    hamza::socket server_socket(server_addr, hamza::Protocol::TCP, true);

    server_socket.listen(5);
    std::cout << "TCP Server listening on port 8080..." << std::endl;

    while (true)
    {
        auto client_socket = server_socket.accept();

        // Handle client in separate thread
        std::thread([client_socket]()
                    {
            while (client_socket->is_connected()) {
                hamza::data_buffer data = client_socket->receive_on_connection();
                if (data.size() == 0) break;

                // Echo back
                std::string response = "Echo: " + data.to_string();
                client_socket->send_on_connection(hamza::data_buffer(response));
            } })
            .detach();
    }

    hamza::cleanup_socket_library();
    return 0;
}