#ifndef AKX_RT_COMPILER_H
#define AKX_RT_COMPILER_H

#include "akx_rt.h"

int akx_compiler_extract_string_list(akx_runtime_ctx_t *rt,
                                     akx_cell_t *list_cell,
                                     const char ***out_strings,
                                     size_t *out_count);

int akx_compiler_extract_define_list(akx_runtime_ctx_t *rt,
                                     akx_cell_t *list_cell,
                                     const char ***out_names,
                                     const char ***out_values,
                                     size_t *out_count);

int akx_compiler_parse_linker_options(akx_runtime_ctx_t *rt,
                                      akx_cell_t *linker_cell,
                                      akx_builtin_compile_opts_t *opts);

void akx_compiler_free_compile_opts(akx_builtin_compile_opts_t *opts);

#endif
