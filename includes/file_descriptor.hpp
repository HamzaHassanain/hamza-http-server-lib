#pragma once

#include <stdexcept>
#include <utilities.hpp>
#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET socket_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
typedef int socket_t;
#endif
namespace hamza
{
    class file_descriptor
    {
    private:
        socket_t fd;

    public:
        explicit file_descriptor() = default;
        explicit file_descriptor(socket_t fd)
        {
            // check if actual file descriptor

            if (fd == INVALID_SOCKET_VALUE)
            {
                this->fd = INVALID_SOCKET_VALUE;
                return;
            }

            this->fd = fd;
        }
        int get() const { return fd; }

        ~file_descriptor() = default;
        file_descriptor(const file_descriptor &) = default;
        file_descriptor &operator=(const file_descriptor &) = default;
        file_descriptor(file_descriptor &&) = default;
        file_descriptor &operator=(file_descriptor &&) = default;

        bool operator==(const file_descriptor &other) const
        {
            return fd == other.fd;
        }

        bool operator!=(const file_descriptor &other) const
        {
            return !(*this == other);
        }

        bool operator<(const file_descriptor &other) const
        {
            return fd < other.fd;
        }

        friend std::ostream &operator<<(std::ostream &os, const file_descriptor &fd)
        {
            os << fd.get();
            return os;
        }
    };
};