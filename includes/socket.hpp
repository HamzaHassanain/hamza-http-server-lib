#pragma once

#include <string>
#include <socket_address.hpp>
#include <file_descriptor.hpp>
#include <data_buffer.hpp>
#include <netinet/in.h>
#include <utilities.hpp>

namespace hamza
{
    class socket
    {
    private:
        socket_address addr;
        file_descriptor fd;
        Protocol protocol;
        void bind(const socket_address &addr);
        void set_reuse_address(bool reuse);

    public:
        // unique_id id;

        socket() = delete;
        explicit socket(const socket_address &addr, const Protocol &protocol);
        explicit socket(const socket_address &addr, const Protocol &protocol, bool reuse);
        explicit socket(const file_descriptor &fd, const Protocol &protocol);

        // Disable copy semantics
        socket(const socket &) = delete;
        socket &operator=(const socket &) = delete;

        // enable move semantics
        socket(socket &&other)
        {
            addr = std::move(other.addr);
            fd = std::move(other.fd);
            protocol = other.protocol;

            other.fd = file_descriptor(INVALID_SOCKET_VALUE); // Invalidate the moved-from socket
            other.addr = socket_address();                    // Reset the moved-from address
        }
        socket &operator=(socket &&other)
        {
            if (this != &other)
            {
                addr = std::move(other.addr);
                fd = std::move(other.fd);
                protocol = other.protocol;

                other.fd = file_descriptor(INVALID_SOCKET_VALUE); // Invalidate the moved-from socket
                other.addr = socket_address();                    // Reset the moved-from address
            }
            return *this;
        }
        // void set_non_blocking(bool non_blocking);
        void connect(const socket_address &addr);
        void listen(int backlog = SOMAXCONN);

        std::shared_ptr<socket> accept();

        // for UDP connections
        data_buffer receive(socket_address &client_addr);
        void send_to(const socket_address &addr, const data_buffer &data);

        // for TCP connections
        socket_address get_remote_address() const;
        file_descriptor get_file_descriptor() const;
        void send_on_connection(const data_buffer &data); // write on this current socket
        data_buffer receive_on_connection();              // read from this current socket

        void disconnect();
        bool is_connected() const;

        bool operator<(const socket &other) const noexcept
        {
            return fd < other.fd;
        }

        ~socket();
    };
};