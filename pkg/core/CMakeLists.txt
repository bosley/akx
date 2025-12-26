add_library(akx_core STATIC
    akx_core.c
)

target_include_directories(akx_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AK24_INCLUDE_DIR}
)

target_link_libraries(akx_core PUBLIC
    ${AK24_LIBRARIES}
    pthread
)

if(OPENSSL_FOUND)
    target_link_libraries(akx_core PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

