file(READ "${CMAKE_CURRENT_SOURCE_DIR}/manifest.txt" MANIFEST_CONTENT)

set(COMPILED_NUCLEI "")
set(ALL_NUCLEI "")
set(NUCLEUS_SOURCES "")

string(REGEX REPLACE "\n" ";" MANIFEST_LINES "${MANIFEST_CONTENT}")
foreach(LINE ${MANIFEST_LINES})
    if(LINE MATCHES "^#" OR LINE STREQUAL "")
        continue()
    endif()
    
    string(REGEX MATCH "^([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([01])" _ ${LINE})
    set(SYMBOL ${CMAKE_MATCH_1})
    set(C_FUNCTION ${CMAKE_MATCH_2})
    set(REL_PATH ${CMAKE_MATCH_3})
    set(COMPILE_IN ${CMAKE_MATCH_4})
    
    list(APPEND ALL_NUCLEI "${SYMBOL}:${C_FUNCTION}:${REL_PATH}:${COMPILE_IN}")
    
    if(COMPILE_IN EQUAL 1)
        list(APPEND COMPILED_NUCLEI "${SYMBOL}:${C_FUNCTION}")
        list(APPEND NUCLEUS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${REL_PATH}")
    endif()
endforeach()

set(REGISTRY_H "${CMAKE_BINARY_DIR}/generated/builtin_registry.h")
set(REGISTRY_C "${CMAKE_BINARY_DIR}/generated/builtin_registry.c")
set(METADATA_H "${CMAKE_BINARY_DIR}/generated/builtin_metadata.h")

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

file(WRITE ${REGISTRY_H} "#ifndef BUILTIN_REGISTRY_H
#define BUILTIN_REGISTRY_H

#include \"akx_rt.h\"

void akx_rt_register_compiled_nuclei(akx_runtime_ctx_t *rt);

#endif
")

file(WRITE ${REGISTRY_C} "#include \"builtin_registry.h\"
#include \"akx_rt_builtins.h\"
#include \"akx_rt.h\"
#include \"akx_cell.h\"
#include <ak24/kernel.h>
#include <ak24/intern.h>
#include <ak24/context.h>
#include <ak24/lambda.h>
#include <ak24/buffer.h>
#include <ak24/filepath.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

")

foreach(NUCLEUS ${COMPILED_NUCLEI})
    string(REPLACE ":" ";" NUCLEUS_PARTS ${NUCLEUS})
    list(GET NUCLEUS_PARTS 0 SYMBOL)
    list(GET NUCLEUS_PARTS 1 C_FUNCTION)
    
    file(APPEND ${REGISTRY_C} "extern akx_cell_t *${C_FUNCTION}(akx_runtime_ctx_t *, akx_cell_t *);
")
endforeach()

file(APPEND ${REGISTRY_C} "
")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assert/helpers.c")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/assert/helpers.c" HELPERS_CONTENT)
    file(APPEND ${REGISTRY_C} "${HELPERS_CONTENT}\n\n")
endif()

foreach(SOURCE_FILE ${NUCLEUS_SOURCES})
    file(READ ${SOURCE_FILE} SOURCE_CONTENT)
    file(APPEND ${REGISTRY_C} "${SOURCE_CONTENT}\n\n")
endforeach()

file(APPEND ${REGISTRY_C} "

void akx_rt_register_compiled_nuclei(akx_runtime_ctx_t *rt) {
    if (!rt) return;
    
")

foreach(NUCLEUS ${COMPILED_NUCLEI})
    string(REPLACE ":" ";" NUCLEUS_PARTS ${NUCLEUS})
    list(GET NUCLEUS_PARTS 0 SYMBOL)
    list(GET NUCLEUS_PARTS 1 C_FUNCTION)
    
    file(APPEND ${REGISTRY_C} "    {
        akx_builtin_info_t *info = AK24_ALLOC(sizeof(akx_builtin_info_t));
        if (info) {
            info->function = ${C_FUNCTION};
            info->source_path = NULL;
            info->load_time = time(NULL);
            info->init_fn = NULL;
            info->deinit_fn = NULL;
            info->reload_fn = NULL;
            info->module_data = NULL;
            info->module_name = ak_intern(\"${SYMBOL}\");
            info->unit = NULL;
            akx_rt_add_builtin(rt, \"${SYMBOL}\", info);
            AK24_LOG_TRACE(\"Registered compiled nucleus: ${SYMBOL}\");
        }
    }
")
endforeach()

file(APPEND ${REGISTRY_C} "}
")

file(WRITE ${METADATA_H} "#ifndef BUILTIN_METADATA_H
#define BUILTIN_METADATA_H

#include <stddef.h>

typedef struct {
    const char *symbol;
    const char *c_function;
    const char *source_path;
    int compiled_in;
} akx_nucleus_metadata_t;

")

list(LENGTH ALL_NUCLEI NUCLEUS_COUNT)
file(APPEND ${METADATA_H} "extern const akx_nucleus_metadata_t akx_nucleus_metadata[${NUCLEUS_COUNT}];
extern const size_t akx_nucleus_metadata_count;

#endif
")

set(METADATA_C "${CMAKE_BINARY_DIR}/generated/builtin_metadata.c")
file(WRITE ${METADATA_C} "#include \"builtin_metadata.h\"

const akx_nucleus_metadata_t akx_nucleus_metadata[] = {
")

foreach(NUCLEUS ${ALL_NUCLEI})
    string(REPLACE ":" ";" NUCLEUS_PARTS ${NUCLEUS})
    list(GET NUCLEUS_PARTS 0 SYMBOL)
    list(GET NUCLEUS_PARTS 1 C_FUNCTION)
    list(GET NUCLEUS_PARTS 2 REL_PATH)
    list(GET NUCLEUS_PARTS 3 COMPILE_IN)
    
    file(APPEND ${METADATA_C} "    {\"${SYMBOL}\", \"${C_FUNCTION}\", \"${REL_PATH}\", ${COMPILE_IN}},
")
endforeach()

file(APPEND ${METADATA_C} "};

const size_t akx_nucleus_metadata_count = ${NUCLEUS_COUNT};
")

add_library(akx_nucleus STATIC ${REGISTRY_C} ${METADATA_C})
target_include_directories(akx_nucleus PUBLIC
    ${CMAKE_SOURCE_DIR}/pkg/cell
    ${CMAKE_SOURCE_DIR}/pkg/rt
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_CURRENT_SOURCE_DIR}/fs
    ${CMAKE_CURRENT_SOURCE_DIR}/os
    ${AK24_INCLUDE_DIR}
)
target_link_libraries(akx_nucleus PUBLIC
    akx_cell
    ${AK24_LIBRARIES}
)

