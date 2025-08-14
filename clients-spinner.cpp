#include <bits/stdc++.h>
#include "includes/socket.hpp"
#include "includes/socket_address.hpp"
#include "includes/data_buffer.hpp"
#include "includes/port.hpp"
#include "includes/family.hpp"
#include "includes/ip_address.hpp"
#include "includes/utilities.hpp"

// compile this cpp file, and link with other fiels inside src,
/*

g++ -std=c++17 -Wall -Wextra -pedantic -g \
    -I./includes \
    clients-spinner.cpp \
    src/*.cpp \
    -o clients_spinner \
    -pthread

*/

std::mutex cout_mutex;

void print_client_message(const std::string &message)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << message << std::endl;
}

int main()
{
    hamza::socket_address server_address(hamza::ip_address("127.0.0.1"), hamza::port(12349), hamza::family(hamza::IPV4));

    // spin 100 client one on each thread
    std::vector<std::thread> client_threads;
    std::function<void()> spinner = [&]()
    {
        hamza::port rand_port = hamza::get_random_free_port();
        hamza::socket_address client_address(hamza::ip_address("127.0.0.1"), rand_port, hamza::family(hamza::IPV4));
        try
        {
            hamza::socket client_socket(client_address, hamza::Protocol::TCP);
            client_socket.connect(server_address);
            auto data_sent = hamza::data_buffer("Hello from client on port " + std::to_string(rand_port.get()) + "\n");
            client_socket.send_on_connection(data_sent);

            auto msg = client_socket.receive_on_connection();
            print_client_message("Client on port " + std::to_string(rand_port.get()) + " received: " + msg.to_string());

            std::this_thread::sleep_for(std::chrono::seconds(2));
            auto exit_data = hamza::data_buffer("Goodbye from client on port " + std::to_string(rand_port.get()) + "\n");
            client_socket.send_on_connection(exit_data);
        }

        catch (const std::exception &e)
        {
            print_client_message("Error connecting client at port " + std::to_string(rand_port.get()) + ": " + e.what());
        }
    };
    for (int i = 0; i < 10; i++)
    {
        client_threads.emplace_back(spinner);
    }

    for (auto &thread : client_threads)
    {
        thread.join();
    }

    return 0;
}
