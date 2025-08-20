#include <socket.hpp>
#include <utilities.hpp>
#include <exceptions.hpp>
#include <iostream>
int main(int argc, char **argv)
{
    if (!hamza::initialize_socket_library())
    {
        std::cerr << "Failed to initialize socket library\n";
        return 1;
    }
    try
    {

        // CLI: ./udp_client [server_ip] [server_port]
        std::string server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
        int server_port = (argc > 2) ? std::atoi(argv[2]) : 8080;

        // Bind local UDP socket on a random free port (>= MIN_PORT)
        hamza::port local_port = hamza::get_random_free_port();
        hamza::socket_address local_addr(hamza::ip_address("0.0.0.0"), local_port, hamza::family(hamza::IPV4));
        hamza::socket client_socket(local_addr, hamza::Protocol::UDP);

        // Remote server address
        hamza::socket_address server_addr(hamza::ip_address(server_ip), hamza::port(server_port), hamza::family(hamza::IPV4));

        // Send a datagram
        std::string text = "Hello from hamza UDP client";
        client_socket.send_to(server_addr, hamza::data_buffer(text));
        std::cout << "Sent to " << server_addr << ": " << text << "\n";

        // Receive a reply (blocking)
        hamza::socket_address sender;
        hamza::data_buffer reply = client_socket.receive(sender);
        if (reply.size() > 0)
        {
            std::cout << "Received from " << sender << ": " << reply.to_string() << "\n";
        }
        else
        {
            std::cout << "No data received (zero-length)\n";
        }
    }
    catch (const hamza::socket_exception &e)
    {
        std::cerr << "Socket error: " << e.what() << '\n';
        return 2;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 3;
    }

    hamza::cleanup_socket_library();
    return 0;
}