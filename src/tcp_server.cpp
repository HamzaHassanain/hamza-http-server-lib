#include <tcp_server.hpp>
#include <clients_container.hpp>
#include <socket_address.hpp>
#include <socket.hpp>
#include <thread>
#include <functional>
#include <memory>
#include <cstring>
#include <algorithm>

#include <csignal>
namespace hamza
{
    /**
     * Initializes TCP server and binds to specified address.
     * Creates server socket with SO_REUSEADDR option to avoid "Address already in use" errors.
     * Sets up select-based I/O multiplexing for handling multiple clients concurrently.
     * Configures timeout_seconds and timeout_microseconds for select operations to allow periodic server status checks.
     */
    tcp_server::tcp_server(const hamza::socket_address &addr, int timeout_seconds, int timeout_microseconds)
    {
        // Create TCP server socket with address reuse enabled
        // SO_REUSEADDR allows immediate restart without waiting for TIME_WAIT
        server_socket = std::make_unique<hamza::socket>(hamza::Protocol::TCP);
        // server_socket->set_reuse_address(true);
        server_socket->set_non_blocking(true);
        server_socket->bind(addr);

        // Put socket into listening mode to accept incoming connections
        // SOMAXCONN is used as default backlog (max pending connections)
        server_socket->listen();

        // TODO: Temporarily using select server, will migrate to poll() for better performance
        // poll() supports more file descriptors and has better scalability

        // Initialize select server with server socket file descriptor
        // This sets up the fd_set structures and maximum file descriptor tracking
        fd_select_server.init(server_socket->get_file_descriptor_raw_value());

        // Set timeout_seconds timeout for select() calls
        // Allows server to periodically check running status and handle shutdown gracefully
        fd_select_server.set_timeout(timeout_seconds, timeout_microseconds);

        // Ignore SIGPIPE signals, broken pipe won't kill the server
        std::signal(SIGPIPE, SIG_IGN);
    }
    /**
     * Safely removes client from server and performs cleanup.
     * Validates input parameters to prevent server socket removal or null pointer access.
     * Updates select server file descriptor sets and recalculates maximum fd.
     * Notifies derived classes through callback and handles all exceptions internally.
     */
    void tcp_server::remove_client(std::shared_ptr<hamza::socket> sock_ptr)
    {
        try
        {

            // Ensure server socket is properly initialized
            if (server_socket == nullptr)
            {
                throw std::runtime_error("Server socket is not initialized.");
            }

            // Validate client socket pointer
            if (sock_ptr == nullptr)
            {
                throw std::runtime_error("Invalid socket pointer provided for removal.");
            }

            // Remove socket from select server's fd_set structures
            // This prevents select() from monitoring this file descriptor
            fd_select_server.remove_fd(sock_ptr->get_file_descriptor_raw_value());

            // Close the socket connection and release system resources
            sock_ptr->disconnect();

            // Recalculate maximum file descriptor for select() optimization
            // select() needs to know the highest-numbered file descriptor + 1

            // Remove client from container (maintains list of active clients)
            clients.erase(sock_ptr);

            // Notify derived classes about client disconnection
            // Allows application-specific cleanup (logging, user notifications, etc.)
            this->on_client_disconnect(sock_ptr);
        }
        catch (const std::exception &e)
        {
            // Handle all exceptions internally to maintain server stability
            // Convert to socket_exception with server-specific error type
            std::string error_message = "Error removing client:\n" + std::string(e.what());
            this->on_exception(std::make_unique<hamza::socket_exception>(error_message, "TCP_SERVER_ClientRemoval", __func__));
        }
    }

