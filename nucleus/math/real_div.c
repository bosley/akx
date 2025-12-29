#include <math.h>

akx_cell_t *real_div(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "real// requires at least 2 arguments");
    return NULL;
  }

  akx_cell_t *first_arg = akx_rt_list_nth(args, 0);
  akx_cell_t *evaled_first = akx_rt_eval_and_assert(
      rt, first_arg, AKX_TYPE_REAL_LITERAL, "real// requires real arguments");
  if (!evaled_first) {
    return NULL;
  }

  double result_val = akx_rt_cell_as_real(evaled_first);
  akx_rt_free_cell(rt, evaled_first);

  akx_cell_t *current = akx_rt_cell_next(args);
  while (current) {
    akx_cell_t *evaled = akx_rt_eval_and_assert(
        rt, current, AKX_TYPE_REAL_LITERAL, "real// requires real arguments");
    if (!evaled) {
      return NULL;
    }

    double divisor = akx_rt_cell_as_real(evaled);
    if (fabs(divisor) < 1e-10) {
      akx_rt_free_cell(rt, evaled);
      akx_rt_error(rt, "real//: division by zero");
      return NULL;
    }

    result_val /= divisor;
    akx_rt_free_cell(rt, evaled);
    current = akx_rt_cell_next(current);
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_REAL_LITERAL);
  akx_rt_set_real(rt, result, result_val);
  return result;
}
