#ifndef AKX_RT_H
#define AKX_RT_H

#include "akx_cell.h"
#include "akx_sv.h"
#include <ak24/context.h>
#include <ak24/kernel.h>
#include <ak24/list.h>
#include <stdarg.h>

typedef struct akx_rt_error_ctx_t akx_rt_error_ctx_t;

typedef struct akx_runtime_ctx_t akx_runtime_ctx_t;

typedef list_t(akx_cell_t *) akx_cell_list_t;

typedef akx_cell_t *(*akx_builtin_fn)(akx_runtime_ctx_t *, akx_cell_t *);

typedef struct {
  akx_builtin_fn function;
  char *source_path;
  time_t load_time;
} akx_builtin_info_t;

akx_runtime_ctx_t *akx_runtime_init(void);

void akx_runtime_deinit(akx_runtime_ctx_t *ctx);

int akx_runtime_start(akx_runtime_ctx_t *ctx, akx_cell_list_t *cells);

ak_context_t *akx_runtime_get_current_scope(akx_runtime_ctx_t *ctx);

int akx_runtime_load_builtin(akx_runtime_ctx_t *rt, const char *name,
                              const char *source_path);

akx_cell_t *akx_rt_alloc_cell(akx_runtime_ctx_t *rt, akx_type_t type);
void akx_rt_free_cell(akx_runtime_ctx_t *rt, akx_cell_t *cell);

void akx_rt_set_symbol(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       const char *sym);
void akx_rt_set_int(akx_runtime_ctx_t *rt, akx_cell_t *cell, int value);
void akx_rt_set_real(akx_runtime_ctx_t *rt, akx_cell_t *cell, double value);
void akx_rt_set_string(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                       const char *str);
void akx_rt_set_list(akx_runtime_ctx_t *rt, akx_cell_t *cell,
                     akx_cell_t *head);

int akx_rt_cell_is_type(akx_cell_t *cell, akx_type_t type);
const char *akx_rt_cell_as_symbol(akx_cell_t *cell);
int akx_rt_cell_as_int(akx_cell_t *cell);
double akx_rt_cell_as_real(akx_cell_t *cell);
const char *akx_rt_cell_as_string(akx_cell_t *cell);
akx_cell_t *akx_rt_cell_as_list(akx_cell_t *cell);
akx_cell_t *akx_rt_cell_next(akx_cell_t *cell);

size_t akx_rt_list_length(akx_cell_t *list);
akx_cell_t *akx_rt_list_nth(akx_cell_t *list, size_t n);
akx_cell_t *akx_rt_list_append(akx_runtime_ctx_t *rt, akx_cell_t *list,
                               akx_cell_t *item);

ak_context_t *akx_rt_get_scope(akx_runtime_ctx_t *rt);
int akx_rt_scope_set(akx_runtime_ctx_t *rt, const char *key, void *value);
void *akx_rt_scope_get(akx_runtime_ctx_t *rt, const char *key);

void akx_rt_error(akx_runtime_ctx_t *rt, const char *message);
void akx_rt_error_fmt(akx_runtime_ctx_t *rt, const char *fmt, ...);

akx_cell_t *akx_rt_eval(akx_runtime_ctx_t *rt, akx_cell_t *expr);
akx_cell_t *akx_rt_eval_list(akx_runtime_ctx_t *rt, akx_cell_t *list);
akx_cell_t *akx_rt_eval_and_assert(akx_runtime_ctx_t *rt, akx_cell_t *expr,
                                   akx_type_t expected_type,
                                   const char *error_msg);

#endif
