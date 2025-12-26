add_library(akx_sv STATIC
    akx_sv.c
)

target_include_directories(akx_sv PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AK24_INCLUDE_DIR}
)

target_link_libraries(akx_sv PUBLIC
    ${AK24_LIBRARY}
)

