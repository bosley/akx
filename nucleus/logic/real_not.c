#include <math.h>

akx_cell_t *real_not_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 1) {
    akx_rt_error(rt, "real/not: requires exactly 1 argument");
    return NULL;
  }

  akx_cell_t *arg = akx_rt_list_nth(args, 0);
  akx_cell_t *evaled = akx_rt_eval(rt, arg);
  if (!evaled) {
    return NULL;
  }

  int is_true = 0;
  if (akx_rt_cell_get_type(evaled) == AKX_TYPE_REAL_LITERAL &&
      fabs(akx_rt_cell_as_real(evaled)) > 1e-10) {
    is_true = 1;
  }

  akx_rt_free_cell(rt, evaled);

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, ret, is_true ? 0 : 1);
  return ret;
}
