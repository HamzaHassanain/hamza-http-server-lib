#pragma once

#include <string>
#include <netinet/in.h>
#include <memory>

#include <ip_address.hpp>
#include <family.hpp>
#include <port.hpp>

namespace hamza
{
    class socket_address
    {

    private:
        ip_address address;             // IP address
        family family_id;               // Address family
        port port_id;                   // Port
        std::shared_ptr<sockaddr> addr; // Socket address pointer

    public:
        explicit socket_address() = default; // Default constructor
        explicit socket_address(const ip_address &address, const port &port_id, const family &family_id);
        explicit socket_address(sockaddr_storage &addr);
        socket_address(const socket_address &other);            // Custom copy constructor
        socket_address &operator=(const socket_address &other); // Custom copy assignment operator
        socket_address(socket_address &&) = default;            // Move constructor
        socket_address &operator=(socket_address &&) = default; // Move assignment operator
        ~socket_address() = default;                            // Destructor

        ip_address get_ip_address() const { return address; }
        port get_port() const { return port_id; }
        family get_family() const { return family_id; }

        sockaddr *get_sock_addr() const;
        socklen_t get_sock_addr_len() const;

        friend void handle_ipv4(socket_address *addr, const ip_address &address, const port &port_id, const family &family_id);
        friend void handle_ipv6(socket_address *addr, const ip_address &address, const port &port_id, const family &family_id);
        friend std::ostream &operator<<(std::ostream &os, const socket_address &sa)
        {
            os << "IP Address: " << sa.get_ip_address() << ", Port: " << sa.get_port() << ", Family: " << sa.get_family();
            return os;
        }
    };
};