add_executable(akx
    main.c
    akx.c
    commands.c
    nucleus_info.c
    nucleus_list.c
    help.c
)

target_include_directories(akx PRIVATE
    ${CMAKE_SOURCE_DIR}/pkg/core
    ${CMAKE_SOURCE_DIR}/pkg/cell
    ${CMAKE_SOURCE_DIR}/pkg/sv
    ${CMAKE_SOURCE_DIR}/pkg/repl
    ${AK24_INCLUDE_DIR}
)

target_link_libraries(akx PRIVATE
    akx_core
    akx_cell
    akx_rt
    akx_sv
    akx_repl
    ${AK24_LIBRARIES}
    pthread
)

if(OPENSSL_FOUND)
    target_link_libraries(akx PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endif()

if(APPLE)
    set_target_properties(akx PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LINK_FLAGS "-Wl,-w"
    )
else()
    set_target_properties(akx PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
endif()

