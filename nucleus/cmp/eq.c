#include <string.h>

akx_cell_t *eq_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  size_t arg_count = akx_rt_list_length(args);
  if (arg_count < 2) {
    akx_rt_error(rt, "eq requires at least 2 arguments");
    return NULL;
  }

  akx_cell_t **evaled_args = AK24_ALLOC(sizeof(akx_cell_t *) * arg_count);
  if (!evaled_args) {
    akx_rt_error(rt, "eq: memory allocation failed");
    return NULL;
  }

  for (size_t i = 0; i < arg_count; i++) {
    akx_cell_t *arg = akx_rt_list_nth(args, i);
    evaled_args[i] = akx_rt_eval(rt, arg);
    if (!evaled_args[i]) {
      for (size_t j = 0; j < i; j++) {
        if (akx_rt_cell_get_type(evaled_args[j]) != AKX_TYPE_LAMBDA) {
          akx_rt_free_cell(rt, evaled_args[j]);
        }
      }
      AK24_FREE(evaled_args);
      return NULL;
    }
  }

  int all_equal = 1;
  akx_type_t first_type = akx_rt_cell_get_type(evaled_args[0]);

  for (size_t i = 1; i < arg_count && all_equal; i++) {
    akx_type_t current_type = akx_rt_cell_get_type(evaled_args[i]);
    if (first_type != current_type) {
      all_equal = 0;
      break;
    }

    switch (first_type) {
    case AKX_TYPE_INTEGER_LITERAL:
      if (akx_rt_cell_as_int(evaled_args[0]) !=
          akx_rt_cell_as_int(evaled_args[i])) {
        all_equal = 0;
      }
      break;
    case AKX_TYPE_REAL_LITERAL:
      if (akx_rt_cell_as_real(evaled_args[0]) !=
          akx_rt_cell_as_real(evaled_args[i])) {
        all_equal = 0;
      }
      break;
    case AKX_TYPE_STRING_LITERAL: {
      const char *first_str = akx_rt_cell_as_string(evaled_args[0]);
      const char *current_str = akx_rt_cell_as_string(evaled_args[i]);
      if (strcmp(first_str, current_str) != 0) {
        all_equal = 0;
      }
      break;
    }
    case AKX_TYPE_SYMBOL: {
      const char *first_sym = akx_rt_cell_as_symbol(evaled_args[0]);
      const char *current_sym = akx_rt_cell_as_symbol(evaled_args[i]);
      if (strcmp(first_sym, current_sym) != 0) {
        all_equal = 0;
      }
      break;
    }
    case AKX_TYPE_LAMBDA:
      if (akx_rt_cell_as_lambda(evaled_args[0]) !=
          akx_rt_cell_as_lambda(evaled_args[i])) {
        all_equal = 0;
      }
      break;
    default:
      all_equal = 0;
      break;
    }
  }

  for (size_t i = 0; i < arg_count; i++) {
    if (akx_rt_cell_get_type(evaled_args[i]) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, evaled_args[i]);
    }
  }
  AK24_FREE(evaled_args);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, all_equal);
  return result;
}

