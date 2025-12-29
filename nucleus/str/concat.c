#include <string.h>

akx_cell_t *str_concat_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 1) {
    akx_rt_error(rt, "str/+ requires at least 1 argument");
    return NULL;
  }

  size_t total_length = 0;
  size_t arg_count = akx_rt_list_length(args);

  akx_cell_t **evaled_args = AK24_ALLOC(sizeof(akx_cell_t *) * arg_count);
  if (!evaled_args) {
    akx_rt_error(rt, "str/+: memory allocation failed");
    return NULL;
  }

  for (size_t i = 0; i < arg_count; i++) {
    akx_cell_t *arg = akx_rt_list_nth(args, i);
    evaled_args[i] = akx_rt_eval_and_assert(rt, arg, AKX_TYPE_STRING_LITERAL,
                                            "str/+ requires string arguments");
    if (!evaled_args[i]) {
      for (size_t j = 0; j < i; j++) {
        akx_rt_free_cell(rt, evaled_args[j]);
      }
      AK24_FREE(evaled_args);
      return NULL;
    }

    const char *str = akx_rt_cell_as_string(evaled_args[i]);
    if (str) {
      total_length += strlen(str);
    }
  }

  char *result_str = AK24_ALLOC(total_length + 1);
  if (!result_str) {
    for (size_t i = 0; i < arg_count; i++) {
      akx_rt_free_cell(rt, evaled_args[i]);
    }
    AK24_FREE(evaled_args);
    akx_rt_error(rt, "str/+: memory allocation failed");
    return NULL;
  }

  result_str[0] = '\0';
  for (size_t i = 0; i < arg_count; i++) {
    const char *str = akx_rt_cell_as_string(evaled_args[i]);
    if (str) {
      strcat(result_str, str);
    }
    akx_rt_free_cell(rt, evaled_args[i]);
  }
  AK24_FREE(evaled_args);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
  akx_rt_set_string(rt, result, result_str);
  AK24_FREE(result_str);

  return result;
}
