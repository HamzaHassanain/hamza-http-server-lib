#include <socket.hpp>
#include <file_descriptor.hpp>
#include <utilities.hpp>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
namespace hamza
{
    socket::socket(const socket_address &addr, const Protocol &protocol)
        : addr(addr), fd(socket_t(INVALID_SOCKET_VALUE))
    {
        int socket_file_descriptor = ::socket(addr.get_family().get(), static_cast<int>(protocol), 0);
        if (!is_valid_socket(socket_file_descriptor))
        {
            throw std::runtime_error("Failed to create socket.");
        }
        fd = file_descriptor(socket_file_descriptor);
        this->protocol = protocol;

        this->bind(addr); // Bind the socket to the address
    }
    socket::socket(const socket_address &addr, const Protocol &protocol, bool reuse)
        : addr(addr), fd(socket_t(INVALID_SOCKET_VALUE))
    {
        int socket_file_descriptor = ::socket(addr.get_family().get(), static_cast<int>(protocol), 0);
        if (!is_valid_socket(socket_file_descriptor))
        {
            throw std::runtime_error("Failed to create socket.");
        }
        this->fd = file_descriptor(socket_file_descriptor);
        this->protocol = protocol;
        this->set_reuse_address(reuse); // Set SO_REUSEADDR option
        this->bind(addr);               // Bind the socket to the address
    }
    socket::socket(const file_descriptor &fd, const Protocol &protocol) : addr(socket_address()), fd(fd), protocol(protocol)
    {

        if (!is_socket_open(fd.get()))
        {
            throw std::runtime_error("Invalid file descriptor for socket.");
        }

        this->protocol = protocol;
        this->fd = fd;
    }

    void socket::connect(const socket_address &server_address)
    {

        if (::connect(fd.get(), server_address.get_sock_addr(), server_address.get_sock_addr_len()) == SOCKET_ERROR_VALUE)
        {
            throw std::runtime_error("Failed to connect to address: " + std::string(strerror(errno)));
        }
    }

    void socket::bind(const socket_address &addr)
    {
        this->addr = addr;

        if (::bind(fd.get(), this->addr.get_sock_addr(), this->addr.get_sock_addr_len()) == SOCKET_ERROR_VALUE)
        {
            throw std::runtime_error("Failed to bind to address: " + std::string(strerror(errno)));
        }
    }

    void socket::listen(int backlog)
    {
        if (protocol != Protocol::TCP)
        {
            throw std::runtime_error("Listen is only supported for TCP sockets");
        }
        if (::listen(fd.get(), backlog) == SOCKET_ERROR_VALUE)
        {
            throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
        }
    }

    void socket::set_reuse_address(bool reuse)
    {
        int optval = reuse ? 1 : 0;
        if (setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == SOCKET_ERROR_VALUE)
        {
            throw std::runtime_error("Failed to set SO_REUSEADDR option: " + std::string(strerror(errno)));
        }
    }
    socket socket::accept()
    {
        if (protocol != Protocol::TCP)
        {
            throw std::runtime_error("Accept is only supported for TCP sockets");
        }

        sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        socket_t client_fd = ::accept(fd.get(), reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);
        if (!is_valid_socket(client_fd))
        {
            throw std::runtime_error("Failed to accept connection: " + std::string(strerror(errno)));
        }

        socket_address client_socket_address(client_addr, client_addr_len);
        socket new_socket(file_descriptor(client_fd), protocol);
        new_socket.addr = client_socket_address; // Set the address of the new socket
        return new_socket;
    }

