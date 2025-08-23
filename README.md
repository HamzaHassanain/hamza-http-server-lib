# Hamza HTTP Server Library

An HTTP server written in C++17. Provides some networking stack with TCP server capabilities.

## Table of Contents

- [Overview](#overview)

- [Building the Project](#building-the-project)

  - [Prerequisites](#prerequisites)
  - [How to Build](#how-to-build)

    - [Prerequisites](#prerequisites-1)
      - [For Linux (Ubuntu/Debian)](#for-linux-ubuntudebian)
      - [For Linux (CentOS/RHEL/Fedora)](#for-linux-centosrhelfedora)
      - [For Windows](#for-windows)
    - [Step 1: Clone the Repository](#step-1-clone-the-repository)
    - [Step 2: Understanding Build Modes](#step-2-understanding-build-modes)
      - [Development Mode (HTTP_LOCAL_TEST=1)](#development-mode-http_local_test1)
      - [Library Mode (HTTP_LOCAL_TEST≠1)](#library-mode-http_local_test1)
    - [Step 3: Configure Build Mode](#step-3-configure-build-mode)
      - [For Development/Testing](#for-developmenttesting)
      - [For Library Distribution](#for-library-distribution)
    - [Step 4: Build the Project](#step-4-build-the-project)
      - [Option A: Using the Provided Script (Linux/Mac)](#option-a-using-the-provided-script-linuxmac)
      - [Option B: Manual Build (Linux/Mac/Windows)](#option-b-manual-build-linuxmacwindows)
      - [Windows-Specific Build Instructions](#windows-specific-build-instructions)
    - [Step 5: Run the Project](#step-5-run-the-project)
      - [Development Mode (HTTP_LOCAL_TEST=1)](#development-mode-http_local_test1)
      - [Library Mode (HTTP_LOCAL_TEST≠1)](#library-mode-http_local_test1)
    - [Project Structure After Build](#project-structure-after-build)
    - [Using the Library in Your Own Project](#using-the-library-in-your-own-project)

    - [API Documentation](#api-documentation)

- [HTTP Server Examples](#http-server-examples)

## Overview

This library provides a modern C++17 implementation of an HTTP server built on fundamental networking principles. It abstracts away the complexity of low-level socket programming while providing full control over HTTP request/response handling. The library is built over another library I built [Simple C++ Socket Library](https://github.com/HamzaHassanain/hamza-socket-lib) that it uses to handle all the low-level socket operations.

## Building The Project

### How to build

This guide will walk you through cloning, building, and running the project on both Linux and Windows systems.

### Prerequisites

Before you start, ensure you have the following installed:

#### For Linux (Ubuntu/Debian):

```bash
# Update package list
sudo apt update

# Install essential build tools
sudo apt install build-essential cmake git

# Verify installations
gcc --version      # Should show GCC 7+ for C++17 support
cmake --version    # Should show CMake 3.10+
git --version      # Any recent version
```

#### For Linux (CentOS/RHEL/Fedora):

```bash
# For CentOS/RHEL
sudo yum groupinstall "Development Tools"
sudo yum install cmake git

# For Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake git
```

#### For Windows:

1. **Install Git**: Download from [git-scm.com](https://git-scm.com/download/win)
2. **Install CMake**: Download from [cmake.org](https://cmake.org/download/)
3. **Install a C++ Compiler** (choose one):
   - **Visual Studio 2019/2022** (recommended): Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/)
   - **MinGW-w64**: Download from [mingw-w64.org](https://www.mingw-w64.org/)
   - **MSYS2**: Download from [msys2.org](https://www.msys2.org/)

### Step 1: Clone the Repository

Open your terminal (Linux) or Command Prompt/PowerShell (Windows):

```bash
# Clone the repository
git clone https://github.com/HamzaHassanain/hamza-http-server-lib.git

# Navigate to the project directory
cd hamza-http-server-lib

# Verify you're in the right directory
ls -la  # Linux/Mac
dir     # Windows CMD
```

### Step 2: Understanding Build Modes

The project supports two build modes controlled by the `.env` file:

#### Development Mode (HTTP_LOCAL_TEST=1)

- Builds an **executable** for testing
- Includes debugging symbols and AddressSanitizer
- Perfect for learning and development

#### Library Mode (HTTP_LOCAL_TEST≠1)

- Builds a **static library** for distribution
- Optimized for production use
- Other projects can link against it

### Step 3: Configure Build Mode

Create a `.env` file in the project root:

#### For Development/Testing

```bash
# Create .env file for development mode
echo "HTTP_LOCAL_TEST=1" > .env

# On Windows (PowerShell):
echo "HTTP_LOCAL_TEST=1" | Out-File -FilePath .env -Encoding ASCII

# On Windows (CMD):
echo HTTP_LOCAL_TEST=1 > .env
```

#### For Library Distribution:

```bash
# Create .env file for library mode (or leave empty)
echo "HTTP_LOCAL_TEST=0" > .env
```

### Step 4: Build the Project

#### Option A: Using the Provided Script (Linux/Mac)

```bash
# Make the script executable
chmod +x run.sh

# Run the build script
./run.sh
```

This script automatically:

1. Creates a `build` directory
2. Runs CMake configuration
3. Compiles the project
4. Runs the executable (if in development mode)

#### Option B: Manual Build (Linux/Mac/Windows)

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build .

# Alternative: use make on Linux/Mac
make -j$(nproc)  # Linux
make -j$(sysctl -n hw.ncpu)  # Mac
```

#### Windows-Specific Build Instructions

**Using Visual Studio:**

```cmd
# Open Developer Command Prompt for Visual Studio
mkdir build
cd build
cmake ..
cmake --build . --config Release

```

**Using MinGW:**

```cmd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

### Step 5: Run the Project

#### Development Mode (HTTP_LOCAL_TEST=1):

```bash
# Linux/Mac
./build/http_server

# Windows
.\build\Debug\http_server.exe   # Debug build
.\build\Release\http_server.exe # Release build
```

#### Library Mode (HTTP_LOCAL_TEST):

The build will create a static library file:

- **Linux/Mac**: `build/libhttp_server.a`
- **Windows**: `build/http_server.lib` or `build/Debug/http_server.lib`

### Project Structure After Build

```
hamza-http-server-lib/
├── build/                     # Build artifacts
│   ├── http_server           # Executable (Linux, development mode)
│   ├── http_server.exe       # Executable (Windows, development mode)
│   ├── libhttp_server.a      # Static library (Linux, library mode)
│   └── http_server.lib       # Static library (Windows, library mode)
├── src/                      # Source files
├── includes/                 # Header files
├── app.cpp                   # Example application
├── CMakeLists.txt           # Build configuration
├── .env                     # Build mode configuration
└── run.sh                   # Build script (Linux/Mac)
```

### Using the Library in Your Own Project

Once built in library mode, you can use it in other projects:

```cmake
# Your project's CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(my_project)

# Find the library
find_library(HAMZA_HTTP_LIB
    NAMES http_server
    PATHS /path/to/hamza-http-server-lib/build
)

# Link against the library
add_executable(my_app main.cpp)
target_link_libraries(my_app ${HAMZA_HTTP_LIB})
target_include_directories(my_app PRIVATE /path/to/hamza-http-server-lib/includes)
```

## API Documentation

Below is a reference of the core public classes and their commonly used methods. Include the corresponding header before use.

For detailed method signatures and advanced usage patterns, consult the comprehensive inline documentation in the header files located in `includes/` directory.

### hamza_http::http_request

```cpp
#include "http_request.hpp"

// - Purpose: Represents an HTTP request with move-only semantics
// - Features: Encapsulates HTTP method, URI, version, headers, and body
// - Move-only: move constructor available, copy operations deleted
// - Key methods:
  std::string get_method() const                              // — HTTP method (GET, POST, etc.)
  std::string get_uri() const                                 // — Request URI/path
  std::string get_version() const                             // — HTTP version (e.g., "HTTP/1.1")
  std::vector<std::string> get_header(const std::string &name) const  // — Get all values for specific header
  std::vector<std::pair<std::string, std::string>> get_headers() const // — Get all headers
  std::string get_body() const                                // — Request body content
  void destroy(bool Isure)                                    // — Safely destroy request and close connection
```

### hamza_http::http_response

```cpp
#include "http_response.hpp"

// - Purpose: Represents an HTTP response with move-only semantics
// - Features: Status code, headers, body, trailers support
// - Move-only: move constructor available, copy operations deleted
// - Key methods:
  void set_body(const std::string &body)                     // — Set response body content
  void set_status(int status_code, const std::string &status_message) // — Set HTTP status
  void set_version(const std::string &version)               // — Set HTTP version
  void add_header(const std::string &name, const std::string &value)  // — Add response header
  void add_trailer(const std::string &name, const std::string &value) // — Add trailer header
  std::string get_body() const                                // — Get response body
  std::string get_version() const                             // — Get HTTP version
  std::string get_status_message() const                     // — Get status message
  int get_status_code() const                                 // — Get status code
  std::vector<std::string> get_header(const std::string &name) const  // — Get header values
  std::vector<std::string> get_trailer(const std::string &name) const // — Get trailer values
  std::string to_string() const                               // — Convert to HTTP string format
  void send()                                                 // — Send HTTP response to client
  void end()                                                  // — End response and close connection
```

### hamza_http::http_server

```cpp
#include "http_server.hpp"

// - Purpose: High-level HTTP/1.1 server built on TCP server infrastructure
// - Features: Inheritance-based, Callback-driven architecture, epoll-based I/O multiplexing
// - Move-only: copy/move operations deleted for resource safety

// - Constructors:
  http_server(const socket_address &addr, int timeout_ms = 1000)     // — Bind to socket address, creates server socket
  http_server(int port, const std::string &ip = "0.0.0.0", int timeout_ms = 1000) // — Convenience constructor for IP:port

// - Core Server Control:
  void listen()                                               // — [virtual] Start epoll event loop, calls epoll_server::listen()

// - Request Processing Callbacks:
  void set_request_callback(std::function<void(http_request &, http_response &)>) // — [user calls] Set main HTTP handler, called by on_request_received()
  void set_headers_received_callback(std::function<void(std::shared_ptr<connection>, const std::multimap<std::string, std::string> &, const std::string &, const std::string &, const std::string &, const std::string &)>) // — [user calls] Early header processing, called by on_headers_received()

// - Server Lifecycle Callbacks:
  void set_listen_success_callback(std::function<void()>)    // — [user calls] Server startup notification, called by on_listen_success()
  void set_server_stopped_callback(std::function<void()>)    // — [user calls] Server shutdown notification, called by on_shutdown_success()
  void set_error_callback(std::function<void(const std::exception &)>) // — [user calls] Error handling, called by on_exception_occurred()

// - Connection Management Callbacks:
  void set_client_connected_callback(std::function<void(std::shared_ptr<connection>)>) // — [user calls] Client connect events, called by on_connection_opened()
  void set_client_disconnected_callback(std::function<void(std::shared_ptr<connection>)>) // — [user calls] Client disconnect events, called by on_connection_closed()
  void set_waiting_for_activity_callback(std::function<void()>) // — [user calls] Server idle periods, called by on_waiting_for_activity()

// - Protected Virtual Methods (for inheritance):
  void on_message_received(std::shared_ptr<connection>, const data_buffer &) // — [virtual override] Parse HTTP request, auto-called by epoll_server
  void on_listen_success()                                    // — [virtual override] Server started, auto-called by epoll_server
  void on_shutdown_success()                                  // — [virtual override] Server stopped, auto-called by epoll_server
  void on_exception_occurred(const std::exception &)         // — [virtual override] Handle errors, auto-called by epoll_server
  void on_connection_opened(std::shared_ptr<connection>)      // — [virtual override] Client connected, auto-called by epoll_server
  void on_connection_closed(std::shared_ptr<connection>)      // — [virtual override] Client disconnected, auto-called by epoll_server
  void on_waiting_for_activity()                             // — [virtual override] Server idle, auto-called by epoll_server
  void on_request_received(http_request &, http_response &)   // — [virtual] Process parsed HTTP request, called by on_message_received()
  void on_headers_received(std::shared_ptr<connection>, const std::multimap<std::string, std::string> &, const std::string &, const std::string &, const std::string &, const std::string &) // — [virtual] Early header processing, called by http_message_handler
```

## Usage Examples

The library supports two main usage patterns:

### 1. Callback-Based Approach

```cpp
#include <http_server.hpp>

void handle_request(hamza_http::http_request &req, hamza_http::http_response &res) {
    res.set_status(200, "OK");
    res.add_header("Content-Type", "text/plain");
    res.set_body("Hello World!");
    res.send();
}

int main() {
    hamza_http::http_server server(8080);
    server.set_request_callback(handle_request);
    server.listen();
    return 0;
}
```

### 2. Inheritance-Based Approach

```cpp
#include <http_server.hpp>

class MyServer : public hamza_http::http_server {
public:
    MyServer(int port) : hamza_http::http_server(port) {}

protected:
    void on_request_received(hamza_http::http_request &req, hamza_http::http_response &res) override {
        res.set_status(200, "OK");
        res.add_header("Content-Type", "text/plain");
        res.set_body("Hello from custom server!");
        res.send();
    }
};

int main() {
    MyServer server(8080);
    server.listen();
    return 0;
}
```

# HTTP Server Examples

### Complete Examples

For examples demonstrating both approaches, see the [`examples/`](examples/) directory:

- **[`callback_based_server.cpp`](examples/callback_based_server.cpp)** - Server using callbacks with routing, error handling, and HTML responses
- **[`inheritance_based_server.cpp`](examples/inheritance_based_server.cpp)** - Server using inheritance with custom routing system, statistics, and JSON API endpoints

### Take a look at the examples directory.

## Examples Overview

### 1. Callback-Based Server (`callback_based_server.cpp`)

**Approach:** Uses callback functions to handle server events
**Best for:** Simple servers, rapid prototyping, functional programming style

**Features:**

- Request handling via callback functions
- Event-driven architecture with separate callbacks for different events
- No inheritance required
- Clean separation of concerns

**Key Callbacks Used:**

- `set_request_callback()` - Main HTTP request handling
- `set_client_connected_callback()` - Client connection events
- `set_client_disconnected_callback()` - Client disconnection events
- `set_listen_success_callback()` - Server startup notification
- `set_error_callback()` - Error handling
- `set_waiting_for_activity_callback()` - Idle period handling

### 2. Inheritance-Based Server (`inheritance_based_server.cpp`)

**Approach:** Extends the `http_server` class and overrides virtual methods
**Best for:** Complex servers, object-oriented design, custom behavior

**Features:**

- Custom routing system with method/path mapping
- Request statistics and connection tracking
- Virtual method overrides for fine-grained control
- Object-oriented design with encapsulated state

**Virtual Methods Overridden:**

- `on_request_received()` - Custom request processing with routing
- `on_connection_opened()` - Custom connection handling with statistics
- `on_connection_closed()` - Custom disconnection handling
- `on_listen_success()` - Custom server startup behavior
- `on_exception_occurred()` - Custom error handling
- `on_waiting_for_activity()` - Custom idle period processing

## Building and Running

### Prerequisites (Make sure you build as library not executable)

Make sure the main library is built first:

```bash
cd .. # Go to project root
./run.sh # Build the library
```

### Compile the Examples

**Callback-Based Server:**

```bash
cd examples
g++ -std=c++17 -I../includes -L../build -o callback_server callback_based_server.cpp -lhttp_server -pthread
```

**Inheritance-Based Server:**

```bash
cd examples
g++ -std=c++17 -I../includes -L../build -o inheritance_server inheritance_based_server.cpp -lhttp_server -pthread
```

### Run the Servers

**Start Callback-Based Server:**

```bash
./callback_server
```

**Start Inheritance-Based Server:**

```bash
./inheritance_server
```

Both servers will start on `http://localhost:8080`

## Testing the Servers

### Using a Web Browser

Visit `http://localhost:8080` to see the home page and available endpoints.

### Using cURL

**Basic GET requests:**

```bash
curl http://localhost:8080/
curl http://localhost:8080/hello
curl http://localhost:8080/info  # callback-based
curl http://localhost:8080/stats # inheritance-based
```

**POST requests:**

```bash
# Echo endpoint
curl -X POST -d "Hello World" http://localhost:8080/echo          # callback-based
curl -X POST -d "Hello World" http://localhost:8080/api/echo      # inheritance-based

# Uppercase endpoint (inheritance-based only)
curl -X POST -d "make me loud" http://localhost:8080/api/uppercase
```

**JSON endpoints:**

```bash
curl http://localhost:8080/api/info  # inheritance-based
curl http://localhost:8080/headers   # callback-based
```

## Key Differences

| Aspect               | Callback-Based                 | Inheritance-Based               |
| -------------------- | ------------------------------ | ------------------------------- |
| **Code Structure**   | Functional, separate functions | Object-oriented, class methods  |
| **State Management** | Global/static variables        | Class member variables          |
| **Routing**          | Manual if/else logic           | Custom routing system with maps |
| **Extensibility**    | Add more callbacks             | Override more virtual methods   |
| **Complexity**       | Simpler, more direct           | More sophisticated, flexible    |
| **Memory**           | Lower overhead                 | Slightly higher overhead        |
| **Maintenance**      | Good for small servers         | Better for large applications   |
