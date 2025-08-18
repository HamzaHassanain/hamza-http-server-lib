#pragma once

#include <socket_address.hpp>
#include <socket.hpp>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>

namespace hamza
{
    /**
     * @brief Thread-safe container for managing client socket connections.
     *
     * This container provides thread-safe operations for managing a collection of
     * client sockets in a multi-threaded server environment. All operations are
     * protected by mutex to ensure thread safety when multiple threads access
     * the container simultaneously.
     *
     * @note Uses shared_ptr for automatic memory management of socket objects.
     * @note All methods are thread-safe and can be called from multiple threads.
     *
     * Example usage:
     * @code
     * clients_container clients;
     * auto client_socket = server.accept();
     * clients.insert(client_socket);
     *
     * // Process all connected clients
     * clients.for_each([](std::shared_ptr<socket> sock) {
     *     if (sock->is_connected()) {
     *         // Handle client
     *     }
     * });
     *
     * clients.cleanup(); // Remove disconnected clients
     * @endcode
     */
    struct clients_container
    {
    private:
        /// Container holding shared pointers to client sockets
        std::vector<std::shared_ptr<hamza::socket>> sockets;

        /// Type alias for callback functions used in for_each operations
        using callback = std::function<void(std::shared_ptr<hamza::socket>)>;

        /// Mutex for thread-safe access to the container (mutable for const methods)
        mutable std::mutex mtx;

    public:
        /**
         * @brief Insert a new client socket into the container.
         * @param sock Shared pointer to the client socket to add
         *
         * Thread-safe operation that adds a new client socket to the collection.
         * The socket is added to the end of the internal vector.
         */
        void insert(std::shared_ptr<hamza::socket> sock)
        {
            std::lock_guard<std::mutex> lock(mtx);
            sockets.push_back(sock);
        }

        /**
         * @brief Remove a client socket from the container.
         * @param sock Shared pointer to the client socket to remove
         *
         * Thread-safe operation that removes the first socket matching the
         * file descriptor of the provided socket. Uses file descriptor comparison
         * instead of pointer comparison for reliable identification.
         *
         * @note If the socket is not found, the operation has no effect.
         */
        void erase(std::shared_ptr<hamza::socket> sock)
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = sockets.begin();
            for (; it != sockets.end(); ++it)
            {
                if (!(*it))
                    continue;
                if ((*it)->get_file_descriptor_raw_value() == sock->get_file_descriptor_raw_value())
                {
                    sockets.erase(it);
                    break;
                }
            }
        }

        /**
         * @brief Check if a socket is present in the container.
         * @param sock Shared pointer to the socket to search for
         * @return true if the socket is found, false otherwise
         *
         * Thread-safe operation that searches for the socket using pointer comparison.
         */
        bool contains(std::shared_ptr<hamza::socket> sock) const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return std::find(sockets.begin(), sockets.end(), sock) != sockets.end();
        }

        /**
         * @brief Get the number of sockets in the container.
         * @return Number of client sockets currently stored
         *
         * Thread-safe operation that returns the current size of the container.
         */
        size_t size() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return sockets.size();
        }

        /**
         * @brief Remove all sockets from the container.
         *
         * Thread-safe operation that clears the entire container, removing
         * all client socket references.
         */
        void clear()
        {
            std::lock_guard<std::mutex> lock(mtx);
            sockets.clear();
        }

        /**
         * @brief Remove disconnected and null sockets from the container.
         *
         * Thread-safe operation that performs garbage collection by removing:
         * - Null shared_ptr objects
         * - Sockets that are no longer connected
         *
         * This method should be called periodically to prevent accumulation
         * of stale socket references.
         */
        void cleanup()
        {
            std::lock_guard<std::mutex> lock(mtx);
            sockets.erase(std::remove_if(sockets.begin(), sockets.end(),
                                         [](const std::shared_ptr<hamza::socket> &sock)
                                         {
                                             return !sock || !sock->is_connected();
                                         }),
                          sockets.end());
        }

        /**
         * @brief Apply a function to each socket in the container.
         * @param func Function to apply to each socket
         *
         * Thread-safe operation that creates a copy of the socket collection
         * and applies the provided function to each valid socket. The copy
         * is made under lock to ensure consistency, then the function is
         * applied without holding the lock to prevent deadlocks.
         *
         * @note The function is only called on non-null sockets.
         * @note Changes to the container during iteration do not affect the iteration.
         *
         * Example:
         * @code
         * clients.for_each([](std::shared_ptr<socket> sock) {
         *     auto data = sock->receive_on_connection();
         *     if (data.size() > 0) {
         *         process_data(data);
         *     }
         * });
         * @endcode
         */
        void for_each(std::function<void(std::shared_ptr<hamza::socket>)> func) const
        {
            std::vector<std::shared_ptr<hamza::socket>> copy;
            {
                std::lock_guard<std::mutex> lock(mtx);
                copy = sockets; // Copy under lock
            }
            for (const auto &sock : copy)
            {
                if (sock)
                    func(sock);
            }
        }

        /**
         * @brief Get the maximum file descriptor value among all sockets.
         * @return Maximum file descriptor value, or -1 if container is empty
         *
         * Thread-safe operation used primarily for select() system calls,
         * which require the maximum file descriptor value to determine the
         * range of file descriptors to monitor.
         *
         * @note Skips null socket pointers when calculating maximum.
         */
        int max() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (sockets.empty())
            {
                return -1;
            }
            int max_fd = -1;
            for (const auto &sock : sockets)
            {
                if (!sock)
                    continue;
                max_fd = std::max(max_fd, sock->get_file_descriptor_raw_value());
            }
            return max_fd;
        }

        /**
         * @brief Check if the container is empty.
         * @return true if no sockets are stored, false otherwise
         *
         * Thread-safe operation that checks if the container has any sockets.
         */
        bool empty() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return sockets.empty();
        }
    };
}