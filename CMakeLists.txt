cmake_minimum_required(VERSION 3.10)
project(socket VERSION 1.0.0)

# Library configuration
set(DEBUG_MODE ON CACHE BOOL "Enable Debug Mode")
if(DEBUG_MODE)
    add_definitions(-DDEBUG_MODE)
endif()

# Define build the example applications
option(BUILD_EXAMPLES "Build example applications" ON)

# Collect source files
file(GLOB SRC_FILES src/*.cpp)

set(CMAKE_CXX_STANDARD 17)

# Create the library (both static and shared)
add_library(socket_static STATIC ${SRC_FILES} ${LIBS})
add_library(socket_shared SHARED ${SRC_FILES} ${LIBS})

# Set library properties
set_target_properties(socket_static PROPERTIES OUTPUT_NAME socket)
set_target_properties(socket_shared PROPERTIES OUTPUT_NAME socket)

# Include directories for the library
target_include_directories(socket_static PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/includes>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(socket_shared PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/includes>
    $<INSTALL_INTERFACE:include>
)

# Optional: Create an alias for easier usage
add_library(socket::static ALIAS socket_static)
add_library(socket::shared ALIAS socket_shared)

# option(BUILD_EXAMPLES "Build example applications" ON)
if(BUILD_EXAMPLES)
    add_executable(socket_example example.cpp)
    target_link_libraries(socket_example socket_static)
endif()

# Export configuration for find_package support
include(CMakePackageConfigHelpers)

# Install library files
install(TARGETS socket_static socket_shared
    EXPORT socketTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY includes/ DESTINATION include/hamza/socket)

# Export targets
install(EXPORT socketTargets
    FILE socketTargets.cmake
    NAMESPACE socket::
    DESTINATION lib/cmake/socket
)

# Create package config file
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/socketConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/socketConfig.cmake"
    INSTALL_DESTINATION lib/cmake/socket
)

# Create version file
write_basic_package_version_file(
    socketConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/socketConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/socketConfigVersion.cmake"
    DESTINATION lib/cmake/socket
)