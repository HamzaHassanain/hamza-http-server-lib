#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cerrno>

// POSIX socket headers
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

class Select
{
private:
    int server_fd;
    std::unordered_map<int, std::string> clients; // fd -> client info
    fd_set master_fds;
    fd_set read_fds;
    int max_fd;
    sockaddr_in server_addr;

public:
    Select(const std::string &ip, int port)
    {
        // Create socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1)
        {
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }

        // Set socket options
        int opt = 1;
        std::cout << "File descriptor: " << server_fd << std::endl;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            close(server_fd);
            throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
        }

        // Setup server address
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0)
        {
            close(server_fd);
            throw std::runtime_error("Invalid IP address: " + ip);
        }

        // Bind socket
        if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            close(server_fd);
            throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
        }

        // Listen for connections
        if (listen(server_fd, SOMAXCONN) == -1)
        {
            close(server_fd);
            throw std::runtime_error("Failed to listen: " + std::string(strerror(errno)));
        }

        // Initialize file descriptor sets
        FD_ZERO(&master_fds);
        FD_ZERO(&read_fds);

        // Add server socket to master set
        FD_SET(server_fd, &master_fds);
        max_fd = server_fd;

        std::cout << "Select server initialized on fd " << server_fd << std::endl;
    }

    ~Select()
    {
        // Close all client connections
        for (auto &[fd, info] : clients)
        {
            close(fd);
        }
        // Close server socket
        if (server_fd != -1)
        {
            close(server_fd);
        }
    }

    void run()
    {
        std::cout << "Select server running..." << std::endl;
        std::cout << "Use: nc 127.0.0.1 12349 to connect" << std::endl;
        std::cout << "Commands: 'stats', 'broadcast <msg>', 'exit'" << std::endl;
        std::cout << std::endl;

        while (true)
        {
            // Copy master set to working set
            read_fds = master_fds;

            // Set timeout for select
            struct timeval timeout;
            timeout.tv_sec = 1; // 1 second timeout
            timeout.tv_usec = 0;

            // Wait for activity on any socket
            int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);

            if (activity < 0)
            {
                if (errno == EINTR)
                    continue; // Interrupted by signal
                std::cerr << "Select error: " << strerror(errno) << std::endl;
                continue;
            }

            if (activity == 0)
            {
                // Timeout - show server is alive
                std::cout << max_fd << " is alive" << std::endl;
                continue;
            }

            // Check all file descriptors for activity
            for (int fd = 0; fd <= max_fd; ++fd)
            {
                if (FD_ISSET(fd, &read_fds))
                {
                    if (fd == server_fd)
                    {
                        // New connection on server socket
                        handle_new_connection();
                    }
                    else
                    {
                        // Activity on client socket
                        handle_client_activity(fd);
                    }
                }
            }
        }
    }

