akx_cell_t *delete_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (!args) {
    akx_rt_error(rt, "fs/delete: requires a file path");
    return NULL;
  }

  akx_cell_t *path_cell = akx_rt_eval(rt, args);
  if (!path_cell || !akx_rt_cell_is_type(path_cell, AKX_TYPE_STRING_LITERAL)) {
    akx_rt_error(rt, "fs/delete: path must be a string");
    if (path_cell)
      akx_rt_free_cell(rt, path_cell);
    return NULL;
  }

  const char *path = akx_rt_cell_as_string(path_cell);
  char *expanded_path = akx_rt_expand_env_vars(path);
  const char *actual_path = expanded_path ? expanded_path : path;

  int result_code = unlink(actual_path);

  if (expanded_path) {
    free(expanded_path);
  }

  akx_rt_free_cell(rt, path_cell);

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, (result_code == 0) ? 1 : 0);
  return result;
}