    data_buffer socket::receive(socket_address &client_addr)
    {
        if (protocol != Protocol::UDP)
        {
            throw std::runtime_error("receive is only supported for UDP sockets");
        }

        sockaddr_storage sender_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        // Use a larger buffer for UDP - max UDP payload is ~65507 bytes
        char buffer[65536]; // 64KB buffer to handle max UDP size

        ssize_t bytes_received = ::recvfrom(fd.get(), buffer, sizeof(buffer), 0,
                                            reinterpret_cast<sockaddr *>(&sender_addr), &sender_addr_len);

        if (bytes_received == SOCKET_ERROR_VALUE)
        {
            throw std::runtime_error("Failed to receive data: " + std::string(strerror(errno)));
        }

        client_addr = socket_address(sender_addr, sender_addr_len);
        return data_buffer(buffer, static_cast<std::size_t>(bytes_received));
    }

    void socket::send_to(const socket_address &addr, const data_buffer &data)
    {
        if (protocol != Protocol::UDP)
        {
            throw std::runtime_error("send_to is only supported for UDP sockets");
        }

        ssize_t bytes_sent = ::sendto(fd.get(), data.data(), data.size(), 0,
                                      addr.get_sock_addr(), addr.get_sock_addr_len());

        if (bytes_sent == SOCKET_ERROR_VALUE)
        {
            throw std::runtime_error("Failed to send data: " + std::string(strerror(errno)));
        }

        if (static_cast<std::size_t>(bytes_sent) != data.size())
        {
            throw std::runtime_error("Partial send: only " + std::to_string(bytes_sent) +
                                     " of " + std::to_string(data.size()) + " bytes sent");
        }
    }

    void socket::send_on_connection(const data_buffer &data)
    {
        if (protocol != Protocol::TCP)
        {
            throw std::runtime_error("send is only supported for TCP sockets");
        }

        std::size_t total_sent = 0;
        const char *buffer = data.data();
        std::size_t data_size = data.size();

        while (total_sent < data_size)
        {
            // ::write
            ssize_t bytes_sent = ::write(fd.get(), buffer + total_sent, data_size - total_sent);

            if (bytes_sent == SOCKET_ERROR_VALUE)
            {
                throw std::runtime_error("Failed to write data: " + std::string(strerror(errno)));
            }

            total_sent += static_cast<std::size_t>(bytes_sent);
        }

        if (total_sent != data_size)
        {
            throw std::runtime_error("Partial write: only " + std::to_string(total_sent) +
                                     " of " + std::to_string(data_size) + " bytes sent");
        }
    }

    data_buffer socket::receive_on_connection()
    {
        if (protocol != Protocol::TCP)
        {
            throw std::runtime_error("receive_on_connection is only supported for TCP sockets");
        }

        data_buffer received_data;
        const int MAX_BUFFER_SIZE = 1024;
        char buffer[MAX_BUFFER_SIZE];
        int itrs = 0;
        int total_received = 0;
        while (true)
        {
            itrs++;
            ssize_t bytes_received = ::read(fd.get(), buffer, sizeof(buffer));

            if (bytes_received == SOCKET_ERROR_VALUE)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                throw std::runtime_error("Failed to read data: " + std::string(strerror(errno)));
            }

            if (bytes_received == 0)
            {
                break;
            }

            received_data.append(buffer, static_cast<std::size_t>(bytes_received));
            total_received += bytes_received;
            if (bytes_received < MAX_BUFFER_SIZE)
            {
                break;
            }
        }
        // std::cout << "Received " << itrs << " iterations of data." << std::endl;
        // std::cout << "Total bytes received: " << total_received << std::endl;
        return received_data;
    }

    void socket::disconnect()
    {
        try
        {

            if (fd.get() != INVALID_SOCKET_VALUE)
            {
                close_socket(fd.get());
                fd = file_descriptor(INVALID_SOCKET_VALUE);
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error disconnecting socket: " << e.what() << std::endl;
        }
    }

    bool socket::is_connected() const
    {
        if (fd.get() == INVALID_SOCKET_VALUE)
        {
            return false;
        }

        return is_socket_connected(fd.get());
    }

    socket_address socket::get_remote_address() const
    {
        return addr;
    }

    file_descriptor socket::get_file_descriptor() const
    {
        return fd;
    }

    socket::~socket()
    {
    }
}