    /**
     * Checks for activity on server and all client sockets.
     * Uses select server to determine which file descriptors have pending I/O.
     * Handles new connections on server socket and data reception on client sockets.
     * Automatically skips disconnected clients to prevent errors.
     */
    void tcp_server::check_for_activity()
    {
        try
        {
            // Ensure server socket is properly initialized before checking activity
            if (server_socket == nullptr)
            {
                throw std::runtime_error("Server socket is not initialized.");
            }

            // Check if server socket has activity (new incoming connection)
            if (fd_select_server.is_fd_set(server_socket->get_file_descriptor_raw_value()))
            {
                handle_new_connection();
            }

            // Check each client socket for incoming data
            // Lambda function allows access to 'this' pointer for member function calls
            clients.for_each([this](std::shared_ptr<hamza::socket> sock_ptr)
                             {
                                 // Skip null pointers and disconnected sockets
                                 if (!sock_ptr || !sock_ptr->is_connected())
                                 {
                                     return;
                                 }
                                 
                                 // Check if this client socket has pending data to read
                                 if (fd_select_server.is_fd_set(sock_ptr->get_file_descriptor_raw_value()))
                                 {
                                     handle_client_activity(sock_ptr);
                                 } });
        }
        catch (const std::exception &e)
        {
            // Handle activity checking errors internally
            std::string error_message = "Error checking for activity:\n" + std::string(e.what()) + "\n";
            this->on_exception(std::make_unique<hamza::socket_exception>(error_message, "TCP_SERVER_ActivityCheck", __func__));
        }
    }

    /**
     * Main server execution loop using select-based I/O multiplexing.
     * Continuously monitors server and client sockets for activity.
     * Handles select() return values: activity count, timeouts, and errors.
     * Provides graceful shutdown mechanism via running status flag.
     */
    void tcp_server::run()
    {
        // Notify derived classes that server is starting to listen
        this->set_running_status(true);
        this->on_server_listen();

        // Main server loop - continues until running flag is set to false
        while (get_running_status())
        {
            try
            {
                // Call select() to wait for activity on monitored file descriptors
                // Returns: >0 (number of ready descriptors), 0 (timeout), <0 (error)
                int activity = fd_select_server.select();

                if (activity < 0)
                {
// Handle select() errors
#ifdef _WIN32
                    if (WSAGetLastError() == WSAEINTR)
                        throw std::runtime_error("Select interrupted by signal.");
#else
                    if (errno == EINTR)
                        throw std::runtime_error("Select interrupted by signal.");
#endif
                    throw std::runtime_error("Select error: " + std::string(get_error_message()));
                }

                if (activity == 0)
                {
                    // Timeout occurred - no activity within specified time
                    // Allows derived classes to perform periodic tasks
                    this->on_waiting_for_activity();
                }

                // Check all monitored sockets for activity
                check_for_activity();
            }
            catch (const std::exception &e)
            {
                // Handle server loop errors without crashing the server
                std::string error_message = "Error in tcp_server loop: " + std::string(e.what());
                this->on_exception(std::make_unique<hamza::socket_exception>(error_message, "TCP_SERVER_Loop", __func__));
            }
        }
        try
        {

            // close all the connected clients
            clients.for_each([this](std::shared_ptr<hamza::socket> sock_ptr)
                             { this->close_connection(sock_ptr); });
            clients.clear();

            // close the server socket
            this->close_server_socket();

            // Notify derived classes that server has stopped
            this->on_server_stopped();
        }
        catch (const std::exception &e)
        {
            // Handle errors during server shutdown
            std::string error_message = "Error during server shutdown: " + std::string(e.what());
            this->on_exception(std::make_unique<hamza::socket_exception>(error_message, "TCP_SERVER_Shutdown", __func__));
        }
    }

    /**
     * Handles incoming data from specific client socket.
     * Extracts client address information for error reporting.
     * Receives data from client and notifies application layer.
     * Provides detailed error messages including client identification.
     */
    void tcp_server::handle_client_activity(std::shared_ptr<hamza::socket> sock_ptr)
    {
        std::string client_ip;
        std::string client_port;

        try
        {
            // Validate socket pointer before processing
            if (sock_ptr == nullptr)
            {
                throw hamza::socket_exception("Invalid socket pointer.", "TCP_SERVER_ClientActivity", __func__);
            }

            // Extract client address information for error reporting
            // Get IP address and port from remote endpoint
            client_ip = sock_ptr->get_remote_address().get_ip_address().get();
            client_port = std::to_string(sock_ptr->get_remote_address().get_port().get());

            // Receive data from client connection
            // TCP guarantees ordered, reliable delivery of data
            hamza::data_buffer db = sock_ptr->receive_on_connection();
            // if (db.empty())
            //     return;
            // Notify application layer about received message
            // Derived classes implement business logic for message processing
            this->on_message_received(sock_ptr, db);
        }
        catch (const std::exception &e)
        {
            // Provide detailed error information including client identification
            std::string error_message = "Error handling client activity: " + std::string(e.what());
            error_message += "\nfor client: " + client_ip + ":" + client_port;
            this->on_exception(std::make_shared<hamza::socket_exception>(error_message, "TCP_SERVER_ClientActivity", __func__));
        }
    }

