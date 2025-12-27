#include <string.h>

static int cells_equal(akx_cell_t *a, akx_cell_t *b) {
  akx_type_t type_a = akx_rt_cell_get_type(a);
  akx_type_t type_b = akx_rt_cell_get_type(b);

  if (type_a != type_b) {
    return 0;
  }

  switch (type_a) {
  case AKX_TYPE_INTEGER_LITERAL:
    return akx_rt_cell_as_int(a) == akx_rt_cell_as_int(b);
  case AKX_TYPE_REAL_LITERAL:
    return akx_rt_cell_as_real(a) == akx_rt_cell_as_real(b);
  case AKX_TYPE_STRING_LITERAL:
    return strcmp(akx_rt_cell_as_string(a), akx_rt_cell_as_string(b)) == 0;
  case AKX_TYPE_SYMBOL:
    return strcmp(akx_rt_cell_as_symbol(a), akx_rt_cell_as_symbol(b)) == 0;
  case AKX_TYPE_LAMBDA:
    return akx_rt_cell_as_lambda(a) == akx_rt_cell_as_lambda(b);
  default:
    return 0;
  }
}

akx_cell_t *neq_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  size_t arg_count = akx_rt_list_length(args);
  if (arg_count < 2) {
    akx_rt_error(rt, "neq requires at least 2 arguments");
    return NULL;
  }

  akx_cell_t **evaled_args = AK24_ALLOC(sizeof(akx_cell_t *) * arg_count);
  if (!evaled_args) {
    akx_rt_error(rt, "neq: memory allocation failed");
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

  int all_different = 1;
  for (size_t i = 0; i < arg_count && all_different; i++) {
    for (size_t j = i + 1; j < arg_count && all_different; j++) {
      if (cells_equal(evaled_args[i], evaled_args[j])) {
        all_different = 0;
      }
    }
  }

  for (size_t i = 0; i < arg_count; i++) {
    if (akx_rt_cell_get_type(evaled_args[i]) != AKX_TYPE_LAMBDA) {
      akx_rt_free_cell(rt, evaled_args[i]);
    }
  }
  AK24_FREE(evaled_args);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, all_different);
  return result;
}

