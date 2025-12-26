#include "math_helper.h"

akx_cell_t *advanced_math(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) != 1) {
    akx_rt_error(rt, "advanced_math requires 1 argument");
    return NULL;
  }

  akx_cell_t *evaled = akx_rt_eval_and_assert(
      rt, akx_rt_list_nth(args, 0), AKX_TYPE_INTEGER_LITERAL,
      "advanced_math requires an integer argument");
  if (!evaled)
    return NULL;

  int x = akx_rt_cell_as_int(evaled);
  akx_rt_free_cell(rt, evaled);

  int result = math_square(x) + math_multiply(x, 10);

  akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, ret, result);
  return ret;
}
