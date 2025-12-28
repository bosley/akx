#include <stdio.h>
#include <stdlib.h>
#include <string.h>

akx_cell_t *rename_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/rename: requires old and new path arguments");
    return NULL;
  }

  akx_cell_t *old_path_cell = akx_rt_eval(rt, args);
  if (!old_path_cell ||
      !akx_rt_cell_is_type(old_path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/rename: old path must be a string");
    if (old_path_cell)
      akx_rt_free_cell(rt, old_path_cell);
    return NULL;
  }

  akx_cell_t *new_path_arg = akx_rt_cell_next(args);
  if (!new_path_arg) {
    akx_rt_error(rt, "fs/rename: requires new path argument");
    akx_rt_free_cell(rt, old_path_cell);
    return NULL;
  }

  akx_cell_t *new_path_cell = akx_rt_eval(rt, new_path_arg);
  if (!new_path_cell ||
      !akx_rt_cell_is_type(new_path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/rename: new path must be a string");
    akx_rt_free_cell(rt, old_path_cell);
    if (new_path_cell)
      akx_rt_free_cell(rt, new_path_cell);
    return NULL;
  }

  const char *old_path = akx_rt_cell_as_string(old_path_cell);
  const char *new_path = akx_rt_cell_as_string(new_path_cell);

  char *expanded_old = akx_rt_expand_env_vars(old_path);
  char *expanded_new = akx_rt_expand_env_vars(new_path);
  const char *actual_old = expanded_old ? expanded_old : old_path;
  const char *actual_new = expanded_new ? expanded_new : new_path;

  int result_code = rename(actual_old, actual_new);

  if (expanded_old)
    free(expanded_old);
  if (expanded_new)
    free(expanded_new);
  akx_rt_free_cell(rt, old_path_cell);
  akx_rt_free_cell(rt, new_path_cell);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, (result_code == 0) ? 1 : 0);
  return result;
}
