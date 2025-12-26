add_library(akx_cell STATIC
    akx_cell.c
)

target_include_directories(akx_cell PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pkg/sv
    ${AK24_INCLUDE_DIR}
)

target_link_libraries(akx_cell PUBLIC
    akx_sv
    ${AK24_LIBRARY}
)

