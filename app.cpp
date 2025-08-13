#include <bits/stdc++.h>
#include <socket_address.hpp>
#include <socket.hpp>
#include <thread>
#include <set>
#include <functional>
#include <memory>

struct compare_shared_ptr
{
    bool operator()(const std::shared_ptr<hamza::socket> &a, const std::shared_ptr<hamza::socket> &b) const
    {
        return a->get_file_descriptor().get() < b->get_file_descriptor().get();
    }
};

struct clients_container
{
private:
    std::vector<std::shared_ptr<hamza::socket>> sockets;
    using callback = std::function<void(std::shared_ptr<hamza::socket>)>;

public:
    void insert(std::shared_ptr<hamza::socket> sock)
    {
        sockets.push_back(sock);
    }

    void erase(std::shared_ptr<hamza::socket> sock)
    {
        auto it = std::find(sockets.begin(), sockets.end(), sock);
        if (it != sockets.end())
        {
            sockets.erase(it);
        }
    }

    bool contains(std::shared_ptr<hamza::socket> sock) const
    {
        return std::find(sockets.begin(), sockets.end(), sock) != sockets.end();
    }
    size_t size() const
    {
        return sockets.size();
    }
    void clear()
    {
        sockets.clear();
    }

    std::vector<std::shared_ptr<hamza::socket>>::const_iterator begin() const
    {
        return sockets.begin();
    }
    std::vector<std::shared_ptr<hamza::socket>>::const_iterator end() const
    {
        return sockets.end();
    }

    int max() const
    {
        if (sockets.empty())
        {
            return -1;
        }
        int max_fd = -1;
        for (const auto &sock : sockets)
        {
            max_fd = std::max(max_fd, sock->get_file_descriptor().get());
        }
        return max_fd;
    }
};

class Server
{
    clients_container clients;
    fd_set master_fds;
    fd_set read_fds;
    int max_fds;
    std::shared_ptr<hamza::socket> server_socket;

    struct timeval make_timeout(int seconds)
    {
        // Set timeout for select
        struct timeval timeout;
        timeout.tv_sec = seconds;
        timeout.tv_usec = 0;

        return timeout;
    }

    void remove_client(std::shared_ptr<hamza::socket> sock_ptr)
    {
        try
        {
            FD_CLR(sock_ptr->get_file_descriptor().get(), &master_fds);

            std::cout << "Client disconnected: " << sock_ptr->get_remote_address().get_ip_address() << ":" << sock_ptr->get_remote_address().get_port() << std::endl;

            clients.erase(sock_ptr);

            // Recalculate max_fds after removing client
            int server_fd = server_socket->get_file_descriptor().get();
            int max_client_fd = clients.max(); // This returns -1 if no clients
            max_fds = (max_client_fd == -1) ? server_fd : std::max(server_fd, max_client_fd);

            std::cout << "Client removed, new max_fds: " << max_fds << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error removing client: " << e.what() << std::endl;
        }
    }

    void check_for_activity()
    {
        if (FD_ISSET(server_socket->get_file_descriptor().get(), &read_fds))
        {
            handle_new_connection();
        }
        for (auto sock_ptr : clients)
        {
            // std::cout << "Checking client activity for: " << sock_ptr->get_remote_address().get_ip_address() << ":" << sock_ptr->get_remote_address().get_port() << std::endl;
            if (FD_ISSET(sock_ptr->get_file_descriptor().get(), &read_fds))
            {
                handle_client_activity(sock_ptr);
            }
        }
    }

public:
    Server(const hamza::socket_address &addr, const hamza::Protocol &protocol)
    {
        server_socket = std::make_shared<hamza::socket>(addr, protocol, true);
        server_socket->listen();
        FD_ZERO(&master_fds);
        FD_ZERO(&read_fds);
        FD_SET(server_socket->get_file_descriptor().get(), &master_fds);
        max_fds = server_socket->get_file_descriptor().get();

        // std::cout << "MAX FDs " << max_fds << std::endl;
    }

    void start_listening()
    {

        auto addr = server_socket->get_remote_address();
        std::cout << "server running..." << std::endl;
        std::cout << "connect to server at: " << addr.get_ip_address() << " " << addr.get_port() << std::endl;
        while (true)
        {
            try
            {
                read_fds = master_fds;

                auto timeout = make_timeout(1);
                // Wait for activity on any socket
                int activity = select(max_fds + 1, &read_fds, nullptr, nullptr, &timeout);
                // std::cout << "Activity detected: " << activity << std::endl;
                if (activity < 0)
                {
                    if (errno == EINTR)
                        throw std::runtime_error("Select interrupted");
                    throw std::runtime_error("Select error: " + std::string(strerror(errno)));
                }

                if (activity == 0)
                {
                    // Timeout - show server is alive
                    std::cout << "." << std::flush;
                    continue;
                }
                std::cout << std::endl;
                // std::cout << "Activity detected on server socket: " << server_socket.get_file_descriptor().get() << std::endl;
                check_for_activity();
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error in server loop: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }

    void handle_client_activity(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // read client message only,
        try
        {

            hamza::data_buffer db = sock_ptr->receive_on_connection();

            if (db.empty())
            {
                remove_client(sock_ptr);
                return;
            }
            auto client_address = sock_ptr->get_remote_address();
            std::cout << "Received message from client at: " << client_address.get_ip_address() << ":" << client_address.get_port() << std::endl;
            std::cout << "Data what: " << db.to_string() << std::endl;
            sock_ptr->send_on_connection(db); // Echo back the received data
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling client activity: " << e.what() << std::endl;
        }
    }

    void handle_new_connection()
    {
        try
        {
            auto client_socket = server_socket->accept();
            auto client_socket_ptr = std::make_shared<hamza::socket>(std::move(client_socket));
            clients.insert(client_socket_ptr);
            // std::cout << "New client connected: " << client_ptr->get_remote_address().get_ip_address() << ":" << client_ptr->get_remote_address().get_port() << std::endl;
            FD_SET(client_socket_ptr->get_file_descriptor().get(), &master_fds);

            // Update max_fds to include both server socket and all clients
            int server_fd = server_socket->get_file_descriptor().get();
            int max_client_fd = clients.max();
            max_fds = std::max(server_fd, max_client_fd);

            std::cout << "New client connected, server_fd: " << server_fd
                      << ", max_client_fd: " << max_client_fd
                      << ", max_fds: " << max_fds << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error handling new connection: " << e.what() << std::endl;
        }
    }
};

int main()
{
    try
    {

        hamza::port server_port(12349);
        hamza::ip_address server_ip("127.0.0.1");
        hamza::family server_family(hamza::IPV4);
        hamza::socket_address server_address(server_ip, server_port, server_family);
        hamza::Protocol protocol = hamza::Protocol::TCP;
        Server server(server_address, protocol);

        server.start_listening();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
