cmake_minimum_required(VERSION 3.20)

project(akx VERSION 0.1.0 LANGUAGES C)

if(DEFINED ENV{AKX_HOME})
    set(AKX_HOME $ENV{AKX_HOME} CACHE PATH "AKX installation directory")
else()
    set(AKX_HOME "$ENV{HOME}/.akx" CACHE PATH "AKX installation directory")
endif()

message(STATUS "AKX_HOME: ${AKX_HOME}")

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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ak24)
ensure_ak24_installed()

find_library(AK24_LIBRARY
    NAMES ak24_kernel
    PATHS ${AK24_LIB_DIR}
    NO_DEFAULT_PATH
    REQUIRED
)

message(STATUS "Found AK24 library: ${AK24_LIBRARY}")

set(AK24_LIBRARIES ${AK24_LIBRARY})

if(AK24_GC_LIBRARY)
    list(APPEND AK24_LIBRARIES ${AK24_GC_LIBRARY})
endif()

if(AK24_TCC_LIBRARY)
    list(APPEND AK24_LIBRARIES ${AK24_TCC_LIBRARY})
endif()

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
add_subdirectory(nucleus)
add_subdirectory(pkg/rt)
add_subdirectory(cmd/akx)

install(TARGETS akx
    RUNTIME DESTINATION ${AKX_HOME}/bin
)

install(DIRECTORY nucleus/
    DESTINATION ${AKX_HOME}/nucleus
    FILES_MATCHING 
    PATTERN "*.c"
    PATTERN "*.h"
    PATTERN "*.txt"
    PATTERN "*.md"
)

install(DIRECTORY stdlib/
    DESTINATION ${AKX_HOME}/stdlib
    FILES_MATCHING PATTERN "*.akx"
)

