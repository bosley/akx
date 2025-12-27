#ifndef AKX_RT_BUILTINS_H
#define AKX_RT_BUILTINS_H

#include "akx_rt.h"

typedef struct {
  akx_runtime_ctx_t *rt;
  const char **param_names;
  size_t param_count;
  akx_cell_t *body;
  akx_cell_t *result;
} akx_lambda_context_t;

void akx_rt_register_bootstrap_builtins(akx_runtime_ctx_t *rt);

#endif
