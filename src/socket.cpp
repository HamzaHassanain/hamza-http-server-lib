#include <socket.hpp>
#include <file_descriptor.hpp>
#include <utilities.hpp>
#include <exceptions.hpp>
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
    /**
     * Creates a new socket and binds it to the specified address.
     * Uses ::socket() system call to create socket with given family and protocol.
     * Validates socket creation and automatically binds to the provided address.
     */
    socket::socket(const socket_address &addr, const Protocol &protocol)
        : addr(addr)
    {
        // Create socket: ::socket(domain, type, protocol)
        // domain: AF_INET (IPv4) or AF_INET6 (IPv6)
        // type: SOCK_STREAM (TCP) or SOCK_DGRAM (UDP)
        // protocol: Usually 0 for default protocol
        int socket_file_descriptor = ::socket(addr.get_family().get(), static_cast<int>(protocol), 0);

        // Check if socket creation succeeded (returns -1 on failure)
        if (!is_valid_socket(socket_file_descriptor))
        {
            throw socket_exception("Invalid File Descriptor", "SocketCreation", __func__);
        }

        fd = file_descriptor(socket_file_descriptor);
        this->protocol = protocol;
        this->bind(addr); // Bind the socket to the address
    }

    /**
     * Creates a new socket with SO_REUSEADDR option and binds it to the specified address.
     * SO_REUSEADDR allows reusing addresses that are in TIME_WAIT state.
     * Useful for servers that need to restart quickly without "Address already in use" errors.
     */
    socket::socket(const socket_address &addr, const Protocol &protocol, bool reuse)
        : addr(addr), fd(socket_t(INVALID_SOCKET_VALUE))
    {
        // Create socket with same parameters as above
        int socket_file_descriptor = ::socket(addr.get_family().get(), static_cast<int>(protocol), 0);

        if (!is_valid_socket(socket_file_descriptor))
        {
            throw socket_exception("Invalid File Descriptor", "SocketCreation", __func__);
        }

        this->fd = file_descriptor(socket_file_descriptor);
        this->protocol = protocol;
        this->set_reuse_address(reuse); // Set SO_REUSEADDR option before binding
        this->bind(addr);               // Bind the socket to the address
    }

    /**
     * Creates a socket wrapper from an existing file descriptor.
     * Used internally when accepting connections or wrapping existing sockets.
     * Validates that the file descriptor represents a valid open socket.
     */
    socket::socket(file_descriptor &&fd, const Protocol &protocol)
        : addr(socket_address()), fd(std::move(fd)), protocol(protocol)
    {
        // Verify the file descriptor is valid and represents an open socket
        if (!is_socket_open(this->fd.get()))
        {
            throw socket_exception("Invalid file descriptor for socket", "FileDescriptorValidation", __func__);
        }

        this->protocol = protocol;
    }

    /**
     * Establishes a connection to a remote server (TCP only).
     * Uses ::connect() system call to initiate connection to server_address.
     * For TCP: Creates a reliable, connection-oriented communication channel.
     * For UDP: This would set the default destination for send/receive operations.
     */
    void socket::connect(const socket_address &server_address)
    {
        // ::connect(sockfd, addr, addrlen) - connect socket to remote address
        // Returns 0 on success, -1 on error (sets errno)
        if (::connect(fd.get(), server_address.get_sock_addr(), server_address.get_sock_addr_len()) == SOCKET_ERROR_VALUE)
        {
            throw socket_exception("Failed to connect to address: " + std::string(strerror(errno)), "SocketConnection", __func__);
        }
    }

    /**
     * Binds the socket to a local address and port.
     * Uses ::bind() system call to associate socket with specific address.
     * Required for servers to listen on specific port, optional for clients.
     * For UDP servers: Necessary to receive packets on specific port.
     */
    void socket::bind(const socket_address &addr)
    {
        this->addr = addr;

        // ::bind(sockfd, addr, addrlen) - bind socket to local address
        // Returns 0 on success, -1 on error
        // Common errors: EADDRINUSE (port in use), EACCES (permission denied)
        if (::bind(fd.get(), this->addr.get_sock_addr(), this->addr.get_sock_addr_len()) == SOCKET_ERROR_VALUE)
        {
            throw socket_exception("Failed to bind to address: " + std::string(strerror(errno)), "SocketBinding", __func__);
        }
    }

    /**
     * Puts TCP socket into listening mode to accept incoming connections.
     * Uses ::listen() system call to mark socket as passive (server) socket.
     * backlog parameter specifies maximum number of pending connections in queue.
     * Only valid for TCP sockets (SOCK_STREAM).
     */
    void socket::listen(int backlog)
    {
        // Verify this is a TCP socket - UDP doesn't support listen/accept
        if (protocol != Protocol::TCP)
        {
            throw socket_exception("Listen is only supported for TCP sockets", "ProtocolMismatch", __func__);
        }

        // ::listen(sockfd, backlog) - listen for connections
        // backlog: max number of pending connections (typically SOMAXCONN)
        // Returns 0 on success, -1 on error
        if (::listen(fd.get(), backlog) == SOCKET_ERROR_VALUE)
        {
            throw socket_exception("Failed to listen on socket: " + std::string(strerror(errno)), "SocketListening", __func__);
        }
    }

    /**
     * Sets SO_REUSEADDR socket option to allow address reuse.
     * Prevents "Address already in use" errors when restarting servers.
     * Uses setsockopt() system call to modify socket behavior.
     * Should be called before bind() to be effective.
     */
    void socket::set_reuse_address(bool reuse)
    {
        int optval = reuse ? 1 : 0;

        // setsockopt(sockfd, level, optname, optval, optlen) - set socket option
        // SOL_SOCKET: socket level options
        // SO_REUSEADDR: allow reuse of local addresses
        // Returns 0 on success, -1 on error
        if (setsockopt(fd.get(), SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == SOCKET_ERROR_VALUE)
        {
            throw socket_exception("Failed to set SO_REUSEADDR option: " + std::string(strerror(errno)), "SocketOption", __func__);
        }
    }

    /**
     * Accepts an incoming TCP connection and creates a new socket for it.
     * Uses ::accept() system call to extract first pending connection.
     * Returns a new socket object representing the client connection.
     * Original socket remains in listening state for more connections.
     */
    std::shared_ptr<socket> socket::accept()
    {
        // Verify this is a TCP socket - UDP doesn't have connections to accept
        if (protocol != Protocol::TCP)
        {
            throw socket_exception("Accept is only supported for TCP sockets", "ProtocolMismatch", __func__);
        }

        sockaddr_storage client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        // ::accept(sockfd, addr, addrlen) - accept pending connection
        // Returns new socket descriptor for the connection, -1 on error
        // Fills client_addr with client's address information
        socket_t client_fd = ::accept(fd.get(), reinterpret_cast<sockaddr *>(&client_addr), &client_addr_len);

        if (!is_valid_socket(client_fd))
        {
            throw socket_exception("Failed to accept connection: " + std::string(strerror(errno)), "SocketAcceptance", __func__);
        }

        // Create new socket object for the accepted connection
        socket_address client_socket_address(client_addr);
        socket new_socket(file_descriptor(client_fd), protocol);
        new_socket.addr = client_socket_address;
        return std::make_shared<socket>(std::move(new_socket));
    }

    /**
     * Receives data from any client via UDP socket.
     * Uses ::recvfrom() system call to receive datagram and sender information.
     * UDP is connectionless - can receive from any sender.
     * Buffer size is set to 64KB to handle maximum UDP payload size.
     */
    data_buffer socket::receive(socket_address &client_addr)
    {
        // Verify this is a UDP socket - TCP uses receive_on_connection()
        if (protocol != Protocol::UDP)
        {
            throw socket_exception("receive is only supported for UDP sockets", "ProtocolMismatch", __func__);
        }

        sockaddr_storage sender_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        // Use 64KB buffer for UDP - theoretical max UDP payload is 65507 bytes
        char buffer[65536];

        // ::recvfrom(sockfd, buf, len, flags, src_addr, addrlen) - receive datagram
        // Returns number of bytes received, -1 on error
        // Fills sender_addr with sender's address information
        ssize_t bytes_received = ::recvfrom(fd.get(), buffer, sizeof(buffer), 0,
                                            reinterpret_cast<sockaddr *>(&sender_addr), &sender_addr_len);

        if (bytes_received == SOCKET_ERROR_VALUE)
        {
            throw socket_exception("Failed to receive data: " + std::string(strerror(errno)), "SocketReceive", __func__);
        }

        // Extract sender's address and return received data
        client_addr = socket_address(sender_addr);
        return data_buffer(buffer, static_cast<std::size_t>(bytes_received));
    }

    /**
     * Sends data to specific address via UDP socket.
     * Uses ::sendto() system call to send datagram to specified destination.
     * UDP is connectionless - each send specifies destination address.
     * Verifies all data was sent in single operation.
     */
    void socket::send_to(const socket_address &addr, const data_buffer &data)
    {
        // Verify this is a UDP socket - TCP uses send_on_connection()
        if (protocol != Protocol::UDP)
        {
            throw socket_exception("send_to is only supported for UDP sockets", "ProtocolMismatch", __func__);
        }

        // ::sendto(sockfd, buf, len, flags, dest_addr, addrlen) - send datagram
        // Returns number of bytes sent, -1 on error
        ssize_t bytes_sent = ::sendto(fd.get(), data.data(), data.size(), 0,
                                      addr.get_sock_addr(), addr.get_sock_addr_len());

        if (bytes_sent == SOCKET_ERROR_VALUE)
        {
            throw socket_exception("Failed to send data: " + std::string(strerror(errno)), "SocketSend", __func__);
        }

        // UDP should send all data in one operation - partial sends indicate problems
        if (static_cast<std::size_t>(bytes_sent) != data.size())
        {
            throw socket_exception("Partial send: only " + std::to_string(bytes_sent) +
                                       " of " + std::to_string(data.size()) + " bytes sent",
                                   "PartialSend", __func__);
        }
    }

    /**
     * Sends data over established TCP connection.
     * Uses ::write() system call in loop to ensure all data is transmitted.
     * TCP may send data in multiple chunks, so loop until all data sent.
     * Tracks total bytes sent to detect partial transmission issues.
     */
    void socket::send_on_connection(const data_buffer &data)
    {
        // Verify this is a TCP socket - UDP uses send_to()
        if (protocol != Protocol::TCP)
        {
            throw socket_exception("send is only supported for TCP sockets", "ProtocolMismatch", __func__);
        }

        std::size_t total_sent = 0;
        const char *buffer = data.data();
        std::size_t data_size = data.size();

        // TCP may require multiple send operations for large data
        while (total_sent < data_size)
        {
            // ::write(fd, buf, count) - write data to file descriptor
            // Returns number of bytes written, -1 on error
            // May write less than requested (partial write)
            ssize_t bytes_sent = ::write(fd.get(), buffer + total_sent, data_size - total_sent);

            if (bytes_sent == SOCKET_ERROR_VALUE)
            {
                throw socket_exception("Failed to write data: " + std::string(strerror(errno)), "SocketWrite", __func__);
            }

            total_sent += static_cast<std::size_t>(bytes_sent);
        }

        // Verify all data was eventually sent
        if (total_sent != data_size)
        {
            throw socket_exception("Partial write: only " + std::to_string(total_sent) +
                                       " of " + std::to_string(data_size) + " bytes sent",
                                   "PartialWrite", __func__);
        }
    }

    /**
     * Receives data from established TCP connection.
     * Uses ::read() system call in loop to receive all available data.
     * Continues reading until no more data available or connection closed.
     * Handles non-blocking sockets by checking for EAGAIN/EWOULDBLOCK.
     */
    data_buffer socket::receive_on_connection()
    {
        // Verify this is a TCP socket - UDP uses receive()
        if (protocol != Protocol::TCP)
        {
            throw socket_exception("receive_on_connection is only supported for TCP sockets", "ProtocolMismatch", __func__);
        }

        data_buffer received_data;
        char buffer[DEFAULT_BUFFER_SIZE];
        int itrs = 0;
        int total_received = 0;

        // Read data in chunks until no more data available
        while (true)
        {
            itrs++;
            // ::read(fd, buf, count) - read data from file descriptor
            // Returns number of bytes read, 0 on EOF, -1 on error
            ssize_t bytes_received = ::read(fd.get(), buffer, sizeof(buffer));

            if (bytes_received == SOCKET_ERROR_VALUE)
            {
                // For non-blocking sockets, EAGAIN/EWOULDBLOCK means no data available
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                throw socket_exception("Failed to read data: " + std::string(strerror(errno)), "SocketRead", __func__);
            }

            // bytes_received == 0 indicates connection closed by peer
            if (bytes_received == 0)
            {
                break;
            }

            received_data.append(buffer, static_cast<std::size_t>(bytes_received));
            total_received += bytes_received;

            // If we received less than buffer size, likely no more data available
            if (bytes_received < DEFAULT_BUFFER_SIZE)
            {
                break;
            }
        }

        return received_data;
    }

    /**
     * Safely closes the socket connection and releases system resources.
     * Uses close_socket() utility function to properly close the file descriptor.
     * Invalidates the file descriptor to prevent further use.
     * Throws socket_exception on error.
     */
    void socket::disconnect()
    {
        try
        {
            if (fd.get() != INVALID_SOCKET_VALUE)
            {
                close_socket(fd.get()); // Close the socket file descriptor
                fd.invalidate();        // Mark file descriptor as invalid
            }
        }
        catch (const std::exception &e)
        {
            throw socket_exception("Error disconnecting socket: " + std::string(e.what()), "SocketDisconnect", __func__);
        }
    }

    /**
     * Checks if the socket is currently connected and valid.
     * First verifies the file descriptor is valid.
     * Then uses utility function to check actual connection status.
     * For TCP: checks if connection is established.
     * For UDP: checks if socket is bound and operational.
     */
    bool socket::is_connected() const
    {
        // First check if file descriptor is valid
        if (fd.get() == INVALID_SOCKET_VALUE)
        {
            return false;
        }

        // Use utility function to check actual socket connection status
        return is_socket_connected(fd.get());
    }

    /**
     * returns the bound local address.
     */
    socket_address socket::get_remote_address() const
    {
        return addr;
    }

    /**
     * Returns the raw file descriptor value for advanced operations.
     * Should be used carefully as it bypasses the wrapper's safety mechanisms.
     */
    int socket::get_file_descriptor_raw_value() const
    {
        return fd.get();
    }

    /**
     * No explicit resource management needed here, we trust the socket user to close the socket, when done
     */
    socket::~socket()
    {
        // No explicit resource management needed here, we trust the socket user to close the socket, when done
    }
}