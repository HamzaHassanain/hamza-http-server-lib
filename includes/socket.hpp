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

    public:
        explicit socket() = default;
        explicit socket(const socket_address &addr, const Protocol &protocol);
        explicit socket(const file_descriptor &fd, const Protocol &protocol);

        // Disable copy semantics
        socket(const socket &) = delete;
        socket &operator=(const socket &) = delete;

        // enable move semantics
        socket(socket &&) = default;
        socket &operator=(socket &&) = default;
        void set_non_blocking(bool non_blocking);
        void connect(const socket_address &addr);
        void bind(const socket_address &addr);
        void listen(int backlog = SOMAXCONN);
        socket accept();

        // for UDP connections
        data_buffer receive(socket_address &client_addr);
        void send_to(const socket_address &addr, const data_buffer &data);

        // for TCP connections
        socket_address get_remote_address() const;
        file_descriptor get_file_descriptor() const;
        void send(const data_buffer &data);  // write on this current socket
        data_buffer receive_on_connection(); // read from this current socket

        void disconnect();
        bool is_connected() const;

        ~socket();
    };
};