cmake_minimum_required(VERSION 3.20)

project(akx VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(UNIX OR APPLE)
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Wextra -Wpedantic")
endif()

if(DEFINED ENV{AK24_HOME})
    set(AK24_HOME "$ENV{AK24_HOME}")
else()
    set(AK24_HOME "$ENV{HOME}/.ak24")
endif()

message(STATUS "AK24_HOME: ${AK24_HOME}")

if(NOT EXISTS "${AK24_HOME}/include/ak24/kernel.h")
    message(FATAL_ERROR "AK24 not found at ${AK24_HOME}. Please install AK24 first.")
endif()

set(AK24_INCLUDE_DIR "${AK24_HOME}/include")
set(AK24_LIB_DIR "${AK24_HOME}/lib")

find_library(AK24_LIBRARY
    NAMES ak24_kernel
    PATHS ${AK24_LIB_DIR}
    NO_DEFAULT_PATH
    REQUIRED
)

message(STATUS "Found AK24 library: ${AK24_LIBRARY}")

find_package(OpenSSL QUIET)
if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
else()
    message(STATUS "OpenSSL not found, trying Homebrew paths...")
    if(APPLE)
        set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl@3)
        find_package(OpenSSL QUIET)
        if(NOT OPENSSL_FOUND)
            set(OPENSSL_ROOT_DIR /usr/local/opt/openssl@3)
            find_package(OpenSSL QUIET)
        endif()
        if(NOT OPENSSL_FOUND)
            set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl)
            find_package(OpenSSL QUIET)
        endif()
    endif()
    if(OPENSSL_FOUND)
        message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    else()
        message(WARNING "OpenSSL not found - TLS features may not work")
    endif()
endif()

add_subdirectory(pkg/core)
add_subdirectory(pkg/sv)
add_subdirectory(pkg/cell)
add_subdirectory(cmd/akx)

