#pragma once

#include <string>
#include <ostream>
#include <sys/socket.h> // for sockaddr, AF_INET, AF_INET6
#include <vector>
#include <algorithm>
#include <stdexcept>
namespace hamza
{
    class family
    {
        const std::vector<int> allowed_families = {AF_INET, AF_INET6};
        int family_id;

        void set_family_id(int id)
        {
            if (std::find(allowed_families.begin(), allowed_families.end(), id) != allowed_families.end())
            {
                family_id = id;
            }

            else
            {
                throw std::invalid_argument("Invalid family ID. Allowed families are AF_INET and AF_INET6.");
            }
        }

    public:
        explicit family() : family_id(AF_INET) {} // Initialize with default value
        explicit family(int id) { set_family_id(id); }

        family(const family &other)
        {
            family_id = other.family_id;
        }
        family &operator=(const family &other)
        {
            if (this != &other)
            {
                family_id = other.family_id;
            }
            return *this;
        }

        family(family &&other) noexcept
        {
            family_id = other.family_id;
            other.family_id = -1; // Reset other to a default state
        }
        family &operator=(family &&other) noexcept
        {
            if (this != &other)
            {
                family_id = other.family_id;
                other.family_id = -1; // Reset other to a default state
            }
            return *this;
        }

        int get() const { return family_id; }

        bool operator==(const family &other) const
        {
            return family_id == other.family_id;
        }

        bool operator!=(const family &other) const
        {
            return !(*this == other);
        }

        bool operator<(const family &other) const
        {
            return family_id < other.family_id;
        }

        friend std::ostream &operator<<(std::ostream &os, const family &f)
        {
            os << f.family_id;
            return os;
        }

        ~family() = default;
    };
};