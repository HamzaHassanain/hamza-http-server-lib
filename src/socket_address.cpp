

#include <socket_address.hpp>
#include <utilities.hpp>

#include <ip_address.hpp>
#include <family.hpp>
#include <port.hpp>

namespace hamza
{
    socket_address::socket_address(const ip_address &address, const port &port_id, const family &family_id)
        : address(address), family_id(family_id), port_id(port_id)
    {
        handle_ipv4(this, address, port_id, family_id);
        handle_ipv6(this, address, port_id, family_id);
    }

    socket_address::socket_address(const socket_address &other)
        : address(other.address), family_id(other.family_id), port_id(other.port_id)
    {
        if (other.addr)
        {
            handle_ipv4(this, other.address, other.port_id, other.family_id);
            handle_ipv6(this, other.address, other.port_id, other.family_id);
        }
    }

    socket_address::socket_address(sockaddr_storage &addr)
    {
        if (addr.ss_family == AF_INET)
        {
            auto ipv4_addr = reinterpret_cast<sockaddr_in *>(&addr);

            address = ip_address(get_ip_address_from_network_address(addr));
            port_id = port(convert_network_order_to_host(ipv4_addr->sin_port));
            family_id = family(AF_INET);

            this->addr = std::reinterpret_pointer_cast<sockaddr>(std::make_shared<sockaddr_in>(*ipv4_addr));
        }
        else if (addr.ss_family == AF_INET6)
        {
            auto ipv6_addr = reinterpret_cast<sockaddr_in6 *>(&addr);
            address = ip_address(get_ip_address_from_network_address(addr));
            port_id = port(convert_network_order_to_host(ipv6_addr->sin6_port));
            family_id = family(AF_INET6);
        }
    }

    socket_address &socket_address::operator=(const socket_address &other)
    {
        if (this != &other)
        {
            address = other.address;
            family_id = other.family_id;
            port_id = other.port_id;

            if (other.addr)
            {
                handle_ipv4(this, other.address, other.port_id, other.family_id);
                handle_ipv6(this, other.address, other.port_id, other.family_id);
            }
        }
        return *this;
    }

    socklen_t socket_address::get_sock_addr_len() const
    {
        if (family_id.get() == AF_INET)
        {
            return sizeof(sockaddr_in);
        }
        else if (family_id.get() == AF_INET6)
        {
            return sizeof(sockaddr_in6);
        }
        return 0;
    }

    sockaddr *socket_address::get_sock_addr() const
    {
        return addr.get();
    }

    void handle_ipv4(socket_address *addr, const ip_address &address, const port &port_id, const family &family_id)
    {
        auto cur_addr = std::make_shared<sockaddr_in>();
        cur_addr->sin_family = family_id.get();
        cur_addr->sin_port = convert_host_to_network_order(port_id.get());
        convert_ip_address_to_network_order(family_id, address, &cur_addr->sin_addr);

        addr->addr = std::reinterpret_pointer_cast<sockaddr>(cur_addr);
    }

    void handle_ipv6(socket_address *addr, const ip_address &address, const port &port_id, const family &family_id)
    {
        auto cur_addr = std::make_shared<sockaddr_in6>();
        cur_addr->sin6_family = family_id.get();
        cur_addr->sin6_port = convert_host_to_network_order(port_id.get());
        convert_ip_address_to_network_order(family_id, address, &cur_addr->sin6_addr);

        addr->addr = std::reinterpret_pointer_cast<sockaddr>(cur_addr);
    }
}
