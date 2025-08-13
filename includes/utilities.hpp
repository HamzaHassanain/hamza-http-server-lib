#pragma once

#include <string>
#include <family.hpp>
#include <ip_address.hpp>
#include <sys/socket.h>
// Platform detection and common socket types
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define HAMZA_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_t = SOCKET;
using socklen_t = int;
const socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
const int SOCKET_ERROR_VALUE = SOCKET_ERROR;
#else
#define HAMZA_PLATFORM_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using socket_t = int;
const socket_t INVALID_SOCKET_VALUE = -1;
const int SOCKET_ERROR_VALUE = -1;
#endif

namespace hamza
{
    const int IPV4 = AF_INET;
    const int IPV6 = AF_INET6;

    enum class Protocol
    {
        TCP = SOCK_STREAM,
        UDP = SOCK_DGRAM
    };
    const char NEW_LINE = '\n';
    // Cross-platform socket utilities
    void convert_ip_address_to_network_order(const family &family_ip, const ip_address &address, void *addr);
    std::string get_ip_address_from_network_address(sockaddr_storage &addr);

    int convert_host_to_network_order(int port);
    int convert_network_order_to_host(int port);
    // Cross-platform socket management
    bool initialize_socket_library();   // Initialize Winsock on Windows
    void cleanup_socket_library();      // Cleanup Winsock on Windows
    void close_socket(socket_t socket); // Cross-platform socket close

    // Socket validation functions
    bool is_valid_socket(socket_t socket);     // Check if socket is valid
    bool is_socket_open(int fd);               // Check if fd is an open socket
    bool is_socket_connected(socket_t socket); // Check if socket is connected
}