#include <utilities.hpp>
#include <string>
#include <ip_address.hpp>
#include <memory>
#include <family.hpp>
#include <port.hpp>
// Platform-specific includes
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // Link Windows socket library
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

namespace hamza
{
    void convert_ip_address_to_network_order(const family &family_ip, const ip_address &address, void *addr)
    {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        // Windows implementation
        if (family_ip.get_family_id() == AF_INET)
        {
            IN_ADDR in_addr;
            if (InetPtonA(AF_INET, address.get_ip_address().c_str(), &in_addr) == 1)
            {
                *(reinterpret_cast<IN_ADDR *>(addr)) = in_addr;
            }
        }
        else if (family_ip.get_family_id() == AF_INET6)
        {
            IN6_ADDR in6_addr;
            if (InetPtonA(AF_INET6, address.get_ip_address().c_str(), &in6_addr) == 1)
            {
                *(reinterpret_cast<IN6_ADDR *>(addr)) = in6_addr;
            }
        }
#else
        // Unix/Linux implementation
        inet_pton(family_ip.get(), address.get().c_str(), addr);
#endif
    }
    std::string get_ip_address_from_network_address(struct sockaddr_storage &addr)
    {
        char ip_str[INET6_ADDRSTRLEN];

        if (addr.ss_family == AF_INET)
        {
            sockaddr_in *addr_in = reinterpret_cast<sockaddr_in *>(&addr);
            inet_ntop(AF_INET, &addr_in->sin_addr, ip_str, sizeof(ip_str));
        }
        else if (addr.ss_family == AF_INET6)
        {
            sockaddr_in6 *addr_in6 = reinterpret_cast<sockaddr_in6 *>(&addr);
            inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip_str, sizeof(ip_str));
        }

        return std::string(ip_str);
    }
    int convert_host_to_network_order(int port)
    {
        // htons() is available on both Windows and Unix/Linux
        // after including the appropriate headers
        return htons(static_cast<uint16_t>(port));
    }
    int convert_network_order_to_host(int port)
    {
        // ntohs() is available on both Windows and Unix/Linux
        // after including the appropriate headers
        return ntohs(static_cast<uint16_t>(port));
    }

    bool initialize_socket_library()
    {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        return result == 0;
#else
        // Unix/Linux doesn't need socket library initialization
        return true;
#endif
    }

    void cleanup_socket_library()
    {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        WSACleanup();
#else
        // Unix/Linux doesn't need socket library cleanup
#endif
    }

    void close_socket(socket_t socket)
    {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        closesocket(socket);
#else
        close(socket);
#endif
    }

    bool is_valid_socket(socket_t socket)
    {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        return socket != INVALID_SOCKET;
#else
        return socket >= 0;
#endif
    }

    bool is_socket_open(int fd)
    {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        // On Windows, use getsockopt to check if it's a valid socket
        int type;
        int type_len = sizeof(type);
        return getsockopt(fd, SOL_SOCKET, SO_TYPE, reinterpret_cast<char *>(&type), &type_len) == 0;
#else
        // On Unix/Linux, use getsockopt to check if fd is a socket
        int type;
        socklen_t type_len = sizeof(type);
        return getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &type_len) == 0;
#endif
    }

    bool is_socket_connected(socket_t socket)
    {
        if (!is_valid_socket(socket))
        {
            return false;
        }

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
        // On Windows, check socket state
        int error = 0;
        int error_len = sizeof(error);
        int result = getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&error), &error_len);

        if (result != 0 || error != 0)
        {
            return false;
        }

        // Try to get peer address
        sockaddr_storage addr;
        int addr_len = sizeof(addr);
        return getpeername(socket, reinterpret_cast<sockaddr *>(&addr), &addr_len) == 0;
#else
        // On Unix/Linux, check socket state
        int error = 0;
        socklen_t error_len = sizeof(error);
        int result = getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &error_len);

        if (result != 0 || error != 0)
        {
            return false;
        }

        // Try to get peer address to confirm connection
        sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);
        return getpeername(socket, reinterpret_cast<sockaddr *>(&addr), &addr_len) == 0;
#endif
    }
}