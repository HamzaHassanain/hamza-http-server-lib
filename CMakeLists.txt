cmake_minimum_required(VERSION 3.10)
project(http_server)


# Find Git package for submodule handling
find_package(Git QUIET)

# Initialize and update git submodules to latest versions
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    option(GIT_SUBMODULE_UPDATE_LATEST "Update submodules to latest remote commits" ON)
    
    if(GIT_SUBMODULE)
        message(STATUS "Initializing and updating submodules...")
        
        # First, initialize submodules if they don't exist
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_INIT_RESULT)
        if(NOT GIT_SUBMOD_INIT_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_INIT_RESULT}, please checkout submodules")
        endif()
        
        # If enabled, update submodules to latest remote commits
        if(GIT_SUBMODULE_UPDATE_LATEST)
            message(STATUS "Updating submodules to latest remote commits...")
            execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --remote --recursive
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                            RESULT_VARIABLE GIT_SUBMOD_UPDATE_RESULT)
            if(NOT GIT_SUBMOD_UPDATE_RESULT EQUAL "0")
                message(WARNING "git submodule update --remote --recursive failed with ${GIT_SUBMOD_UPDATE_RESULT}, continuing with current submodule versions")
            else()
                message(STATUS "Submodules updated to latest versions successfully")
            endif()
        endif()
    endif()
endif()

# Check if submodules exist
set(REQUIRED_SUBMODULES "socket-lib")  # Add more submodules here as needed
foreach(submodule ${REQUIRED_SUBMODULES})
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/${submodule}/CMakeLists.txt")
        message(FATAL_ERROR "The ${submodule} submodule was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
    endif()
endforeach()


# define a variable called debug, that I can acess in a c++ code, to enable debugging
set(DEBUG_MODE ON CACHE BOOL "Enable Debug Mode") 

if(DEBUG_MODE)
    add_definitions(-DDEBUG_MODE)
endif()


file(GLOB SRC_FILES src/*.cpp)


set(CMAKE_CXX_STANDARD 17)
include_directories(${CMAKE_SOURCE_DIR}/includes)
include_directories(${CMAKE_SOURCE_DIR}/libs)




# Function to read .env file and set variables
function(load_env_file env_file)
    if(EXISTS ${env_file})
        file(READ ${env_file} ENV_CONTENT)
        string(REPLACE "\n" ";" ENV_LINES ${ENV_CONTENT})
        
        foreach(line ${ENV_LINES})
            # Skip empty lines and comments
            if(line AND NOT line MATCHES "^#")
                string(FIND ${line} "=" eq_pos)
                if(eq_pos GREATER -1)
                    string(SUBSTRING ${line} 0 ${eq_pos} var_name)
                    math(EXPR val_start "${eq_pos} + 1")
                    string(SUBSTRING ${line} ${val_start} -1 var_value)
                    
                    # Remove any whitespace
                    string(STRIP ${var_name} var_name)
                    string(STRIP ${var_value} var_value)
                    
                    # Set the variable in parent scope
                    set(${var_name} ${var_value} PARENT_SCOPE)
                    message(STATUS "Loaded env variable: ${var_name}=${var_value}")
                endif()
            endif()
        endforeach()
    else()
        message(WARNING ".env file not found at ${env_file}")
    endif()
endfunction()

# Load environment variables from .env file
load_env_file(${CMAKE_SOURCE_DIR}/.env)



if(LOCAL_TEST AND LOCAL_TEST STREQUAL "1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -fsanitize=address -g -O0")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()


# Add include directories for each submodule
foreach(submodule ${REQUIRED_SUBMODULES})
    if(EXISTS "${PROJECT_SOURCE_DIR}/libs/${submodule}/includes")
        include_directories(${CMAKE_SOURCE_DIR}/libs/${submodule}/includes)
    endif()
endforeach()

# Add subdirectories for each submodule
foreach(submodule ${REQUIRED_SUBMODULES})
    # Temporarily suppress warnings for submodules
    set(CMAKE_CXX_FLAGS_BACKUP "${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")  # Suppress all warnings for submodules
    add_subdirectory(libs/${submodule})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_BACKUP}")  # Restore warnings for main project
endforeach()



if(LOCAL_TEST AND LOCAL_TEST STREQUAL "1")
    add_executable(  http_server app.cpp ${SRC_FILES}   )
else()
    add_library(http_server STATIC ${SRC_FILES})
endif()

set(SUBMODULE_LIBRARIES "socket_lib")  # Add more library names here as needed
target_link_libraries(http_server ${SUBMODULE_LIBRARIES})