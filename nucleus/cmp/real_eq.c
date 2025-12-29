#include <math.h>

akx_cell_t *real_eq_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  size_t arg_count = akx_rt_list_length(args);
  if (arg_count < 2) {
    akx_rt_error(rt, "real/eq requires at least 2 arguments");
    return NULL;
  }

  akx_cell_t **evaled_args = AK24_ALLOC(sizeof(akx_cell_t *) * arg_count);
  if (!evaled_args) {
    akx_rt_error(rt, "real/eq: memory allocation failed");
    return NULL;
  }

  for (size_t i = 0; i < arg_count; i++) {
    akx_cell_t *arg = akx_rt_list_nth(args, i);
    evaled_args[i] = akx_rt_eval_and_assert(rt, arg, AKX_TYPE_REAL_LITERAL,
                                            "real/eq requires real arguments");
    if (!evaled_args[i]) {
      for (size_t j = 0; j < i; j++) {
        akx_rt_free_cell(rt, evaled_args[j]);
      }
      AK24_FREE(evaled_args);
      return NULL;
    }
  }

  int all_equal = 1;
  double first_val = akx_rt_cell_as_real(evaled_args[0]);

  for (size_t i = 1; i < arg_count && all_equal; i++) {
    double current_val = akx_rt_cell_as_real(evaled_args[i]);
    if (fabs(first_val - current_val) > 1e-10) {
      all_equal = 0;
    }
  }

  for (size_t i = 0; i < arg_count; i++) {
    akx_rt_free_cell(rt, evaled_args[i]);
  }
  AK24_FREE(evaled_args);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, all_equal);
  return result;
}
