#pragma once

#include <stdexcept>
#include <ostream>

namespace hamza
{

    class port
    {
        int port_id;

        void set_port_id(int id)
        {
            if (id < 0 || id > 65535)
            {
                throw std::invalid_argument("Port ID must be between 0 and 65535.");
            }
            port_id = id;
        }

    public:
        explicit port() = default;
        explicit port(int id) { set_port_id(id); }

        // default copy and move constructors and assignment operators
        port(const port &) = default;
        port(port &&) noexcept = default;
        port &operator=(const port &) = default;
        port &operator=(port &&) noexcept = default;

        int get() const { return port_id; }

        bool operator==(const port &other) const
        {
            return port_id == other.port_id;
        }

        bool operator!=(const port &other) const
        {
            return !(*this == other);
        }

        bool operator<(const port &other) const
        {
            return port_id < other.port_id;
        }

        friend std::ostream &operator<<(std::ostream &os, const port &p)
        {
            os << p.port_id;
            return os;
        }

        ~port() = default;
    };

};