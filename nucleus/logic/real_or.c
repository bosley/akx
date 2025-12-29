#include <math.h>

akx_cell_t *real_or_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "real/or: requires at least 2 arguments");
    return NULL;
  }

  akx_cell_t *current = args;
  while (current) {
    akx_cell_t *evaled = akx_rt_eval(rt, current);
    if (!evaled) {
      return NULL;
    }

    int is_true = 0;
    if (akx_rt_cell_get_type(evaled) == AKX_TYPE_REAL_LITERAL &&
        fabs(akx_rt_cell_as_real(evaled)) > 1e-10) {
      is_true = 1;
    }

    akx_rt_free_cell(rt, evaled);

    if (is_true) {
      akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
      akx_rt_set_int(rt, ret, 1);
      return ret;
    }

    current = akx_rt_cell_next(current);
  }

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, ret, 0);
  return ret;
}