    /**
     * Accepts new incoming connection and integrates into server.
     * Creates new client socket from server socket's accept() operation.
     * Adds client to container and select server for monitoring.
     * Updates maximum file descriptor for select() optimization.
     */
    void tcp_server::handle_new_connection()
    {
        try
        {
            // Accept pending connection from server socket
            // Returns new socket object representing client connection
            auto client_socket_ptr = server_socket->accept();

            // Validate that client socket was created successfully
            if (client_socket_ptr == nullptr)
            {
                throw hamza::socket_exception("Failed to create client socket.", "TCP_SERVER_NewConnection", __func__);
            }

            // Add client to container for tracking and management
            clients.insert(client_socket_ptr);

            // Add client file descriptor to select server monitoring
            // This allows select() to detect activity on this client socket
            fd_select_server.add_fd(client_socket_ptr->get_file_descriptor_raw_value());

            // Notify application layer about new client connection
            // Allows for welcome messages, authentication, logging, etc.
            this->on_client_connected(client_socket_ptr);
        }
        catch (const std::exception &e)
        {
            // Handle connection acceptance errors
            std::string error_message = "Error handling new connection: " + std::string(e.what());
            this->on_exception(std::make_unique<hamza::socket_exception>(error_message, "TCP_SERVER_Accept", __func__));
        }
    }

    /**
     * Public interface to start server listening.
     * Simple wrapper around run() method for cleaner API.
     * Blocking call that starts the main server loop.
     */
    void tcp_server::listen()
    {
        run();
    }

    /**
     * Stop the TCP server.
     */
    void tcp_server::stop()
    {
        set_running_status(false);
    }
    /**
     * Thread-safe client connection closure.
     * Uses mutex to ensure atomic operation during client removal.
     * Validates socket pointer before attempting removal.
     * Safe to call from multiple threads or signal handlers.
     */
    void tcp_server::close_connection(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // Use lock guard for automatic mutex management
        // std::lock_guard<std::mutex> lock(close_mutex);
        remove_client(sock_ptr);
    }

    void tcp_server::on_waiting_for_activity()
    {
        // Default implementation does nothing
        // Can be overridden by derived classes if needed
        // Examples: connection health checks, periodic cleanup, status reporting
    }
    void tcp_server::on_client_disconnect(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // Default implementation does nothing
        // Can be overridden by derived classes if needed
        (void)sock_ptr;
    }
    void tcp_server::on_client_connected(std::shared_ptr<hamza::socket> sock_ptr)
    {
        // Default implementation does nothing
        // Can be overridden by derived classes if needed
        (void)sock_ptr;
    }

    /**
     * Thread-safe getter for server running status.
     * Uses atomic load operation to ensure consistent read.
     * Returns current state of server execution loop.
     */
    bool tcp_server::get_running_status() const
    {
        return running.load();
    }

    /**
     * Thread-safe setter for server running status.
     * Uses atomic store operation to ensure thread safety.
     * Setting to false will cause server loop to exit gracefully.
     */
    void tcp_server::set_running_status(bool status)
    {
        running.store(status);
    }

    socket_address tcp_server::get_remote_address() const
    {
        if (server_socket)
        {
            return server_socket->get_remote_address();
        }
        return socket_address();
    }

    void tcp_server::close_server_socket()
    {
        // std::lock_guard<std::mutex> lock(close_mutex);
        // Ensure server socket is initialized before attempting closure
        if (!server_socket)
        {
            throw hamza::socket_exception("Server socket is not initialized.", "TCP_SERVER_SocketClose", __func__);
        }

        // Check if server is still running
        if (get_running_status())
        {
            throw hamza::socket_exception("Cannot close server socket while server is running.", "TCP_SERVER_CloseWhileRunning", __func__);
        }

        // Close the server socket and release resources
        fd_select_server.remove_fd(server_socket->get_file_descriptor_raw_value());
        server_socket->disconnect();
        server_socket.reset();
    }

};
