#pragma once

#include <string>
#include <netinet/in.h>

#include <socket_address.hpp>
#include <file_descriptor.hpp>
#include <data_buffer.hpp>
#include <utilities.hpp>
#include <exceptions.hpp>

namespace hamza
{
    /**
     * @brief Cross-platform socket wrapper for TCP and UDP network operations.
     *
     * This class provides a high-level, RAII-compliant interface for socket programming.
     * It abstracts system-level socket operations and handles resource management
     * automatically. Supports both TCP and UDP protocols with separate method sets
     * for connection-oriented and connectionless operations.
     *
     * The class implements move-only semantics to ensure unique ownership of
     * socket resources and prevent accidental duplication or resource leaks.
     *
     * @note Uses explicit constructors to prevent implicit conversions
     * @note Move-only design prevents copying socket resources
     * @note Automatically handles socket cleanup in destructor
     */
    class socket
    {
    private:
        /// Socket address (IP, port, family)
        socket_address addr;

        /// Platform-specific file descriptor wrapper
        file_descriptor fd;

        /// Protocol type (TCP or UDP)
        Protocol protocol;

        /**
         * @brief Create socket from existing file descriptor.
         * @param fd File descriptor to wrap
         * @param protocol Network protocol for the socket
         * @note This constructor is intended for internal use only.
         * @throws socket_exception with type "FileDescriptorValidation" if file descriptor is invalid
         */
        explicit socket(file_descriptor &&fd, const Protocol &protocol);

        /**
         * @brief Binds socket to the specified address.
         * @param addr Address to bind to
         * @throws socket_exception with type "SocketBinding" if bind operation fails
         */
        void bind(const socket_address &addr);

        /**
         * @brief Sets SO_REUSEADDR socket option.
         * @param reuse Whether to enable address reuse
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         */
        void set_reuse_address(bool reuse);

    public:
        /// Default constructor deleted - sockets must be explicitly configured
        socket() = delete;

        /**
         * @brief Create and bind socket to address.
         * @param addr Socket address to bind to
         * @param protocol Network protocol (TCP/UDP)
         * @throws socket_exception with type "SocketCreation" if socket creation fails
         * @throws socket_exception with type "SocketBinding" if binding fails
         */
        explicit socket(const socket_address &addr, const Protocol &protocol);

        /**
         * @brief Create and bind socket with address reuse option.
         * @param addr Socket address to bind to
         * @param protocol Network protocol (TCP/UDP)
         * @param reuse Enable SO_REUSEADDR option
         * @throws socket_exception with type "SocketCreation" if socket creation fails
         * @throws socket_exception with type "SocketOption" if SO_REUSEADDR setting fails
         * @throws socket_exception with type "SocketBinding" if binding fails
         */
        explicit socket(const socket_address &addr, const Protocol &protocol, bool reuse);

        // Copy operations - DELETED for resource safety
        socket(const socket &) = delete;
        socket &operator=(const socket &) = delete;

        /**
         * @brief Move constructor.
         * @param other Socket to move from
         *
         * Transfers ownership of socket resources. Source socket becomes invalid.
         */
        socket(socket &&other)
            : addr(std::move(other.addr)), fd(std::move(other.fd)), protocol(other.protocol) {}

        /**
         * @brief Move assignment operator.
         * @param other Socket to move from
         * @return Reference to this socket
         *
         * Transfers ownership of socket resources from another socket.
         */
        socket &operator=(socket &&other)
        {
            if (this != &other)
            {
                addr = std::move(other.addr);
                fd = std::move(other.fd);
                protocol = other.protocol;
            }
            return *this;
        }

        /**
         * @brief Connect to remote address (TCP only).
         * @param addr Remote address to connect to
         * @throws socket_exception with type "SocketConnection" if connection fails
         */
        void connect(const socket_address &addr);

        /**
         * @brief Start listening for connections (TCP only).
         * @param backlog Maximum number of pending connections
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "SocketListening" if listen operation fails
         */
        void listen(int backlog = SOMAXCONN);

        /**
         * @brief Accept incoming connection (TCP only).
         * @return Shared pointer to new socket for the accepted connection
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "SocketAcceptance" if accept operation fails
         */
        std::shared_ptr<socket> accept();

        /**
         * @brief Receive data from any client (UDP only).
         * @param client_addr Will be filled with sender's address
         * @return Buffer containing received data
         * @throws socket_exception with type "ProtocolMismatch" if called on non-UDP socket
         * @throws socket_exception with type "SocketReceive" if receive operation fails
         */
        data_buffer receive(socket_address &client_addr);

        /**
         * @brief Send data to specific address (UDP only).
         * @param addr Destination address
         * @param data Data to send
         * @throws socket_exception with type "ProtocolMismatch" if called on non-UDP socket
         * @throws socket_exception with type "SocketSend" if send operation fails
         * @throws socket_exception with type "PartialSend" if not all data was sent
         */
        void send_to(const socket_address &addr, const data_buffer &data);

        /**
         * @brief Get remote endpoint address.
         * @return Socket address of remote endpoint
         */
        socket_address get_remote_address() const;

        /**
         * @brief Get raw file descriptor value.
         * @return Integer file descriptor value
         */
        int get_file_descriptor_raw_value() const;

        /**
         * @brief Send data on established connection (TCP only).
         * @param data Data buffer to send
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "SocketWrite" if write operation fails
         * @throws socket_exception with type "PartialWrite" if not all data was sent
         */
        void send_on_connection(const data_buffer &data);

        /**
         * @brief Receive data from established connection (TCP only).
         * @return Buffer containing received data
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "SocketRead" if read operation fails
         */
        data_buffer receive_on_connection();

        /**
         * @brief Close the socket connection.
         *
         * Safely closes the socket and releases system resources.
         * Socket becomes unusable after this call.
         * @note This method does not throw exceptions - errors are logged to stderr
         */
        void disconnect();

        /**
         * @brief Check if socket is connected.
         * @return true if socket has valid connection, false otherwise
         */
        bool is_connected() const;

        /**
         * @brief Less-than operator for container ordering.
         * @param other Socket to compare with
         * @return true if this socket's file descriptor is less than other's
         */
        bool operator<(const socket &other) const
        {
            return fd < other.fd;
        }

        /// Destructor - automatically handles resource cleanup
        ~socket();
    };
}