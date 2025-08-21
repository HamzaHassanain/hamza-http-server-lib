#pragma once

#include <clients_container.hpp>
#include <socket_address.hpp>
#include <exceptions.hpp>
#include <socket.hpp>
#include <thread>
#include <functional>
#include <memory>
#include <cstring>
#include <select_server.hpp>
#include <mutex>
#include <atomic>

namespace hamza
{
    /**
     * @brief High-performance TCP server with asynchronous client management.
     *
     * This class provides a robust, thread-safe TCP server implementation using
     * the select() system call for efficient I/O multiplexing. It manages multiple
     * client connections concurrently and provides virtual callback methods for
     * handling various server events.
     *
     * The server uses a non-blocking approach with configurable timeouts and
     * automatic client cleanup. It implements RAII principles.
     *
     * @note This class is abstract and requires derived classes to implement
     *       the pure virtual callback methods
     * @note Thread-safe for connection management operations
     * @note Currently uses select() - will migrate to poll() in future versions
     */
    class tcp_server
    {

    private:
        /// I/O multiplexing handler using select() system call
        select_server fd_select_server;

        /// Container managing all active client connections
        clients_container clients;

        /// Server socket for accepting new connections
        std::unique_ptr<hamza::socket> server_socket;

        /// Atomic flag indicating server running state
        std::atomic<bool> running{false};

        /// Counter for currently waiting connections to be accepted
        std::atomic<int> current_waiting_to_be_accepted{0};

        /**
         * @brief Remove client from server and cleanup resources.
         * @param sock_ptr Client socket to remove
         * @throws socket_exception with type "TCP_SERVER_ClientRemoval" for removal errors
         * @note Automatically calls on_client_disconnect() callback
         * @note Thread-safe operation with internal error handling
         */
        void remove_client(std::shared_ptr<hamza::socket> sock_ptr);

        /**
         * @brief Check for activity on server and client sockets.
         * @throws socket_exception with type "TCP_SERVER_ActivityCheck" for activity check errors
         * @note Handles both new connections and client message reception
         */
        void check_for_activity();

    public:
        /**
         * @brief Construct TCP server bound to specified address.
         * @param addr Socket address to bind server to
         * @param timeout_seconds Timeout duration in seconds for select() calls
         * @param timeout_microseconds Timeout duration in microseconds for select() calls
         * @throws socket_exception with type "SocketCreation" if socket creation fails
         * @throws socket_exception with type "SocketBinding" if binding fails
         * @throws socket_exception with type "SocketListening" if listen operation fails
         * @note Automatically configures select server with specified timeouts
         */
        explicit tcp_server(const hamza::socket_address &addr, int timeout_seconds = 1, int timeout_microseconds = 0);

        // Copy and move operations - DELETED for resource safety
        tcp_server(const tcp_server &) = delete;
        tcp_server &operator=(const tcp_server &) = delete;
        tcp_server(tcp_server &&) = delete;
        tcp_server &operator=(tcp_server &&) = delete;

        /**
         * @brief Get the local address of the server.
         * @return Local socket address
         */
        socket_address get_remote_address() const;

        /**
         * @brief Start the server and begin listening for connections.
         *
         * Begins the main server loop, handling new connections and client
         * communication. This is a blocking call that continues until
         * set_running_status(false) is called.
         *
         * @throws socket_exception with type "TCP_SERVER_Loop" for general server errors
         * @throws socket_exception with type "TCP_SERVER_Accept" for connection acceptance errors
         * @throws socket_exception with type "TCP_SERVER_ClientActivity" for client handling errors
         * @note Calls on_server_listen() before starting and on_server_stopped() when finished
         */
        void listen();

        /**
         * @brief Stop the TCP server.
         * @param server Shared pointer to the TCP server instance
         * @note Safely stops the server and cleans up resources
         */
        void stop();

    protected:
        /**
         * @brief Close client connection safely.
         * @param sock_ptr Client socket to close
         * @note Thread-safe operation using internal mutex
         * @note Calls remove_client() internally for cleanup
         */
        void close_connection(std::shared_ptr<hamza::socket> sock_ptr);

        /**
         * @brief Close server socket safely.
         * @throw socket_exception with type "TCP_SERVER_SocketClose" for closure errors
         * @throw socket_exception with type "TCP_SERVER_CloseWhileRunning" if you close while the server is still running
         */
        void close_server_socket();

        /**
         * @brief Handle incoming message from client (pure virtual).
         * @param sock_ptr Client socket that sent the message
         * @param message Received data buffer
         * @note Must be implemented by derived classes
         */
        virtual void on_message_received(std::shared_ptr<hamza::socket> sock_ptr, const hamza::data_buffer &message) = 0;

        /**
         * @brief Handle server exceptions (pure virtual).
         * @param e Socket exception that occurred
         * @note Must be implemented by derived classes
         * @note Called for all server-level errors
         */
        virtual void on_exception(std::shared_ptr<hamza::socket_exception> e) = 0;

        /**
         * @brief Called when server is waiting for activity.
         * @note Default implementation does nothing
         * @note Called when the server is idle and waiting for client activity
         */
        virtual void on_waiting_for_activity();

        /**
         * @brief Handle new client connection (virtual).
         * @param sock_ptr Newly connected client socket
         * @note Default implementation does nothing
         * @note Called when a new client connects to the server
         */
        virtual void on_client_connected(std::shared_ptr<hamza::socket> sock_ptr);

        /**
         * @brief Handle client disconnection (virtual).
         * @param sock_ptr Client socket that disconnected
         * @note Can be implemented by derived classes
         * @note Default implementation does nothing
         * @note Called when a client disconnects from the server
         */
        virtual void on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr);

        /**
         * @brief Called when server starts listening (virtual).
         * @note Can be implemented by derived classes
         * @note Default implementation does nothing
         * @note Called once before entering main server loop
         */
        virtual void on_server_listen() {};

        /**
         * @brief Called when server stops (virtual).
         * @note Can be implemented by derived classes
         * @note Default implementation does nothing
         * @note Called after exiting main server loop
         */
        virtual void on_server_stopped() {};

        /**
         * @brief Get current server running status.
         * @return true if server is running, false otherwise
         * @note Thread-safe atomic operation
         */
        bool get_running_status() const;

        /**
         * @brief Set server running status.
         * @param status New running status (false to stop server)
         * @note Thread-safe atomic operation
         * @note Setting to false will cause server loop to exit gracefully
         */
        void set_running_status(bool status);

    private:
        /**
         * @brief Main server execution loop.
         * @throws socket_exception with type "TCP_SERVER_Loop" for loop execution errors
         * @note Handles select() calls and dispatches to activity handlers
         * @note Continues until running flag is set to false
         */
        void run();

        /**
         * @brief Handle activity on specific client socket.
         * @param sock_ptr Client socket with pending activity
         * @throws socket_exception with type "TCP_SERVER_ClientActivity" for client handling errors
         * @note Receives data and calls on_message_received() callback
         */
        void handle_client_activity(std::shared_ptr<hamza::socket> sock_ptr);

        /**
         * @brief Handle new incoming connection.
         * @throws socket_exception with type "TCP_SERVER_NewConnection" for connection errors
         * @throws socket_exception with type "TCP_SERVER_Accept" for accept operation errors
         * @note Accepts connection, adds to client container, and calls on_client_connected()
         */
        void handle_new_connection();
    };
};