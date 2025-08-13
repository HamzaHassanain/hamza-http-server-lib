#include <bits/stdc++.h>
#include <socket_address.hpp>
#include <socket.hpp>
#include <thread>
// using namespace std;
// nc -u 127.0.0.1 12345
int main()
{
    hamza::family f1(hamza::IPV4); // family_id = AF_INET (default)
    hamza::port p1(12359);
    hamza::ip_address ip1("127.0.0.1");

    hamza::socket_address sa1(ip1, p1, f1); // Create socket address with AF_INET

    hamza::socket s1(sa1, hamza::Protocol::TCP); // Create TCP socket with the address

    s1.listen();

    while (true)
    {
        auto client_socket = s1.accept();

        // std::thread([client_socket = std::move(client_socket)]() mutable
        //             {
        //     // Handle client connection
        std::cout << "Connection established with " << client_socket.get_remote_address() << std::endl;
        while (client_socket.is_connected())
        {
            auto received_data = client_socket.receive_on_connection();
            std::cout << "Received data: " << received_data.to_string() << std::endl;
            std::cout << "client socket fd is: " << client_socket.get_file_descriptor() << std::endl;

            if (received_data.to_string() == "exit" || received_data.empty())
            {
                std::cout << "Client requested disconnection." << std::endl;
                client_socket.disconnect();
            }
            // Echo back the received data
            client_socket.send(received_data);
        }
        // } })
        // .detach();
    }
}
