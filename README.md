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

## Overview

This library provides a modern C++17 implementation of an HTTP server built on fundamental networking principles. It abstracts away the complexity of low-level socket programming while providing full control over HTTP request/response handling. The library is built over another library I built [Simple C++ Socket Library](https://github.com/HamzaHassanain/hamza-socket-lib) that it uses to handle all the low-level socket operations.

## Building The Project

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard networking libraries (automatically linked)

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

To be added
