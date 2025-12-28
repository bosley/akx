add_library(akx_repl STATIC
    repl.c
    repl_buffer.c
    repl_display.c
    repl_history.c
    repl_input.c
)

target_include_directories(akx_repl PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pkg/core
    ${CMAKE_SOURCE_DIR}/pkg/cell
    ${CMAKE_SOURCE_DIR}/pkg/rt
    ${CMAKE_SOURCE_DIR}/pkg/sv
    ${AK24_INCLUDE_DIR}
)

target_link_libraries(akx_repl PUBLIC
    akx_core
    akx_cell
    akx_rt
    akx_sv
    ${AK24_LIBRARIES}
    pthread
)

if(OPENSSL_FOUND)
    target_link_libraries(akx_repl PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

