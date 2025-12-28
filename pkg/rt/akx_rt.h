#ifndef AKX_RT_H
#define AKX_RT_H

#include "akx_cell.h"
#include "akx_sv.h"
#include <ak24/cjit.h>
#include <ak24/context.h>
#include <ak24/kernel.h>
#include <ak24/list.h>
#include <stdarg.h>

#define AKX_RT_META_STRING_SIZE_MAX 256

typedef struct akx_rt_error_ctx_t akx_rt_error_ctx_t;

typedef struct akx_runtime_ctx_t akx_runtime_ctx_t;

typedef list_t(akx_cell_t *) akx_cell_list_t;

typedef akx_cell_t *(*akx_builtin_fn)(akx_runtime_ctx_t *, akx_cell_t *);

typedef struct {
  akx_builtin_fn function;
  char *source_path;
  time_t load_time;

  void (*init_fn)(akx_runtime_ctx_t *);
  void (*deinit_fn)(akx_runtime_ctx_t *);
  void (*reload_fn)(akx_runtime_ctx_t *, void *);

  void *module_data;
  const char *module_name;
  ak_cjit_unit_t *unit;
} akx_builtin_info_t;

typedef struct {
  const char **include_paths;
  size_t include_path_count;
  const char **impl_files;
  size_t impl_file_count;
  const char **library_paths;
  size_t library_path_count;
  const char **libraries;
  size_t library_count;
  const char **define_names;
  const char **define_values;
  size_t define_count;
} akx_builtin_compile_opts_t;

akx_runtime_ctx_t *akx_runtime_init(void);

void akx_runtime_deinit(akx_runtime_ctx_t *ctx);

int akx_runtime_start(akx_runtime_ctx_t *ctx, akx_cell_list_t *cells);

ak_context_t *akx_runtime_get_current_scope(akx_runtime_ctx_t *ctx);

akx_parse_error_t *akx_runtime_get_errors(akx_runtime_ctx_t *ctx);

int akx_runtime_load_builtin_ex(akx_runtime_ctx_t *rt, const char *name,
                                const char *root_path,
                                const akx_builtin_compile_opts_t *opts);

akx_cell_t *akx_rt_alloc_cell(akx_runtime_ctx_t *rt, akx_type_t type);
void akx_rt_free_cell(akx_runtime_ctx_t *rt, akx_cell_t *cell);

void akx_rt_set_symbol(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       const char *sym);
void akx_rt_set_int(akx_runtime_ctx_t *rt, akx_cell_t *cell, int value);
void akx_rt_set_real(akx_runtime_ctx_t *rt, akx_cell_t *cell, double value);
void akx_rt_set_string(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       const char *str);
void akx_rt_set_list(akx_runtime_ctx_t *rt, akx_cell_t *cell, akx_cell_t *head);
void akx_rt_set_lambda(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       ak_lambda_t *lambda);

int akx_rt_cell_is_type(akx_cell_t *cell, akx_type_t type);
akx_type_t akx_rt_cell_get_type(akx_cell_t *cell);
const char *akx_rt_cell_as_symbol(akx_cell_t *cell);
int akx_rt_cell_as_int(akx_cell_t *cell);
double akx_rt_cell_as_real(akx_cell_t *cell);
const char *akx_rt_cell_as_string(akx_cell_t *cell);
akx_cell_t *akx_rt_cell_as_list(akx_cell_t *cell);
ak_lambda_t *akx_rt_cell_as_lambda(akx_cell_t *cell);
akx_cell_t *akx_rt_cell_next(akx_cell_t *cell);

size_t akx_rt_list_length(akx_cell_t *list);
akx_cell_t *akx_rt_list_nth(akx_cell_t *list, size_t n);
akx_cell_t *akx_rt_list_append(akx_runtime_ctx_t *rt, akx_cell_t *list,
                               akx_cell_t *item);

ak_context_t *akx_rt_get_scope(akx_runtime_ctx_t *rt);
int akx_rt_scope_set(akx_runtime_ctx_t *rt, const char *key, void *value);
void *akx_rt_scope_get(akx_runtime_ctx_t *rt, const char *key);

void akx_rt_push_scope(akx_runtime_ctx_t *rt);
void akx_rt_pop_scope(akx_runtime_ctx_t *rt);

void akx_rt_error(akx_runtime_ctx_t *rt, const char *message);
void akx_rt_error_at(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                     const char *message);
void akx_rt_error_fmt(akx_runtime_ctx_t *rt, const char *fmt, ...);

akx_cell_t *akx_rt_eval(akx_runtime_ctx_t *rt, akx_cell_t *expr);
akx_cell_t *akx_rt_eval_tail(akx_runtime_ctx_t *rt, akx_cell_t *expr);
akx_cell_t *akx_rt_eval_list(akx_runtime_ctx_t *rt, akx_cell_t *list);
akx_cell_t *akx_rt_eval_and_assert(akx_runtime_ctx_t *rt, akx_cell_t *expr,
                                   akx_type_t expected_type,
                                   const char *error_msg);

void akx_rt_module_set_data(akx_runtime_ctx_t *rt, void *data);
void *akx_rt_module_get_data(akx_runtime_ctx_t *rt);

akx_cell_t *akx_rt_invoke_lambda(akx_runtime_ctx_t *rt, akx_cell_t *lambda_cell,
                                 akx_cell_t *args);

akx_cell_t *akx_rt_alloc_continuation(akx_runtime_ctx_t *rt,
                                      akx_cell_t *lambda_cell,
                                      akx_cell_t *args);

int akx_rt_is_continuation(akx_cell_t *cell);

char *akx_rt_expand_env_vars(const char *path);

void akx_rt_add_builtin(akx_runtime_ctx_t *rt, const char *name,
                        akx_builtin_info_t *info);

int akx_rt_register_builtin(akx_runtime_ctx_t *rt, const char *name,
                            const char *root_path, akx_builtin_fn function,
                            ak_cjit_unit_t *unit,
                            void (*init_fn)(akx_runtime_ctx_t *),
                            void (*deinit_fn)(akx_runtime_ctx_t *),
                            void (*reload_fn)(akx_runtime_ctx_t *, void *));

map_void_t *akx_rt_get_builtins(akx_runtime_ctx_t *rt);

#endif
