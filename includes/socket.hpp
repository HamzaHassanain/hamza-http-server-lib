#pragma once

#include <string>

// Platform-specific includes
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include <socket_address.hpp>
#include <file_descriptor.hpp>
#include <data_buffer.hpp>
#include <utilities.hpp>
#include <exceptions.hpp>

#include <mutex>

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
     * Platform Support:
     * - Linux/Unix: Full support for all socket options
     * - Windows: Full support except TCP_QUICKACK (Windows-specific alternatives may vary)
     *
     * The class implements move-only semantics to ensure unique ownership of
     * socket resources and prevent accidental duplication or resource leaks.
     *
     * @note Uses explicit constructors to prevent implicit conversions
     * @note Move-only design prevents copying socket resources
     * @note Cross-platform compatibility with conditional compilation
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

        /// Mutex for thread-safe access
        // mutable std::mutex mtx;

        /**
         * @brief Create socket from existing file descriptor.
         * @param fd File descriptor to wrap
         * @param protocol Network protocol for the socket
         * @note This constructor is intended for internal use only.
         * @throws socket_exception with type "FileDescriptorValidation" if file descriptor is invalid
         */
        explicit socket(file_descriptor &&fd, const Protocol &protocol);

    public:
        /// Default constructor deleted - sockets must be explicitly configured
        socket() = delete;

        /**
         * @brief Create and bind socket to address.
         * @param protocol Network protocol (TCP/UDP)
         * @throws socket_exception with type "SocketCreation" if socket creation fails
         */
        explicit socket(const Protocol &protocol);

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

        /**
         * @brief Sets SO_BROADCAST socket option to enable/disable broadcast packets.
         * @param enable Whether to enable broadcast transmission (UDP only)
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * Allows UDP sockets to send broadcast messages to entire network segments.
         * Required for applications that need to send data to all hosts on a subnet.
         * Only applicable to UDP sockets - TCP doesn't support broadcast.
         */
        void set_broadcast(bool enable);

        /**
         * @brief Sets multicast loopback option for group communication.
         * @param enable Whether to enable multicast loopback
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * Controls whether multicast packets sent by this socket will be looped back
         * to the local host if it's a member of the multicast group.
         * Used in multicast applications for efficient group communication.
         */
        void set_multicast(bool enable);

        /**
         * @brief Sets IPV6_V6ONLY socket option to restrict IPv6 socket to IPv6 only.
         * @param enable Whether to enable IPv6-only mode
         * @throws socket_exception with type "ProtocolMismatch" if called on non-IPv6 socket
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * When enabled, prevents IPv6 sockets from accepting IPv4 connections.
         * By default, IPv6 sockets can handle both IPv4 and IPv6 connections.
         * Only applicable to IPv6 sockets.
         */
        void set_ipv6_only(bool enable);

        /**
         * @brief Sets socket to non-blocking or blocking mode.
         * @param enable Whether to enable non-blocking mode
         * @throws socket_exception with type "SocketOption" if operation fails
         *
         * Cross-platform implementation:
         * - Unix/Linux: Uses fcntl() with O_NONBLOCK flag
         * - Windows: Uses ioctlsocket() with FIONBIO
         *
         * Non-blocking sockets return immediately from I/O operations even if no data
         * is available, preventing the calling thread from being blocked.
         * Essential for implementing asynchronous I/O and event-driven servers.
         */
        void set_non_blocking(bool enable);

        /**
         * @brief Sets SO_KEEPALIVE socket option to enable TCP keep-alive probes.
         * @param enable Whether to enable keep-alive
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * When enabled, TCP automatically sends keep-alive packets to detect
         * dead connections and clean up resources for broken connections.
         * Only applicable to TCP sockets.
         */
        void set_keep_alive(bool enable);

        /**
         * @brief Sets SO_LINGER socket option to control connection termination behavior.
         * @param enable Whether to enable linger mode
         * @param timeout Linger timeout in seconds (ignored if enable is false)
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * Controls what happens to unsent data when socket is closed:
         * - When enabled with timeout > 0: close() blocks until data sent or timeout expires
         * - When enabled with timeout = 0: close() discards unsent data and sends RST
         * - When disabled: close() returns immediately, system handles data transmission
         */
        void set_linger(bool enable, int timeout);

        /**
         * @brief Sets SO_SNDBUF socket option to configure send buffer size.
         * @param size Send buffer size in bytes (must be positive)
         * @throws socket_exception with type "InvalidParameter" if size <= 0
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * Controls the size of the kernel's send buffer for this socket.
         * Larger buffers can improve performance but consume more memory.
         * The kernel may adjust the value to system limits.
         */
        void set_send_buffer_size(int size);

        /**
         * @brief Sets SO_RCVBUF socket option to configure receive buffer size.
         * @param size Receive buffer size in bytes (must be positive)
         * @throws socket_exception with type "InvalidParameter" if size <= 0
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * Controls the size of the kernel's receive buffer for this socket.
         * Larger buffers can prevent data loss in high-throughput scenarios.
         * The kernel may adjust the value to system limits.
         */
        void set_receive_buffer_size(int size);

        /**
         * @brief Sets TCP_NODELAY socket option to disable Nagle's algorithm.
         * @param enable Whether to disable Nagle's algorithm (enable immediate transmission)
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * Nagle's algorithm combines small packets to improve network efficiency
         * but can increase latency. When disabled, packets are sent immediately
         * regardless of size, reducing latency at cost of potential bandwidth efficiency.
         * Only applicable to TCP sockets.
         */
        void set_tcp_nodelay(bool enable);

        /**
         * @brief Sets TCP_QUICKACK socket option to enable quick ACK mode.
         * @param enable Whether to enable immediate ACK transmission
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "UnsupportedOption" if not available on platform
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * Platform Support:
         * - Linux: Supported via TCP_QUICKACK
         * - Windows: Not supported (throws UnsupportedOption)
         * - macOS/BSD: Not supported (throws UnsupportedOption)
         *
         * Controls whether TCP should send ACKs immediately or use delayed ACKs.
         * Can reduce latency in request-response scenarios but may increase network traffic.
         * Only applicable to TCP sockets.
         */
        void set_quick_ack(bool enable);

        /**
         * @brief Sets traffic class/Type of Service (ToS) for packet prioritization.
         * @param value Traffic class value (0-255)
         * @throws socket_exception with type "InvalidParameter" if value not in range 0-255
         * @throws socket_exception with type "SocketOption" if setsockopt fails
         *
         * For IPv4: Sets the ToS field in the IP header for QoS marking.
         * For IPv6: Sets the Traffic Class field for flow classification.
         * Used by routers for traffic prioritization and QoS policies.
         * Values typically follow DSCP standards.
         */
        void set_traffic_class(int value);

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
         * @param NON_BLOCKING Whether to use non-blocking accept for clients (default: true)
         * @return Shared pointer to new socket for the accepted connection
         * @throws socket_exception with type "ProtocolMismatch" if called on non-TCP socket
         * @throws socket_exception with type "SocketAcceptance" if accept operation fails
         */
        std::shared_ptr<socket> accept(bool NON_BLOCKING = true);

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