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

add_executable(akx_cell_tests
    tests/main.c
    tests/tests.c
)

target_include_directories(akx_cell_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/pkg/sv
    ${CMAKE_SOURCE_DIR}/pkg/testing
    ${AK24_INCLUDE_DIR}
)

target_link_libraries(akx_cell_tests PRIVATE
    akx_cell
    akx_sv
    ${AK24_LIBRARY}
)

add_custom_target(run_akx_cell_tests ALL
    COMMAND akx_cell_tests
    DEPENDS akx_cell_tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running akx_cell tests..."
)