private:
    void handle_new_connection()
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_len);
        if (client_fd == -1)
        {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            return;
        }

        // Get client IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        std::string client_info = std::string(client_ip) + ":" + std::to_string(client_port);

        std::cout << "\nNew client connected: " << client_info << " (fd: " << client_fd << ")" << std::endl;

        // Add to master set
        FD_SET(client_fd, &master_fds);
        if (client_fd > max_fd)
        {
            max_fd = client_fd;
        }

        // Store client info
        clients[client_fd] = client_info;

        std::cout << "Client added to select set, total clients: " << clients.size() << std::endl;

        // Send welcome message
        std::string welcome = "Welcome to Select Echo Server! (fd: " +
                              std::to_string(client_fd) + ", " + client_info + ")\n" +
                              "Commands: 'stats', 'broadcast <message>', 'exit'\n";

        send_message(client_fd, welcome);
    }

    void handle_client_activity(int client_fd)
    {
        auto it = clients.find(client_fd);
        if (it == clients.end())
        {
            std::cerr << "Unknown client fd: " << client_fd << std::endl;
            return;
        }

        char buffer[1024];
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0)
        {
            if (bytes_received == 0)
            {
                std::cout << "\nClient disconnected: " << it->second << " (fd: " << client_fd << ")" << std::endl;
            }
            else
            {
                std::cout << "\nReceive error from " << it->second << ": " << strerror(errno) << std::endl;
            }
            remove_client(client_fd);
            return;
        }

        // Null-terminate the message
        buffer[bytes_received] = '\0';
        std::string message(buffer);

        std::cout << "\nReceived from " << it->second << " (fd: " << client_fd << "): " << message;

        // Handle special commands
        if (message.find("exit") != std::string::npos)
        {
            std::cout << "Client requested exit: " << it->second << std::endl;
            send_message(client_fd, "Goodbye from select server!\n");
            remove_client(client_fd);
            return;
        }

        if (message.find("stats") != std::string::npos)
        {
            std::string stats = "Server Stats:\n";
            stats += "- Connected clients: " + std::to_string(clients.size()) + "\n";
            stats += "- Your fd: " + std::to_string(client_fd) + "\n";
            stats += "- Your address: " + it->second + "\n";
            stats += "- Max fd: " + std::to_string(max_fd) + "\n";
            stats += "Client list:\n";
            for (const auto &[fd, info] : clients)
            {
                stats += "  fd " + std::to_string(fd) + ": " + info + "\n";
            }
            send_message(client_fd, stats);
            return;
        }

        if (message.find("broadcast ") == 0)
        {
            // Extract broadcast message
            std::string broadcast_content = message.substr(10); // Remove "broadcast "
            std::string broadcast_msg = "Broadcast from " + it->second + ": " + broadcast_content;
            int sent_count = broadcast_to_all(broadcast_msg, client_fd);

            std::string confirmation = "Broadcast sent to " + std::to_string(sent_count) + " clients\n";
            send_message(client_fd, confirmation);
            return;
        }

        if (message.find("list") != std::string::npos)
        {
            std::string client_list = "Connected clients (" + std::to_string(clients.size()) + "):\n";
            for (const auto &[fd, info] : clients)
            {
                client_list += "  fd " + std::to_string(fd) + ": " + info;
                if (fd == client_fd)
                    client_list += " (you)";
                client_list += "\n";
            }
            send_message(client_fd, client_list);
            return;
        }

        // Echo back with client info
        std::string response = "Echo from select server to " + it->second +
                               " (fd " + std::to_string(client_fd) + "): " + message;
        send_message(client_fd, response);
    }

    void send_message(int client_fd, const std::string &message)
    {
        ssize_t bytes_sent = send(client_fd, message.c_str(), message.length(), 0);
        if (bytes_sent == -1)
        {
            std::cerr << "Failed to send message to fd " << client_fd << ": " << strerror(errno) << std::endl;
        }
        else if (static_cast<size_t>(bytes_sent) != message.length())
        {
            std::cerr << "Partial send to fd " << client_fd << ": " << bytes_sent << "/" << message.length() << " bytes" << std::endl;
        }
    }

    int broadcast_to_all(const std::string &message, int sender_fd)
    {
        int sent_count = 0;

        for (auto &[fd, info] : clients)
        {
            if (fd != sender_fd)
            { // Don't send back to sender
                ssize_t bytes_sent = send(fd, message.c_str(), message.length(), 0);
                if (bytes_sent == -1)
                {
                    std::cerr << "Failed to broadcast to " << info << " (fd " << fd << "): " << strerror(errno) << std::endl;
                }
                else
                {
                    sent_count++;
                }
            }
        }

        return sent_count;
    }

    void remove_client(int client_fd)
    {
        auto it = clients.find(client_fd);
        if (it != clients.end())
        {
            std::cout << "Removing client: " << it->second << " (fd: " << client_fd << ")" << std::endl;
            clients.erase(it);
        }

        // Remove from master set
        FD_CLR(client_fd, &master_fds);

        // Close the socket
        close(client_fd);

        // Update max_fd if necessary
        if (client_fd == max_fd)
        {
            max_fd = server_fd;
            for (const auto &[fd, info] : clients)
            {
                if (fd > max_fd)
                {
                    max_fd = fd;
                }
            }
        }

        std::cout << "Remaining clients: " << clients.size() << std::endl;
    }
};

int main()
{
    try
    {

        Select server("127.0.0.1", 12349);
        std::cout << "Select server listening on 127.0.0.1:12349" << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << std::endl;

        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}