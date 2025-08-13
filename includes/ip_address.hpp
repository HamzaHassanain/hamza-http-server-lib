#pragma once

#include <string>
#include <ostream>

namespace hamza
{

    class ip_address
    {
        std::string address;

    public:
        explicit ip_address() = default;
        explicit ip_address(const std::string &address) : address(address) {}

        // default copy and move constructors and assignment operators
        ip_address(const ip_address &) = default;
        ip_address(ip_address &&) noexcept = default;
        ip_address &operator=(const ip_address &) = default;
        ip_address &operator=(ip_address &&) noexcept = default;

        const std::string &get() const { return address; }

        bool operator==(const ip_address &other) const
        {
            return address == other.address;
        }

        bool operator!=(const ip_address &other) const
        {
            return !(*this == other);
        }

        bool operator<(const ip_address &other) const
        {
            return address < other.address;
        }

        friend std::ostream &operator<<(std::ostream &os, const ip_address &ip)
        {
            os << ip.address;
            return os;
        }

        ~ip_address() = default;
    };

};