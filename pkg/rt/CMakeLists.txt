add_library(akx_rt STATIC
    akx_rt.c
    akx_rt_compiler.c
)

target_include_directories(akx_rt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pkg/cell
    ${CMAKE_SOURCE_DIR}/pkg/sv
    ${AK24_INCLUDE_DIR}
)

target_link_libraries(akx_rt PUBLIC
    akx_cell
    akx_sv
    ${AK24_LIBRARY}
    pthread
)

if(OPENSSL_FOUND)
    target_link_libraries(akx_rt PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

