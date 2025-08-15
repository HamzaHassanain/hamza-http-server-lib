cmake_minimum_required(VERSION 3.10)
project(Socket_example)


# define a variable called debug, that I can acess in a c++ code, to enable debugging
set(DEBUG_MODE ON CACHE BOOL "Enable Debug Mode") 

if(DEBUG_MODE)
    add_definitions(-DDEBUG_MODE)
endif()

# add debuging for c++ gcc

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -fsanitize=address -g -O0")


file(GLOB SRC_FILES src/*.cpp)

# link all cpp files inside framework folder, framework/src
# file(GLOB WEB_SRC_FILES web/src/*.cpp)

set(CMAKE_CXX_STANDARD 17)
# Add include directory for header.hpp
include_directories(${CMAKE_SOURCE_DIR}/includes)
# include_directories(${CMAKE_SOURCE_DIR}/web)
# include_directories(${CMAKE_SOURCE_DIR}/lib)

add_executable(  socket_example app.cpp ${SRC_FILES} ${WEB_SRC_FILES} )