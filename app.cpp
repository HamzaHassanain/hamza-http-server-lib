#include <bits/stdc++.h>
#include <clients_container.hpp>
#include <socket_address.hpp>
#include <socket.hpp>
#include <thread>
#include <set>
#include <functional>
#include <memory>
#include <tcp_server.hpp>

class Server : public hamza::tcp_server
{
public:
    Server(const hamza::socket_address &addr) : hamza::tcp_server(addr) {}

    void on_message_received(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message) override
    {
        try
        {
            std::cout << "Message received from client: " << message.to_string() << std::endl;
            // empty response

            if (message.to_string().find("Goodbye") != std::string::npos)
            {
                close_connection(sock_ptr);
                return;
            }

            if (message.empty())
            {
                std::cout << "Misbehaving client detected. Closing connection." << std::endl;
                std::cout << sock_ptr->get_remote_address() << " sent an empty message." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                // close_connection(sock_ptr);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error processing message: " << e.what() << std::endl;
            throw;
            // Optionally, you can remove the client if there's an error
            // remove_client(sock_ptr);
        }
    }

    void on_listen_success() override
    {
        std::cout << "Server is listening for connections..." << std::endl;
    }

    void on_exception(std::unique_ptr<hamza::general_socket_exception> e) override
    {
        std::cerr << "Type: " << e->type() << std::endl;
        std::cerr << "Socket error: " << e->what() << std::endl;
    }

    void on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr) override
    {
        std::cout << "Client disconnected: " << sock_ptr->get_remote_address() << std::endl;
    }
    void on_new_client_connected(std::shared_ptr<hamza::socket> sock_ptr) override
    {
        std::cout << "New client connected: " << sock_ptr->get_remote_address() << std::endl;
    }
};

int main()
{
    try
    {
        // nc 127.0.0.1 12349
        hamza::socket_address server_address(hamza::ip_address("127.0.0.1"), hamza::port(12349), hamza::family(hamza::IPV4));
        Server server(server_address);

        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
