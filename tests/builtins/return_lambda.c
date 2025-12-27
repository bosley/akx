#include <ak24/lambda.h>

akx_cell_t *return_lambda(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  (void)args;

  ak_lambda_t *lambda = ak_lambda_new();
  if (!lambda) {
    akx_rt_error(rt, "Failed to create lambda");
    return NULL;
  }

  akx_cell_t *lambda_cell = akx_rt_alloc_cell(rt, AKX_TYPE_LAMBDA);
  akx_rt_set_lambda(rt, lambda_cell, lambda);
  return lambda_cell;
}
