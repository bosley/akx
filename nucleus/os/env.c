#include <stdlib.h>
#include <string.h>

akx_cell_t *os_env_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "os/env: requires keyword argument");
    return NULL;
  }

  akx_cell_t *current = args;
  while (current) {
    if (current->type != AKX_TYPE_SYMBOL) {
      akx_rt_error(rt, "os/env: expected keyword (symbol starting with ':')");
      return NULL;
    }

    const char *keyword = akx_rt_cell_as_symbol(current);
    if (keyword[0] != ':') {
      akx_rt_error_fmt(rt, "os/env: keywords must start with ':', got: %s",
                       keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
    if (!current) {
      akx_rt_error_fmt(rt, "os/env: missing value for keyword %s", keyword);
      return NULL;
    }

    if (strcmp(keyword, ":get") == 0) {
      akx_cell_t *var_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_STRING_LITERAL,
                                 "os/env: variable name must be a string");
      if (!var_cell) {
        return NULL;
      }

      const char *var_name = akx_rt_cell_as_string(var_cell);
      const char *var_value = getenv(var_name);
      akx_rt_free_cell(rt, var_cell);

      if (!var_value) {
        akx_cell_t *nil = akx_rt_alloc_cell(rt, AKX_TYPE_SYMBOL);
        akx_rt_set_symbol(rt, nil, "nil");
        return nil;
      }

      akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
      akx_rt_set_string(rt, result, var_value);
      return result;

    } else if (strcmp(keyword, ":set") == 0) {
      akx_cell_t *var_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_STRING_LITERAL,
                                 "os/env: variable name must be a string");
      if (!var_cell) {
        return NULL;
      }

      current = akx_rt_cell_next(current);
      if (!current) {
        akx_rt_free_cell(rt, var_cell);
        akx_rt_error(rt, "os/env: :set requires variable name and value");
        return NULL;
      }

      akx_cell_t *value_cell =
          akx_rt_eval_and_assert(rt, current, AKX_TYPE_STRING_LITERAL,
                                 "os/env: variable value must be a string");
      if (!value_cell) {
        akx_rt_free_cell(rt, var_cell);
        return NULL;
      }

      const char *var_name = akx_rt_cell_as_string(var_cell);
      const char *var_value = akx_rt_cell_as_string(value_cell);

      int result_code = setenv(var_name, var_value, 1);

      akx_rt_free_cell(rt, var_cell);
      akx_rt_free_cell(rt, value_cell);

      akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
      akx_rt_set_int(rt, result, (result_code == 0) ? 1 : 0);
      return result;

    } else {
      akx_rt_error_fmt(rt, "os/env: unknown keyword: %s", keyword);
      return NULL;
    }

    current = akx_rt_cell_next(current);
  }

  akx_rt_error(rt, "os/env: no valid operation specified");
  return NULL;
}
