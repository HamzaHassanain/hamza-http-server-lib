# Hamza HTTP Server Library

A high-performance, cross-platform HTTP server library written in C++17. This library provides a complete networking stack with TCP server capabilities, socket management, and HTTP protocol handling.

## Table of Contents

- [Overview](#overview)
- [Fundamental Networking Concepts](#fundamental-networking-concepts)
- [Features](#features)
- [Building the Project](#building-the-project)
- [Quick Start](#quick-start)
- [Architecture Overview](#architecture-overview)
- [API Documentation](#api-documentation)
- [Examples](#examples)
- [Platform Support](#platform-support)
- [About CMakeLists](#about-cmakelists)

## Overview

This library provides a modern C++17 implementation of an HTTP server built on fundamental networking principles. It abstracts away the complexity of low-level socket programming while providing full control over HTTP request/response handling. The library implements a complete networking stack from raw sockets up to HTTP protocol handling, making it suitable for both educational purposes and production applications.

## Fundamental Networking Concepts

Understanding the underlying networking concepts is crucial for effectively using this library. Here's an explanation of the key concepts:

### What are Sockets?

**Sockets** are programming abstractions that represent endpoints of network communication. Think of them as "plugs" that allow programs to send and receive data over a network.

```cpp
// Creating a socket in our library
hamza::socket server_socket(addr, hamza::Protocol::TCP, true);
```

**Key Socket Concepts:**

- **Network Socket**: A software endpoint that establishes bidirectional communication between applications
- **Socket Address**: Combination of IP address and port number that uniquely identifies a network endpoint
- **Socket Types**:
  - **TCP Sockets** (SOCK_STREAM): Reliable, connection-oriented communication
  - **UDP Sockets** (SOCK_DGRAM): Fast, connectionless communication

**Socket Lifecycle:**

1. **Creation**: Allocate socket resources
2. **Binding**: Associate socket with a specific address/port
3. **Listening**: (Server) Wait for incoming connections
4. **Accepting**: (Server) Accept client connections
5. **Connecting**: (Client) Establish connection to server
6. **Data Transfer**: Send/receive data
7. **Closing**: Release socket resources

### What are File Descriptors?

**File Descriptors (FDs)** are integer handles used by the operating system to track open files, sockets, pipes, and other I/O resources.

```cpp
// Our file_descriptor wrapper
hamza::file_descriptor fd = socket.get_file_descriptor();
int raw_fd = fd.get(); // Get the underlying integer
```

**Key FD Concepts:**

- **Integer Handle**: Each FD is simply an integer (0, 1, 2, 3, ...)
- **Per-Process**: Each process has its own FD table
- **Standard FDs**:
  - `0` = stdin (standard input)
  - `1` = stdout (standard output)
  - `2` = stderr (standard error)
- **Resource Management**: FDs must be properly closed to prevent resource leaks

**Why FDs Matter for Networking:**

- Sockets are treated as files in Unix-like systems
- The same I/O operations (read/write) work on both files and sockets
- I/O multiplexing (select/poll/epoll) operates on FDs

### How the Kernel Handles Networking

The **kernel** is the core of the operating system that manages hardware resources, including network interfaces.

**Networking in the Kernel:**

1. **Network Stack Layers**:

   ```
   Application Layer  ← Your HTTP Server
   Transport Layer    ← TCP/UDP (our focus)
   Network Layer      ← IP (Internet Protocol)
   Data Link Layer    ← Ethernet/WiFi
   Physical Layer     ← Network hardware
   ```

2. **Kernel's Role**:

   - **Packet Processing**: Receives network packets from hardware
   - **Protocol Implementation**: Handles TCP/IP protocol stack
   - **Socket Management**: Maintains socket state and buffers
   - **I/O Multiplexing**: Efficiently manages multiple connections

3. **How Our Library Interacts with Kernel**:

   ```cpp
   // When you call this:
   socket.send_on_connection(data);

   // The kernel:
   // 1. Copies data to kernel buffer
   // 2. Breaks data into TCP segments
   // 3. Adds TCP/IP headers
   // 4. Sends packets via network interface
   ```

**I/O Multiplexing with `select()`:**
Our library uses `select()` to efficiently handle multiple client connections:

```cpp
// Our select_server manages multiple file descriptors
hamza::select_server fd_select_server;
fd_select_server.add_fd(client_fd);
int activity = fd_select_server.select(); // Kernel checks all FDs for activity
```

### What is TCP (Transmission Control Protocol)?

**TCP** is a reliable, connection-oriented transport protocol that ensures data delivery and ordering.

**TCP Characteristics:**

- **Reliable**: Guarantees data delivery through acknowledgments and retransmission
- **Ordered**: Data arrives in the same order it was sent
- **Connection-oriented**: Establishes a connection before data transfer
- **Flow Control**: Manages data transmission speed
- **Error Detection**: Detects and corrects transmission errors

**TCP Connection Process (3-Way Handshake):**

```
Client                    Server
  |                         |
  |-------SYN------------->|  1. Client requests connection
  |<---SYN-ACK-------------|  2. Server acknowledges and responds
  |-------ACK------------->|  3. Client acknowledges, connection established
  |                         |
  |<====Data Transfer=====>|
```

**How Our Library Uses TCP:**

```cpp
// Create TCP socket
hamza::socket server_socket(addr, hamza::Protocol::TCP, true);

// Listen for connections (server becomes passive)
server_socket.listen();

// Accept incoming connections
auto client_socket = server_socket.accept();

// Reliable data transfer
client_socket.send_on_connection(response_data);
```

### What is HTTP (HyperText Transfer Protocol)?

**HTTP** is an application-layer protocol built on top of TCP for transferring web content.

**HTTP Characteristics:**

- **Request-Response**: Client sends request, server sends response
- **Stateless**: Each request is independent
- **Text-based**: Human-readable headers and methods
- **Flexible**: Supports various content types and methods

**HTTP Message Structure:**

```
HTTP Request:
GET /path HTTP/1.1          ← Request Line
Host: example.com           ← Headers
Content-Type: text/html
                           ← Empty line
[optional body]            ← Body

HTTP Response:
HTTP/1.1 200 OK            ← Status Line
Content-Type: text/html    ← Headers
Content-Length: 13
                           ← Empty line
Hello, World!              ← Body
```

**How Our Library Handles HTTP:**

1. **Raw TCP Data Reception**:

   ```cpp
   // TCP layer receives raw bytes
   hamza::data_buffer raw_data = socket.receive_on_connection();
   ```

2. **HTTP Parsing**:

   ```cpp
   // Our http_server parses the raw data into HTTP objects
   http_request request(method, uri, version, headers, body, socket);
   ```

3. **HTTP Response Building**:

   ```cpp
   // Your application logic
   response.set_status(200, "OK");
   response.add_header("Content-Type", "application/json");
   response.set_body("{\"message\": \"Hello\"}");
   ```

4. **HTTP Response Transmission**:
   ```cpp
   // Library converts response to raw TCP data and sends
   response.end(); // Formats and sends via TCP socket
   ```

**Complete Flow in Our Library:**

```
1. Kernel receives TCP packet
2. Kernel notifies select() that FD has data
3. Our library reads raw bytes from socket
4. HTTP parser converts bytes to http_request object
5. Your callback function handles the request
6. You build http_response object
7. Library converts response to raw bytes
8. TCP socket sends bytes back to client
9. Kernel transmits packets over network
```

This foundation allows our library to provide a clean, high-level HTTP API while maintaining full control over the underlying networking mechanisms.

## Features

- **Cross-platform**: Supports Windows, Linux, and Unix-like systems
- **Modern C++17**: Leverages modern C++ features and best practices
- **Thread-safe**: Built with concurrency in mind using mutexes and RAII
- **Memory-safe**: Uses smart pointers and RAII for automatic resource management
- **HTTP/1.1 support**: Full HTTP request/response handling
- **Select-based I/O**: Efficient event-driven networking using select()
- **Exception handling**: Comprehensive error handling with custom exception types

## Building the Project

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard networking libraries (automatically linked)

### Build Instructions

```bash
# Clone the repository
git clone <repository-url>
cd hamza-http-server-lib

# Build the project
./run.sh

# Or manually:
mkdir -p build
cmake -S . -B build
cd build
make -j$(nproc)
```

## Quick Start

Here's a simple HTTP server example:

```cpp
#include <http_server.hpp>
#include <iostream>

int main() {
    // Create server on localhost:8080
    hamza_http::http_server server("127.0.0.1", 8080);

    // Set request handler
    server.set_request_callback([](hamza_http::http_request& req, hamza_http::http_response& res) {
        std::cout << "Request: " << req.get_method() << " " << req.get_uri() << std::endl;

        res.set_status(200, "OK");
        res.add_header("Content-Type", "text/html");
        res.set_body("<h1>Hello, World!</h1>");
        res.end();
    });

    // Start the server
    server.run();
    return 0;
}
```

## Architecture Overview

The library is organized into several layers that mirror the networking stack:

1. **Core Networking Layer** ([`hamza`](includes/) namespace)

   - Socket management and networking primitives
   - Cross-platform socket abstraction
   - TCP server implementation with select-based I/O

2. **HTTP Layer** ([`hamza_http`](includes/) namespace)

   - HTTP protocol handling
   - Request/response objects
   - HTTP server implementation

3. **Utility Layer**
   - Cross-platform utilities
   - Exception handling
   - Data structures and containers

## API Documentation

### Namespaces

#### [`hamza`](includes/)

Core networking namespace containing all low-level networking components.

#### [`hamza_http`](includes/)

HTTP-specific namespace containing HTTP server implementation and HTTP objects.

### Core Classes

#### [`hamza::socket`](includes/socket.hpp)

**File**: [includes/socket.hpp](includes/socket.hpp), [src/socket.cpp](src/socket.cpp)

Low-level socket wrapper providing cross-platform socket operations.

**Key Methods**:

- [`socket(const socket_address& addr, const Protocol& protocol)`](includes/socket.hpp) - Create socket
- [`listen(int backlog = SOMAXCONN)`](includes/socket.hpp) - Listen for connections (TCP)
- [`accept()`](includes/socket.hpp) - Accept incoming connection (TCP)
- [`connect(const socket_address& addr)`](includes/socket.hpp) - Connect to remote address
- [`send_on_connection(const data_buffer& data)`](includes/socket.hpp) - Send data (TCP)
- [`receive_on_connection()`](includes/socket.hpp) - Receive data (TCP)
- [`send_to(const socket_address& addr, const data_buffer& data)`](includes/socket.hpp) - Send data (UDP)
- [`receive(socket_address& client_addr)`](includes/socket.hpp) - Receive data (UDP)
- [`disconnect()`](includes/socket.hpp) - Close socket connection
- [`is_connected()`](includes/socket.hpp) - Check connection status

**Example**:

```cpp
hamza::socket_address addr(hamza::ip_address("127.0.0.1"), hamza::port(8080), hamza::family(AF_INET));
hamza::socket server_socket(addr, hamza::Protocol::TCP, true);
server_socket.listen();
```

#### [`hamza::socket_address`](includes/socket_address.hpp)

**File**: [includes/socket_address.hpp](includes/socket_address.hpp), [src/socket_address.cpp](src/socket_address.cpp)

Represents a network address (IP + Port + Family).

**Key Methods**:

- [`socket_address(const ip_address& addr, const port& port, const family& family)`](includes/socket_address.hpp) - Constructor
- [`get_ip_address()`](includes/socket_address.hpp) - Get IP address
- [`get_port()`](includes/socket_address.hpp) - Get port number
- [`get_family()`](includes/socket_address.hpp) - Get address family
- [`get_sock_addr()`](includes/socket_address.hpp) - Get underlying sockaddr structure

#### [`hamza::tcp_server`](includes/tcp_server.hpp)

**File**: [includes/tcp_server.hpp](includes/tcp_server.hpp), [src/tcp_server.cpp](src/tcp_server.cpp)

Base TCP server class with select-based I/O multiplexing.

**Key Methods**:

- [`tcp_server(const socket_address& addr)`](includes/tcp_server.hpp) - Constructor
- [`run()`](includes/tcp_server.hpp) - Start server main loop
- [`close_connection(std::shared_ptr<socket> sock_ptr)`](includes/tcp_server.hpp) - Close client connection

**Virtual Methods** (to be overridden):

- [`on_message_received(std::shared_ptr<socket>, const data_buffer&)`](includes/tcp_server.hpp) - Handle client messages
- [`on_new_client_connected(std::shared_ptr<socket>)`](includes/tcp_server.hpp) - Handle new connections
- [`on_client_disconnect(std::shared_ptr<socket>)`](includes/tcp_server.hpp) - Handle disconnections
- [`on_listen_success()`](includes/tcp_server.hpp) - Handle successful server start
- [`on_exception(std::shared_ptr<general_socket_exception>)`](includes/tcp_server.hpp) - Handle exceptions

#### [`hamza::select_server`](includes/select_server.hpp)

**File**: [includes/select_server.hpp](includes/select_server.hpp), [src/select_server.cpp](src/select_server.cpp)

Manages file descriptor sets for select-based I/O multiplexing.

**Key Methods**:

- [`init(const file_descriptor& server_fd)`](includes/select_server.hpp) - Initialize with server socket
- [`add_fd(const file_descriptor& fd)`](includes/select_server.hpp) - Add file descriptor to monitoring
- [`remove_fd(const file_descriptor& fd)`](includes/select_server.hpp) - Remove file descriptor
- [`select()`](includes/select_server.hpp) - Perform select operation
- [`is_fd_set(const file_descriptor& fd)`](includes/select_server.hpp) - Check if FD has activity

#### [`hamza::clients_container`](includes/clients_container.hpp)

**File**: [includes/clients_container.hpp](includes/clients_container.hpp)

Thread-safe container for managing client socket connections.

**Key Methods**:

- [`insert(std::shared_ptr<socket>)`](includes/clients_container.hpp) - Add client
- [`erase(std::shared_ptr<socket>)`](includes/clients_container.hpp) - Remove client
- [`for_each(std::function<void(std::shared_ptr<socket>)>)`](includes/clients_container.hpp) - Iterate over clients
- [`cleanup()`](includes/clients_container.hpp) - Remove disconnected clients
- [`max()`](includes/clients_container.hpp) - Get maximum file descriptor value

### HTTP Classes

#### [`hamza_http::http_server`](includes/http_server.hpp)

**File**: [includes/http_server.hpp](includes/http_server.hpp), [src/http_server.cpp](src/http_server.cpp)

High-level HTTP server implementation extending [`tcp_server`](includes/tcp_server.hpp).

**Key Methods**:

- [`http_server(const socket_address& addr)`](includes/http_server.hpp) - Constructor
- [`http_server(const std::string& ip, int port)`](includes/http_server.hpp) - Convenience constructor
- [`set_request_callback(std::function<void(http_request&, http_response&)>)`](includes/http_server.hpp) - Set request handler
- [`set_listen_success_callback(std::function<void()>)`](includes/http_server.hpp) - Set listen success handler
- [`set_error_callback(std::function<void(std::shared_ptr<general_socket_exception>)>)`](includes/http_server.hpp) - Set error handler

#### [`hamza_http::http_request`](includes/http_objects.hpp)

**File**: [includes/http_objects.hpp](includes/http_objects.hpp), [src/http_objects.cpp](src/http_objects.cpp)

Represents an HTTP request with full parsing capabilities.

**Key Methods**:

- [`get_method()`](includes/http_objects.hpp) - Get HTTP method (GET, POST, etc.)
- [`get_uri()`](includes/http_objects.hpp) - Get request URI
- [`get_version()`](includes/http_objects.hpp) - Get HTTP version
- [`get_header(const std::string& name)`](includes/http_objects.hpp) - Get header values
- [`get_headers()`](includes/http_objects.hpp) - Get all headers
- [`get_body()`](includes/http_objects.hpp) - Get request body
- [`destroy(bool sure)`](includes/http_objects.hpp) - Manually destroy request

#### [`hamza_http::http_response`](includes/http_objects.hpp)

**File**: [includes/http_objects.hpp](includes/http_objects.hpp), [src/http_objects.cpp](src/http_objects.cpp)

Represents an HTTP response with building and sending capabilities.

**Key Methods**:

- [`set_status(int code, const std::string& message)`](includes/http_objects.hpp) - Set status code and message
- [`set_body(const std::string& body)`](includes/http_objects.hpp) - Set response body
- [`add_header(const std::string& name, const std::string& value)`](includes/http_objects.hpp) - Add header
- [`add_trailer(const std::string& name, const std::string& value)`](includes/http_objects.hpp) - Add trailer
- [`end()`](includes/http_objects.hpp) - Send response and close connection
- [`get_status_code()`](includes/http_objects.hpp), [`get_body()`](includes/http_objects.hpp), etc. - Getters

### Utility Classes

#### [`hamza::data_buffer`](includes/data_buffer.hpp)

**File**: [includes/data_buffer.hpp](includes/data_buffer.hpp)

Efficient buffer for handling binary and text data.

**Key Methods**:

- [`data_buffer(const std::string& str)`](includes/data_buffer.hpp) - Create from string
- [`data_buffer(const char* data, std::size_t size)`](includes/data_buffer.hpp) - Create from raw data
- [`append(const char* data, std::size_t size)`](includes/data_buffer.hpp) - Append data
- [`data()`](includes/data_buffer.hpp) - Get raw data pointer
- [`size()`](includes/data_buffer.hpp) - Get buffer size
- [`to_string()`](includes/data_buffer.hpp) - Convert to string

#### [`hamza::ip_address`](includes/ip_address.hpp)

**File**: [includes/ip_address.hpp](includes/ip_address.hpp)

Type-safe IP address wrapper.

#### [`hamza::port`](includes/port.hpp)

**File**: [includes/port.hpp](includes/port.hpp)

Type-safe port number wrapper with validation (0-65535).

#### [`hamza::family`](includes/family.hpp)

**File**: [includes/family.hpp](includes/family.hpp)

Address family wrapper (IPv4/IPv6).

#### [`hamza::file_descriptor`](includes/file_descriptor.hpp)

**File**: [includes/file_descriptor.hpp](includes/file_descriptor.hpp)

Cross-platform file descriptor wrapper.

### Utility Functions

#### Cross-platform Socket Functions

**File**: [includes/utilities.hpp](includes/utilities.hpp), [src/utilities.cpp](src/utilities.cpp)

- [`initialize_socket_library()`](includes/utilities.hpp) - Initialize Winsock on Windows
- [`cleanup_socket_library()`](includes/utilities.hpp) - Cleanup socket library
- [`close_socket(socket_t)`](includes/utilities.hpp) - Close socket cross-platform
- [`is_valid_socket(socket_t)`](includes/utilities.hpp) - Check socket validity
- [`is_socket_connected(socket_t)`](includes/utilities.hpp) - Check connection status

#### Network Utilities

- [`convert_ip_address_to_network_order()`](includes/utilities.hpp) - Convert IP to network byte order
- [`get_ip_address_from_network_address()`](includes/utilities.hpp) - Extract IP from sockaddr
- [`convert_host_to_network_order()`](includes/utilities.hpp) - Convert port to network order
- [`convert_network_order_to_host()`](includes/utilities.hpp) - Convert port to host order
- [`get_random_free_port()`](includes/utilities.hpp) - Get available random port
- [`is_valid_port(port)`](includes/utilities.hpp) - Validate port number
- [`is_free_port(port)`](includes/utilities.hpp) - Check if port is available

### Exception Hierarchy

**File**: [includes/exceptions.hpp](includes/exceptions.hpp)

```
general_socket_exception
├── server_listener_exception
├── server_accept_exception
├── client_connection_exception
├── client_remove_exception
├── client_activity_exception
├── client_message_exception
└── client_disconnect_exception
```

All exceptions inherit from `std::runtime_error` and provide a [`type()`](includes/exceptions.hpp) method for exception identification.

## Examples

### Basic HTTP Server

```cpp
#include <http_server.hpp>

int main() {
    hamza_http::http_server server("0.0.0.0", 8080);

    server.set_request_callback([](hamza_http::http_request& req, hamza_http::http_response& res) {
        if (req.get_uri() == "/") {
            res.set_status(200, "OK");
            res.add_header("Content-Type", "text/html");
            res.set_body("<h1>Welcome!</h1>");
        } else {
            res.set_status(404, "Not Found");
            res.set_body("Page not found");
        }
        res.end();
    });

    server.set_error_callback([](std::shared_ptr<hamza::general_socket_exception> e) {
        std::cerr << "Server error: " << e->what() << std::endl;
    });

    server.run();
    return 0;
}
```

### JSON API Server

```cpp
server.set_request_callback([](hamza_http::http_request& req, hamza_http::http_response& res) {
    res.add_header("Content-Type", "application/json");

    if (req.get_method() == "GET" && req.get_uri() == "/api/status") {
        res.set_status(200, "OK");
        res.set_body(R"({"status": "running", "version": "1.0"})");
    } else if (req.get_method() == "POST" && req.get_uri() == "/api/data") {
        // Process POST data
        std::string body = req.get_body();
        res.set_status(201, "Created");
        res.set_body(R"({"message": "Data received"})");
    } else {
        res.set_status(404, "Not Found");
        res.set_body(R"({"error": "Endpoint not found"})");
    }

    res.end();
});
```

### Low-Level Socket Examples

For developers who want to use the raw socket API without the higher-level abstractions:

#### Simple TCP Server (Echo Server)

```cpp
#include <socket.hpp>

int main() {
    hamza::initialize_socket_library();

    // Create server address and socket
    hamza::socket_address server_addr(
        hamza::ip_address("127.0.0.1"),
        hamza::port(8080),
        hamza::family(AF_INET)
    );
    hamza::socket server_socket(server_addr, hamza::Protocol::TCP, true);

    server_socket.listen(5);
    std::cout << "TCP Server listening on port 8080..." << std::endl;

    while (true) {
        auto client_socket = server_socket.accept();

        // Handle client in separate thread
        std::thread([client_socket]() {
            while (client_socket->is_connected()) {
                hamza::data_buffer data = client_socket->receive_on_connection();
                if (data.size() == 0) break;

                // Echo back
                std::string response = "Echo: " + data.to_string();
                client_socket->send_on_connection(hamza::data_buffer(response));
            }
        }).detach();
    }

    hamza::cleanup_socket_library();
    return 0;
}
```

#### Simple UDP Server

```cpp
#include <socket.hpp>

int main() {
    hamza::initialize_socket_library();

    // Create UDP server
    hamza::socket_address server_addr(
        hamza::ip_address("127.0.0.1"),
        hamza::port(8081),
        hamza::family(AF_INET)
    );
    hamza::socket server_socket(server_addr, hamza::Protocol::UDP, true);

    std::cout << "UDP Server listening on port 8081..." << std::endl;

    while (true) {
        hamza::socket_address client_addr(
            hamza::ip_address("0.0.0.0"),
            hamza::port(0),
            hamza::family(AF_INET)
        );

        // Receive from any client
        hamza::data_buffer data = server_socket.receive(client_addr);
        std::cout << "Received: " << data.to_string() << std::endl;

        // Echo back to sender
        std::string response = "UDP Echo: " + data.to_string();
        server_socket.send_to(client_addr, hamza::data_buffer(response));
    }

    hamza::cleanup_socket_library();
    return 0;
}
```

The examples demonstrate:

- **TCP Server**: Multi-threaded echo server handling multiple clients
- **UDP Server**: Connectionless echo server responding to any client
- **TCP Client**: Connects and sends multiple test messages
- **UDP Client**: Sends datagrams and receives responses

## Platform Support

### Windows

- Uses Winsock2 API
- Automatic library initialization/cleanup
- MSVC, MinGW, and Clang support

### Linux/Unix

- Uses POSIX socket API
- Supports all major Linux distributions
- GCC and Clang support

### About CMakeLists

The CMakeLists.txt file is the heart of the build system and contains several important configurations that control how the library is built and tested.

### Build System Overview

The CMake configuration provides a flexible build system that adapts based on environment variables and build modes:

```cmake
cmake_minimum_required(VERSION 3.10)
project(http_server)
set(CMAKE_CXX_STANDARD 17)
```

**Key Components:**

1. **Source File Collection**:

   ```cmake
   file(GLOB SRC_FILES src/*.cpp)
   include_directories(${CMAKE_SOURCE_DIR}/includes)
   ```

   - Automatically finds all `.cpp` files in the `src/` directory
   - Sets up include paths for header files

2. **Debug Mode Configuration**:
   ```cmake
   set(DEBUG_MODE ON CACHE BOOL "Enable Debug Mode")
   if(DEBUG_MODE)
       add_definitions(-DDEBUG_MODE)
   endif()
   ```
   - Enables a `DEBUG_MODE` preprocessor definition
   - Can be used in C++ code for conditional compilation:
     ```cpp
     #ifdef DEBUG_MODE
         std::cout << "Debug: " << debug_info << std::endl;
     #endif
     ```

### Environment File (.env) Loading

The CMakeLists includes a custom function to load environment variables from a `.env` file:

```cmake
function(load_env_file env_file)
    # Reads .env file and sets CMake variables
    # Supports KEY=VALUE format
    # Ignores comments (lines starting with #)
    # Strips whitespace from variable names and values
endfunction()

load_env_file(${CMAKE_SOURCE_DIR}/.env)
```

**Supported .env Format:**

```properties
# This is a comment
LOCAL_TEST=1
DEBUG_MODE=ON
# Another comment
CUSTOM_VAR=value
```

### LOCAL_TEST Variable - Development vs Production Builds

The `LOCAL_TEST` variable is the key differentiator between development testing and library production builds:

#### When LOCAL_TEST=1 (Development Mode)

```properties
# .env file
LOCAL_TEST=1
```

**What happens:**

1. **Compiler Flags Change**:

   ```cmake
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -fsanitize=address -g -O0")
   ```

   - `-Wall -Wextra -pedantic`: Enable comprehensive warnings
   - `-fsanitize=address`: **AddressSanitizer** for memory error detection
   - `-g`: Include debugging symbols
   - `-O0`: Disable optimizations for better debugging

2. **Build Target Changes**:

   ```cmake
   add_executable(http_server app.cpp ${SRC_FILES})
   ```

   - Creates an **executable** that includes `app.cpp`
   - Links the library code directly into the executable
   - Perfect for testing and development

3. **Memory Safety Testing**:
   - AddressSanitizer detects:
     - Buffer overflows
     - Use-after-free errors
     - Memory leaks
     - Double-free errors
   - Runtime overhead but catches memory bugs early

**Use Case**: Development, testing, debugging, and learning

#### When LOCAL_TEST≠1 or not set (Production Mode)

```properties
# .env file is empty or LOCAL_TEST=0
```

**What happens:**

1. **Optimized Compiler Flags**:

   ```cmake
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
   ```

   - Uses default compiler optimizations
   - No AddressSanitizer overhead
   - Smaller binary size

2. **Build Target Changes**:

   ```cmake
   add_library(http_server STATIC ${SRC_FILES})
   ```

   - Creates a **static library** (.a file on Linux, .lib on Windows)
   - `app.cpp` is NOT included in the library
   - Other projects can link against this library

3. **Production Ready**:
   - Optimized for performance
   - No debugging overhead
   - Clean library interface

**Use Case**: Production deployments, library distribution

### Build Configuration Examples

#### Development Build (with LOCAL_TEST=1)

```bash
# .env contains: LOCAL_TEST=1
./run.sh

# Result: Creates executable ./build/http_server
# Memory checking enabled, full debugging support
./build/http_server  # Runs the app.cpp example
```

#### Production Build (library mode)

```bash
# .env is empty or LOCAL_TEST=0
mkdir build && cd build
cmake ..
make

# Result: Creates static library ./build/libhttp_server.a
# Other projects can link against this library
```

#### Using the Library in Another Project

```cmake
# Another project's CMakeLists.txt
find_library(HAMZA_HTTP_LIB http_server PATHS /path/to/your/build)
target_link_libraries(my_app ${HAMZA_HTTP_LIB})
```

### Advanced Build Options

You can override CMake variables from command line:

```bash
# Force debug mode regardless of .env
cmake -DDEBUG_MODE=ON ..

# Disable debug mode
cmake -DDEBUG_MODE=OFF ..

# Set custom compiler flags
cmake -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
```

### Memory Testing with AddressSanitizer

When `LOCAL_TEST=1`, AddressSanitizer helps catch memory issues:

```cpp
// This would be caught by AddressSanitizer:
int* ptr = new int[10];
delete[] ptr;
ptr[0] = 42;  // Use-after-free - ASAN will detect this!
```

**AddressSanitizer Output Example:**

```
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x60200000eff0
READ of size 4 at 0x60200000eff0 thread T0
    #0 0x4567890 in main app.cpp:15
```

### Best Practices

1. **Development Workflow**:

   - Set `LOCAL_TEST=1` in `.env` for development
   - Use `./run.sh` for quick builds and testing
   - Monitor AddressSanitizer output for memory issues

2. **Production Deployment**:

   - Remove or set `LOCAL_TEST=0` for production builds
   - Build static library for distribution
   - Use release optimizations

3. **Continuous Integration**:

   - Test with both `LOCAL_TEST=1` (memory checking) and `LOCAL_TEST=0` (performance)
   - Ensure library builds cleanly without `app.cpp` dependencies

4. **Cross-Platform Builds**:
   - The same CMakeLists.txt works on Windows, Linux, and macOS
   - AddressSanitizer support varies by compiler (best with GCC/Clang)

This flexible build system allows seamless switching between development testing and production library builds, making it ideal for both learning and real-world usage.
